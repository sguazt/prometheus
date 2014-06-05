/**
 * \file dcs/testbed/libvirt/virtual_machine_manager.hpp
 *
 * \brief Libvirt-based Virtual Machine Manager.
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

#ifndef DCS_TESTBED_LIBVIRT_VIRTUAL_MACHINE_MANAGER_HPP
#define DCS_TESTBED_LIBVIRT_VIRTUAL_MACHINE_MANAGER_HPP


#include <dcs/assert.hpp>
#include <dcs/debug.hpp>
#include <dcs/exception.hpp>
#include <dcs/logging.hpp>
#include <dcs/testbed/base_virtual_machine_manager.hpp>
#include <dcs/testbed/libvirt/detail/utility.hpp>
#include <dcs/testbed/libvirt/virtual_machine.hpp>
#include <dcs/uri.hpp>
#include <exception>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>


namespace dcs { namespace testbed { namespace libvirt {

/*
inline
::std::string strip_vmm_uri(::std::string const& uri)
{
	::std::ostringstream oss;

	::dcs::uri u(uri);
	if (!u.relative())
	{
		oss << u.scheme() << "://" << u.authority() << "/";
	}

	return oss.str();
}
*/

template <typename TraitsT>
class virtual_machine_manager: public base_virtual_machine_manager<TraitsT>
{
	private: typedef base_virtual_machine_manager<TraitsT> base_type;
	public: typedef typename base_type::traits_type traits_type;
	public: typedef typename base_type::identifier_type identifier_type;
	public: typedef typename base_type::vm_identifier_type vm_identifier_type;
	public: typedef typename base_type::vm_pointer vm_pointer;
	public: typedef typename base_type::uint_type uint_type;
	private: typedef ::std::map<vm_identifier_type,vm_pointer> vm_container;


	public: virtual_machine_manager(::std::string const& uri)
	: uri_(detail::vmm_uri(uri)),
	  p_conn_(0)
	{
		init();
	}

	public: ~virtual_machine_manager()
	{
		// According to http://www.stlport.org/doc/exception_safety.html we avoid to throw any exception inside the destructor
		try
		{
			if (p_conn_)
			{
				detail::disconnect(p_conn_);
			}
		}
		catch (::std::exception const& e)
		{
			::std::ostringstream oss;
			oss << "Failed to disconnect from hypervisor '" << uri_ << "': " << e.what();
			dcs::log_error(DCS_LOGGING_AT, oss.str());
		}
		catch (...)
		{
			::std::ostringstream oss;
			oss << "Failed to disconnect from hypervisor '" << uri_ << "': Unknown error";
			dcs::log_error(DCS_LOGGING_AT, oss.str());
		}
	}

	public: ::std::string uri() const
	{
		return uri_;
	}

	public: ::virConnectPtr connection()
	{
		return p_conn_;
	}

	public: ::virConnectPtr connection() const
	{
		return p_conn_;
	}

	private: void init()
	{
		// Connect to libvirtd daemon
		p_conn_ = detail::connect(uri_);
	}

	private: identifier_type do_id() const
	{
		return uri_;
	}

	private: bool do_alive() const
	{
		int ret = ::virConnectIsAlive(p_conn_);
		if (-1 == ret)
		{
			::std::ostringstream oss;
			oss << "Failed to check for VMM aliveness " << detail::last_error(p_conn_);
			DCS_EXCEPTION_THROW(::std::runtime_error, oss.str());
		}

		return ret ? true : false;
	}

	private: vm_pointer do_vm(vm_identifier_type const& id)
	{
		if (vm_map_.count(id) == 0)
		{
			vm_map_[id] = ::boost::make_shared< virtual_machine<traits_type> >(this, id);

			// check: vm_map.at(id) != null
			DCS_DEBUG_ASSERT( vm_map_.at(id) );
		}

		return vm_map_.at(id);
	}

	private: vm_pointer do_vm(vm_identifier_type const& id) const
	{
		DCS_ASSERT(vm_map_.count(id) > 0,
				   DCS_EXCEPTION_THROW(::std::invalid_argument,
									   "VM not found"));

		return vm_map_.at(id);
	}

	private: uint_type do_max_supported_num_vcpus() const
	{
		DCS_ASSERT(p_conn_,
				   DCS_EXCEPTION_THROW(::std::logic_error,
									   "Not connected"));

		int max_nvcpus = detail::max_supported_num_vcpus(p_conn_);

		return static_cast<uint_type>(max_nvcpus);
	}


	private: ::std::string uri_;
	private: ::virConnectPtr p_conn_;
	private: vm_container vm_map_;
}; // virtual_machine_manager

}}} // Namespace dcs::testbed::libvirt

#endif // DCS_TESTBED_LIBVIRT_VIRTUAL_MACHINE_MANAGER_HPP
