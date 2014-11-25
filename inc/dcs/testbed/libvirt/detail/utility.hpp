/**
 * \file dcs/testbed/libvirt/detail/utility.hpp
 *
 * \brief Utilities to communicate with libvirt.
 *
 * \author Marco Guazzone (marco.guazzone@gmail.com)
 *
 * <hr/>
 *
 * Copyright 2012 Marco Guazzone (marco.guazzone@gmail.com)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef DCS_TESTBED_LIBVIRT_DETAIL_UTILITY_HPP
#define DCS_TESTBED_LIBVIRT_DETAIL_UTILITY_HPP


#include <boost/cstdint.hpp>
#include <cstdlib>
#include <cstring>
#include <dcs/assert.hpp>
#include <dcs/exception.hpp>
#include <dcs/uri.hpp>
#include <libvirt/libvirt.h>
#include <libvirt/virterror.h>
#include <sstream>
#include <stdexcept>
#include <string>


// This macro is available only in recent versions of libvirt.
// It is useful for enabling code that only works since a given version
#if !defined(LIBVIR_CHECK_VERSION)

# define LIBVIR_CHECK_VERSION(major, minor, micro) \
     ((major) * 1000000 + (minor) * 1000 + (micro) <= LIBVIR_VERSION_NUMBER)

#endif // LIBVIR_CHECK_VERSION


namespace dcs { namespace testbed { namespace libvirt { namespace detail {

inline
::std::string vmm_uri(::std::string const& uri)
{
    ::std::ostringstream oss;

    ::dcs::uri u(uri);
    if (!u.relative())
    {
        oss << u.scheme() << "://" << u.authority() << "/";
    }

    return oss.str();
}

inline
::std::string vm_name(::std::string const& uri)
{
    ::dcs::uri u(uri);

	::std::string name(u.path_etc());

	if (name[0] == '/')
	{
		return name.substr(1);
	}
	return name;
}

::std::string to_string(virTypedParameter const& param)
{
	::std::ostringstream oss;

	switch (param.type)
	{
		case VIR_TYPED_PARAM_INT:
			oss << param.value.i;
			break;
		case VIR_TYPED_PARAM_UINT:
			oss << param.value.ui;
			break;
		case VIR_TYPED_PARAM_LLONG:
			oss << param.value.l;
			break;
		case VIR_TYPED_PARAM_ULLONG:
			oss << param.value.ul;
			break;
		case VIR_TYPED_PARAM_DOUBLE:
			oss << param.value.d;
			break;
		case VIR_TYPED_PARAM_BOOLEAN:
			oss << param.value.b;
			break;
#if LIBVIR_CHECK_VERSION(0,10,0)
//#if LIBVIR_VERSION_NUMBER >= 10000
		case VIR_TYPED_PARAM_STRING:
			oss << param.value.s;
			break;
//#endif // LIBVIR_VERSION_NUMBER
#endif // LIBVIR_CHECK_VERSION
	}

	return oss.str();
}

::std::string last_error(virConnectPtr conn)
{
	DCS_DEBUG_ASSERT( conn );

	::std::string err_str;
//XXX: old style. Deprecated by virGestLastError
//	virErrorPtr err = new virError();
//
//	int ret = virConnCopyLastError(conn, err);
//	switch (ret)
//[/XXX]
	virErrorPtr err = virGetLastError();
	switch (err->code)
	{
		case VIR_ERR_OK:
			// No error found
			break;
		case -1:
			err_str = "Parameter error when attempting to get last error";
			break;
		default:
			err_str = err->message;
			break;
	}

//[XXX]: old style. Deprecated by virGestLastError
//	virResetError(err);
//	delete err;
//[/XXX]

	return err_str;
}

virConnectPtr connect(::std::string const& uri)
{
	// Connect to libvirtd daemons
	//
	// virConnectOpenAuth is called here with all default parameters,
	// except, possibly, the URI of the hypervisor.
	//
	virConnectPtr conn = virConnectOpenAuth((!uri.empty() ? uri.c_str() : 0), virConnectAuthPtrDefault, 0);
	if (0 == conn)
	{
		::std::ostringstream oss;
		oss << "No connection to hypervisor with URI '" << uri << "': " << last_error(0);
		DCS_EXCEPTION_THROW(::std::runtime_error, oss.str());
	}

	return conn;
}

void disconnect(virConnectPtr conn)
{
	DCS_DEBUG_ASSERT( conn );

	int ret = virConnectClose(conn);
	if (ret < 0)
	{
		throw ::std::runtime_error(last_error(conn));
	}
	if (ret > 0)
	{
		//TODO
		// One or more references are possible leaked after disconnect from the hypervisor
	}
}

::std::string hypervisor_info(virConnectPtr conn)
{
	/* virConnectGetType returns a pointer to a static string, so no
	* allocation or freeing is necessary; it is possible for the call
	* to fail if, for example, there is no connection to a
	* hypervisor, so check what it returns. */
	const char *hvType(0);
	hvType = virConnectGetType(conn);
	if (0 == hvType)
	{
		::std::ostringstream oss;
		oss << "Failed to get hypervisor type: " << last_error(conn);
		DCS_EXCEPTION_THROW(::std::runtime_error, oss.str());
	}

	unsigned long hvVer;

	if (0 != virConnectGetVersion(conn, &hvVer))
	{
		::std::ostringstream oss;
		oss << "Failed to get hypervisor version: " << last_error(conn);
		DCS_EXCEPTION_THROW(::std::runtime_error, oss.str());
	}

	unsigned long major;
	unsigned long minor;
	unsigned long release;

	major = hvVer / 1000000;
	hvVer %= 1000000;
	minor = hvVer / 1000;
	release = hvVer % 1000;

	::std::ostringstream oss;

	oss << hvType << " (" << major << "." << minor << "." << release << ")";

	return oss.str();
}

int max_num_cpus(virConnectPtr conn)
{
	DCS_DEBUG_ASSERT( conn );

	int ret;

#if LIBVIR_CHECK_VERSION(1,0,0)
	// First try to use virNodeGetCPUMap since it is the lightest way
    ret = virNodeGetCPUMap(conn, 0, 0, 0);
	if (-1 == ret)
	{
		// Fall back to virNodeGetInfo
		virNodeInfo info;

		ret = virNodeGetInfo(conn, &info);
		if (-1 == ret)
		{
			::std::ostringstream oss;
			oss << "Failed to get node info: " << last_error(conn);

			DCS_EXCEPTION_THROW(::std::runtime_error, oss.str());
		}

		ret = VIR_NODEINFO_MAXCPUS(info);
	}
#else
	// Function "virNodeGetCPUMap" not available
	virNodeInfo info;

	ret = virNodeGetInfo(conn, &info);
	if (-1 == ret)
	{
		::std::ostringstream oss;
		oss << "Failed to get node info: " << last_error(conn);

		DCS_EXCEPTION_THROW(::std::runtime_error, oss.str());
	}

	ret = VIR_NODEINFO_MAXCPUS(info);
#endif // LIBVIR_CHECK_VERSION

	return ret;
}

int max_supported_num_vcpus(virConnectPtr conn)
{
	DCS_DEBUG_ASSERT( conn );

	int ret;

	ret = virConnectGetMaxVcpus(conn, 0);
	if (-1 == ret)
	{
		::std::ostringstream oss;
		oss << "Failed to get the max number of vCPUs: " << last_error(conn);

		DCS_EXCEPTION_THROW(::std::runtime_error, oss.str());
	}

	return ret;
}

virDomainPtr connect_domain(virConnectPtr conn, ::std::string const& name)
{
	DCS_DEBUG_ASSERT( conn );

	virDomainPtr dom = virDomainLookupByName(conn, name.c_str());
	if (0 == dom)
	{
		::std::ostringstream oss;
		oss << "Failed to get Domain for \"" << name << "\": " << last_error(conn);
		DCS_EXCEPTION_THROW(::std::runtime_error, oss.str());
	}

	return dom;
}

void disconnect_domain(virConnectPtr conn, virDomainPtr dom)
{
	DCS_DEBUG_ASSERT( conn );
	DCS_DEBUG_ASSERT( dom );

	if (0 != virDomainFree(dom))
	{
		::std::ostringstream oss;
		oss << "Failed to free data for domain \"" << virDomainGetName(dom) << "\": " << last_error(conn);
		DCS_EXCEPTION_THROW(::std::runtime_error, oss.str());
	}
}

template <typename ParamT>
struct sched_param_value;

template <>
struct sched_param_value<int>
{
	int& operator()(virTypedParameter& param)
	{
		return param.value.i;
	}

	int operator()(virTypedParameter const& param) const
	{
		return param.value.i;
	}
}; // sched_param_value

template <>
struct sched_param_value<unsigned int>
{
	unsigned int& operator()(virTypedParameter& param)
	{
		return param.value.ui;
	}

	unsigned int operator()(virTypedParameter const& param) const
	{
		return param.value.ui;
	}
}; // sched_param_value

template <>
struct sched_param_value<long long int>
{
	long long int& operator()(virTypedParameter& param)
	{
		return param.value.l;
	}

	long long int operator()(virTypedParameter const& param) const
	{
		return param.value.l;
	}
}; // sched_param_value

template <>
struct sched_param_value<unsigned long long int>
{
	unsigned long long int& operator()(virTypedParameter& param)
	{
		return param.value.ul;
	}

	unsigned long long int operator()(virTypedParameter const& param) const
	{
		return param.value.ul;
	}
}; // sched_param_value

template <>
struct sched_param_value<double>
{
	double& operator()(virTypedParameter& param)
	{
		return param.value.d;
	}

	double operator()(virTypedParameter const& param) const
	{
		return param.value.d;
	}
}; // sched_param_value

template <>
struct sched_param_value<char>
{
	char& operator()(virTypedParameter& param)
	{
		return param.value.b;
	}

	char operator()(virTypedParameter const& param) const
	{
		return param.value.b;
	}
}; // sched_param_value

#if LIBVIR_CHECK_VERSION(0,10,0)
//#if LIBVIR_VERSION_NUMBER >= 10000
template <>
struct sched_param_value<char*>
{
	char const* operator()(virTypedParameter& param)
	{
		return param.value.s;
	}

	char* operator()(virTypedParameter const& param) const
	{
		return param.value.s;
	}
}; // sched_param_value
//#endif // LIBVIR_VERSION_NUMBER
#endif // LIBVIR_CHECK_VERSION

template <typename ParamT>
ParamT sched_param(virConnectPtr conn, virDomainPtr dom, ::std::string const& name, int flags)
{
	DCS_DEBUG_ASSERT( conn );
	DCS_DEBUG_ASSERT( dom );

	int ret(0);

	char* sched(0);
	int sched_nparams(0);

	sched = virDomainGetSchedulerType(dom, &sched_nparams);
	if (0 != ret)
	{
		::std::ostringstream oss;
		oss << "Failed to get scheduler type for domain \"" << virDomainGetName(dom) << "\": " << last_error(conn);
		DCS_EXCEPTION_THROW(::std::runtime_error, oss.str());
	}
	DCS_DEBUG_TRACE("Scheduler: " << ::std::string(sched));
	::std::free(sched);

	virTypedParameterPtr sched_params = new virTypedParameter[sched_nparams];
	ret = virDomainGetSchedulerParametersFlags(dom, sched_params, &sched_nparams, flags);
	if (0 != ret)
	{
		::std::ostringstream oss;
		oss << "Failed to get scheduler parameters for domain \"" << virDomainGetName(dom) << "\": " << last_error(conn);
		DCS_EXCEPTION_THROW(::std::runtime_error, oss.str());
	}
#ifdef DCS_DEBUG
	for (int i = 0; i < sched_nparams; ++i)
	{
		DCS_DEBUG_TRACE("Scheduler parameter #" << (i+1) << ": <" << sched_params[i].field << "," << to_string(sched_params[i]) << ">");
	}
#endif // DCS_DEBUG

	ParamT value;
	bool found(false);
	for (int i = 0; i < sched_nparams && !found; ++i)
	{
		if (!::std::strncmp(name.c_str(), sched_params[i].field, VIR_TYPED_PARAM_FIELD_LENGTH))
		{
			value = sched_param_value<ParamT>()(sched_params[i]);
			found = true;
		}
	}

	delete[] sched_params;

	if (!found)
	{
		::std::ostringstream oss;
		oss << "Failed to get scheduler parameter '" << name << "' for domain \"" << virDomainGetName(dom) << "\": Not Found";
		DCS_EXCEPTION_THROW(::std::runtime_error, oss.str());
	}

	return value;
}

template <typename ParamT>
void sched_param(virConnectPtr conn, virDomainPtr dom, ::std::string const& name, ParamT value, int flags)
{
	DCS_DEBUG_ASSERT( conn );
	DCS_DEBUG_ASSERT( dom );

	int ret(0);

	char* sched(0);
	int sched_nparams(0);

	sched = virDomainGetSchedulerType(dom, &sched_nparams);
	if (0 != ret)
	{
		::std::ostringstream oss;
		oss << "Failed to get scheduler type for domain \"" << virDomainGetName(dom) << "\": " << last_error(conn);
		DCS_EXCEPTION_THROW(::std::runtime_error, oss.str());
	}
	DCS_DEBUG_TRACE("Scheduler: " << ::std::string(sched));
	::std::free(sched);

	virTypedParameterPtr sched_params = new virTypedParameter[sched_nparams];
	ret = virDomainGetSchedulerParametersFlags(dom, sched_params, &sched_nparams, flags);
	if (0 != ret)
	{
		::std::ostringstream oss;
		oss << "Failed to get scheduler parameters for domain \"" << virDomainGetName(dom) << "\": " << last_error(conn);
		DCS_EXCEPTION_THROW(::std::runtime_error, oss.str());
	}
#ifdef DCS_DEBUG
	for (int i = 0; i < sched_nparams; ++i)
	{
		DCS_DEBUG_TRACE("Scheduler parameter #" << (i+1) << ": <" << sched_params[i].field << "," << to_string(sched_params[i]) << ">");
	}
#endif // DCS_DEBUG

	bool found(false);
	for (int i = 0; i < sched_nparams && !found; ++i)
	{
		if (!::std::strncmp(name.c_str(), sched_params[i].field, VIR_TYPED_PARAM_FIELD_LENGTH))
		{
			sched_param_value<ParamT>()(sched_params[i]) = value;
			found = true;
		}
	}

	if (found)
	{
		ret = virDomainSetSchedulerParametersFlags(dom, sched_params, sched_nparams, flags);
		if (0 != ret)
		{
			::std::ostringstream oss;
			oss << "Failed to set scheduler parameters for domain \"" << virDomainGetName(dom) << "\": " << last_error(conn);
			DCS_EXCEPTION_THROW(::std::runtime_error, oss.str());
		}
	}

	delete[] sched_params;

	if (!found)
	{
		::std::ostringstream oss;
		oss << "Failed to set scheduler parameter '" << name << "' for domain \"" << virDomainGetName(dom) << "\": Not Found";
		DCS_EXCEPTION_THROW(::std::runtime_error, oss.str());
	}
}

int num_vcpus(virConnectPtr conn, virDomainPtr dom, int flags)
{
	DCS_DEBUG_ASSERT( conn );
	DCS_DEBUG_ASSERT( dom );

	int ret = virDomainGetVcpusFlags(dom, flags);
	if (0 > ret)
	{
		::std::ostringstream oss;
		oss << "Failed to query the number of vCPUs for domain \"" << virDomainGetName(dom) << "\": " << last_error(conn);
		DCS_EXCEPTION_THROW(::std::runtime_error, oss.str());
	}

	return ret;
}

int num_cpus(virConnectPtr conn, virDomainPtr dom, int flags)
{
	DCS_DEBUG_ASSERT( conn );
	DCS_DEBUG_ASSERT( dom );

	int ret = 0;

	virDomainInfo info;
	ret = virDomainGetInfo(dom, &info);
	if (0 > ret)
	{
		::std::ostringstream oss;
		oss << "Failed to query information for domain \"" << virDomainGetName(dom) << "\": " << last_error(conn);
		DCS_EXCEPTION_THROW(::std::runtime_error, oss.str());
	}

	const int nvcpus = info.nrVirtCpu;
	const int maxncpus = max_num_cpus(conn);

	const ::std::size_t cpumaplen = VIR_CPU_MAPLEN(maxncpus);
	unsigned char* cpumaps = new unsigned char[nvcpus*cpumaplen];

	ret = virDomainGetVcpuPinInfo(dom, info.nrVirtCpu, cpumaps, cpumaplen, flags);
	if (0 > ret)
	{
		::std::ostringstream oss;
		oss << "Failed to query the number of vCPUs for domain \"" << virDomainGetName(dom) << "\": " << last_error(conn);
		DCS_EXCEPTION_THROW(::std::runtime_error, oss.str());
	}

	int ncpu = 0;
	for (int cpu = 0; cpu < maxncpus; ++cpu)
	{
		for (std::size_t vcpu = 0; vcpu < cpumaplen; ++vcpu)
		{
			//if (VIR_CPU_USED(VIR_GET_CPUMAP(cpumaps, cpumaplen, vcpu), cpu))
			if (VIR_CPU_USABLE(cpumaps, cpumaplen, vcpu, cpu))
			{
				++ncpu;
				break;
			}
		}
	}

	delete[] cpumaps;

	return ncpu;
}

unsigned int domain_id(virConnectPtr conn, virDomainPtr dom)
{
	DCS_DEBUG_ASSERT( conn );
	DCS_DEBUG_ASSERT( dom );

	int ret(0);

	ret = virDomainGetID(dom);
	if (0 > ret)
	{
		::std::ostringstream oss;
		oss << "Failed to query the ID for domain \"" << virDomainGetName(dom) << "\": " << last_error(conn);
		DCS_EXCEPTION_THROW(::std::runtime_error, oss.str());
	}

	return ret;
}

::std::string domain_name(virConnectPtr conn, virDomainPtr dom)
{
	DCS_DEBUG_ASSERT( conn );
	DCS_DEBUG_ASSERT( dom );

	char const* ret = 0;

	ret = virDomainGetName(dom);
	if (0 == ret)
	{
		::std::ostringstream oss;
		oss << "Failed to query the name for domain: " << last_error(conn);
		DCS_EXCEPTION_THROW(::std::runtime_error, oss.str());
	}

	return ret;
}

::std::string domain_hostname(virConnectPtr conn, virDomainPtr dom)
{
	DCS_DEBUG_ASSERT( conn );
	DCS_DEBUG_ASSERT( dom );

	char const* ret = 0;

	ret = virDomainGetHostname(dom, 0);
	if (0 == ret)
	{
		::std::ostringstream oss;
		oss << "Failed to query the hostname for domain: " << last_error(conn);
		DCS_EXCEPTION_THROW(::std::runtime_error, oss.str());
	}

	return ret;
}

unsigned long current_memory(virConnectPtr conn, virDomainPtr dom)
{
	::virDomainInfo node_info;
	int ret = ::virDomainGetInfo(dom, &node_info);
	if (0 > ret)
	{
		::std::ostringstream oss;
		oss << "Failed to query the current memory for domain \"" << ::virDomainGetName(dom) << "\": " << last_error(conn);
		DCS_EXCEPTION_THROW(::std::runtime_error, oss.str());
	}

	return node_info.memory;
}

void current_memory(virConnectPtr conn, virDomainPtr dom, unsigned long mem)
{
	int ret = ::virDomainSetMemory(dom, mem);
	//int ret = ::virDomainSetMemoryFlags(dom, mem, VIR_DOMAIN_AFFECT_CURRENT);
	if (0 > ret)
	{
		::std::ostringstream oss;
		oss << "Failed to set the current memory for domain \"" << ::virDomainGetName(dom) << "\": " << last_error(conn);
		DCS_EXCEPTION_THROW(::std::runtime_error, oss.str());
	}
}

unsigned long config_max_memory(virConnectPtr conn, virDomainPtr dom)
{
	unsigned long mem = ::virDomainGetMaxMemory(dom);
	if (0 > mem)
	{
		::std::ostringstream oss;
		oss << "Failed to query the config max memory for domain \"" << ::virDomainGetName(dom) << "\": " << last_error(conn);
		DCS_EXCEPTION_THROW(::std::runtime_error, oss.str());
	}

	return mem;
}

void config_max_memory(virConnectPtr conn, virDomainPtr dom, unsigned long mem)
{
	//int ret = ::virDomainSetMemoryFlags(dom, mem, VIR_DOMAIN_AFFECT_CURRENT | VIR_DOMAIN_MEM_CONFIG | VIR_DOMAIN_MEM_MAXIMUM);
	int ret = ::virDomainSetMaxMemory(dom, mem);
	if (0 > ret)
	{
		::std::ostringstream oss;
		oss << "Failed to set the config max memory for domain \"" << ::virDomainGetName(dom) << "\": " << last_error(conn);
		DCS_EXCEPTION_THROW(::std::runtime_error, oss.str());
	}
}

unsigned long max_memory(virConnectPtr conn, virDomainPtr dom)
{
	::virDomainInfo node_info;
	int ret = ::virDomainGetInfo(dom, &node_info);
	if (0 > ret)
	{
		::std::ostringstream oss;
		oss << "Failed to query the max memory for domain \"" << ::virDomainGetName(dom) << "\": " << last_error(conn);
		DCS_EXCEPTION_THROW(::std::runtime_error, oss.str());
	}

	return node_info.maxMem;
}

void max_memory(virConnectPtr conn, virDomainPtr dom, unsigned long mem)
{
	int ret = ::virDomainSetMaxMemory(dom, mem);
	if (0 > ret)
	{
		::std::ostringstream oss;
		oss << "Failed to set the max memory for domain \"" << ::virDomainGetName(dom) << "\": " << last_error(conn);
		DCS_EXCEPTION_THROW(::std::runtime_error, oss.str());
	}
}

}}}} // Namespace dcs::testbed::libvirt::detail

#endif // DCS_TESTBED_LIBVIRT_DETAIL_UTILITY_HPP
