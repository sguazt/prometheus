/**
 * \file dcs/testbed/libvirt/virtual_machine.hpp
 *
 * \brief Manages VMs by means of libvirt toolkit.
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

#ifndef DCS_TESTBED_LIBVIRT_VIRTUAL_MACHINE_HPP
#define DCS_TESTBED_LIBVIRT_VIRTUAL_MACHINE_HPP


#include <algorithm>
#include <dcs/assert.hpp>
#include <dcs/debug.hpp>
#include <dcs/logging.hpp>
#include <dcs/testbed/base_virtual_machine.hpp>
#include <dcs/testbed/base_sensor.hpp>
#include <dcs/testbed/libvirt/detail/utility.hpp>
#include <dcs/testbed/libvirt/sensors.hpp>
#include <dcs/testbed/virtual_machine_performance_category.hpp>
#include <dcs/uri.hpp>
#include <iostream>
#include <libvirt/libvirt.h>
#include <sstream>
#include <stdexcept>
#include <string>


namespace dcs { namespace testbed { namespace libvirt {

// Forward declarations
template <typename TraitsT>
class virtual_machine_manager;


/**
 * \brief Manages VMs by means of libvirt toolkit.
 *
 * \tparam TraitsT Traits type.
 *
 * \author Marco Guazzone (marco.guazzone@gmail.com)
 */
template <typename TraitsT>
class virtual_machine: public base_virtual_machine<TraitsT>
{
	private: typedef base_virtual_machine<TraitsT> base_type;
	public: typedef typename base_type::traits_type traits_type;
	public: typedef typename base_type::real_type real_type;
	public: typedef typename base_type::uint_type uint_type;
	public: typedef typename base_type::identifier_type identifier_type;
	public: typedef typename base_type::vmm_pointer vmm_pointer;
	public: typedef virtual_machine_manager<traits_type>* vmm_impl_pointer;
	public: typedef base_sensor<traits_type> sensor_type;
	public: typedef ::boost::shared_ptr<sensor_type> sensor_pointer;


	public: virtual_machine(::std::string const& uri)
	: name_(detail::vm_name(uri)),
	  p_vmm_(0),
	  p_dom_(0)
	{
	}

	public: virtual_machine(vmm_impl_pointer p_vmm, ::std::string const& name)
	: name_(detail::vm_name(name)),
	  p_vmm_(p_vmm),
	  p_dom_(0)
	{
		init();
	}

	public: ~virtual_machine()
	{
		// According to http://www.stlport.org/doc/exception_safety.html we avoid to throw any exception inside the destructor
		try
		{
			detail::disconnect_domain(p_vmm_->connection(), p_dom_);
		}
		catch (::std::exception const& e)
		{
			::std::ostringstream oss;
			oss << "Failed to disconnect from hypervisor '" << p_vmm_->id() << "': " << e.what();
			dcs::log_error(DCS_LOGGING_AT, oss.str());
		}
		catch (...)
		{
			::std::ostringstream oss;
			oss << "Failed to disconnect from hypervisor '" << p_vmm_->id() << "': Unknown error";
			dcs::log_error(DCS_LOGGING_AT, oss.str());
		}
	}

	public: unsigned long raw_id() const
	{
		// pre: p_vmm_ != null
		DCS_ASSERT(p_vmm_,
				   DCS_EXCEPTION_THROW(::std::logic_error,
									   "Not connected to VMM"));
		// pre: p_dom_ != null
		DCS_ASSERT(p_dom_,
				   DCS_EXCEPTION_THROW(::std::logic_error,
									   "Not attached to a domain"));

		return detail::domain_id(p_vmm_->connection(), p_dom_);
	}

	public: ::virDomainPtr domain() const
	{
		// pre: p_vmm_ != null
		DCS_ASSERT(p_vmm_,
				   DCS_EXCEPTION_THROW(::std::logic_error,
									   "Not connected to VMM"));
		// pre: p_dom_ != null
		DCS_ASSERT(p_dom_,
				   DCS_EXCEPTION_THROW(::std::logic_error,
									   "Not attached to a domain"));

		return p_dom_;
	}

	public: ::virDomainPtr domain()
	{
		return p_dom_;
	}

	private: void init()
	{
		// pre: p_vmm_ != null
		DCS_ASSERT(p_vmm_,
				   DCS_EXCEPTION_THROW(::std::logic_error,
									   "Not connected to VMM"));

		// Connect to libvirtd daemon
		p_dom_ = detail::connect_domain(p_vmm_->connection(), name_);
	}

	private: ::std::string do_name() const
	{
		return name_;
	}

	private: identifier_type do_id() const
	{
		if (id_.empty())
		{
			::std::ostringstream oss;
			if (p_vmm_)
			{
				oss << p_vmm_->id();
			}
			else
			{
				oss << "<None>";
			}
			oss << ":" << this->name();
			id_ = oss.str();
		}

		return id_;
	}

	private: vmm_pointer do_vmm()
	{
		return p_vmm_;
	}

	private: vmm_pointer do_vmm() const
	{
		return p_vmm_;
	}

	private: uint_type do_max_num_vcpus() const
	{
		// pre: p_vmm_ != null
		DCS_ASSERT(p_vmm_,
				   DCS_EXCEPTION_THROW(::std::logic_error,
									   "Not connected to VMM"));
		// pre: p_dom_ != null
		DCS_ASSERT(p_dom_,
				   DCS_EXCEPTION_THROW(::std::logic_error,
									   "Not attached to a domain"));

		int nvcpus = detail::num_vcpus(p_vmm_->connection(), p_dom_, VIR_DOMAIN_VCPU_MAXIMUM);

		return static_cast<uint_type>(nvcpus);
	}

	private: void do_num_vcpus(uint_type value)
	{
		// pre: p_vmm_ != null
		DCS_ASSERT(p_vmm_,
				   DCS_EXCEPTION_THROW(::std::logic_error,
									   "Not connected to VMM"));
		// pre: p_dom_ != null
		DCS_ASSERT(p_dom_,
				   DCS_EXCEPTION_THROW(::std::logic_error,
									   "Not attached to a domain"));

		detail::num_vcpus(p_vmm_->connection(), p_dom_, value, VIR_DOMAIN_AFFECT_CURRENT);
	}

	private: uint_type do_num_vcpus() const
	{
		// pre: p_vmm_ != null
		DCS_ASSERT(p_vmm_,
				   DCS_EXCEPTION_THROW(::std::logic_error,
									   "Not connected to VMM"));
		// pre: p_dom_ != null
		DCS_ASSERT(p_dom_,
				   DCS_EXCEPTION_THROW(::std::logic_error,
									   "Not attached to a domain"));

		int nvcpus = detail::num_vcpus(p_vmm_->connection(), p_dom_, VIR_DOMAIN_AFFECT_CURRENT);

		return static_cast<uint_type>(nvcpus);
	}

	private: void do_cpu_cap(real_type cap)
	{
		// pre: p_vmm_ != null
		DCS_ASSERT(p_vmm_,
				   DCS_EXCEPTION_THROW(::std::logic_error,
									   "Not connected to VMM"));
		// pre: p_dom_ != null
		DCS_ASSERT(p_dom_,
				   DCS_EXCEPTION_THROW(::std::logic_error,
									   "Not attached to a domain"));

/*
		const int nvcpus(this->max_num_vcpus());

		//FIXME: This is a Xen-related stuff. What for other hypervisors?
		//FIXME: Actually we assume that weight is 256 (its default value)
		int kap = cap;
		if (kap > (nvcpus*100))
		{
			kap = 0; //Note: cap == 0 ==> No upper cap
		}
		detail::sched_param<int>(p_vmm_->connection(), p_dom_, "cap", kap, VIR_DOMAIN_AFFECT_CURRENT);
*/
		detail::cpu_cap(p_vmm_->connection(), p_dom_, cap);
	}

	private: real_type do_cpu_cap() const
	{
		// pre: p_vmm_ != null
		DCS_ASSERT(p_vmm_,
				   DCS_EXCEPTION_THROW(::std::logic_error,
									   "Not connected to VMM"));
		// pre: p_dom_ != null
		DCS_ASSERT(p_dom_,
				   DCS_EXCEPTION_THROW(::std::logic_error,
									   "Not attached to a domain"));

/*
		const int nvcpus = this->max_num_vcpus();
		int kap = detail::sched_param<int>(p_vmm_->connection(), p_dom_, "cap", VIR_DOMAIN_AFFECT_CURRENT);
		if (kap == 0)
		{
			kap = nvcpus*100;
		}

		return kap;
*/
		return detail::cpu_cap(p_vmm_->connection(), p_dom_);
	}

	private: void do_cpu_share(real_type share)
	{
		// pre: p_vmm_ != null
		DCS_ASSERT(p_vmm_,
				   DCS_EXCEPTION_THROW(::std::logic_error,
									   "Not connected to VMM"));
		// pre: p_dom_ != null
		DCS_ASSERT(p_dom_,
				   DCS_EXCEPTION_THROW(::std::logic_error,
									   "Not attached to a domain"));

/*
		const int nvcpus = this->max_num_vcpus();

		//FIXME: This is a Xen-related stuff. What for other hypervisors?
		//FIXME: Actually we assume that weight is 256 (its default value)
		const int cap = share < 1.0 ? share*nvcpus*100 : 0; //Note: cap == 0 ==> No upper cap
		detail::sched_param<int>(p_vmm_->connection(), p_dom_, "cap", cap, VIR_DOMAIN_AFFECT_CURRENT);
*/
		detail::cpu_share(p_vmm_->connection(), p_dom_, share);
	}

	private: real_type do_cpu_share() const
	{
		// pre: p_vmm_ != null
		DCS_ASSERT(p_vmm_,
				   DCS_EXCEPTION_THROW(::std::logic_error,
									   "Not connected to VMM"));
		// pre: p_dom_ != null
		DCS_ASSERT(p_dom_,
				   DCS_EXCEPTION_THROW(::std::logic_error,
									   "Not attached to a domain"));

/*
		const int cap = detail::sched_param<int>(p_vmm_->connection(), p_dom_, "cap", VIR_DOMAIN_AFFECT_CURRENT);

		const int nvcpus = this->max_num_vcpus();

		//FIXME: This is a Xen-related stuff. What for other hypervisors?
		//FIXME: Actually we assume that weight is 256 (its default value)
		const real_type share = cap/(nvcpus*100.0);

		return share > 0 ? share : 1; //Note: cap == 0 ==> No upper cap
*/
		return detail::cpu_share(p_vmm_->connection(), p_dom_);
	}

	private: void do_max_memory(uint_type value)
	{
		// pre: p_vmm_ != null
		DCS_ASSERT(p_vmm_,
				   DCS_EXCEPTION_THROW(::std::logic_error,
									   "Not connected to VMM"));
		// pre: p_dom_ != null
		DCS_ASSERT(p_dom_,
				   DCS_EXCEPTION_THROW(::std::logic_error,
									   "Not attached to a domain"));

		detail::max_memory(p_vmm_->connection(), p_dom_, value);
	}

	private: uint_type do_max_memory() const
	{
		// pre: p_vmm_ != null
		DCS_ASSERT(p_vmm_,
				   DCS_EXCEPTION_THROW(::std::logic_error,
									   "Not connected to VMM"));
		// pre: p_dom_ != null
		DCS_ASSERT(p_dom_,
				   DCS_EXCEPTION_THROW(::std::logic_error,
									   "Not attached to a domain"));

		const unsigned long max_mem = detail::max_memory(p_vmm_->connection(), p_dom_);

		return static_cast<uint_type>(max_mem);
	}

	private: void do_memory(uint_type value)
	{
		// pre: p_vmm_ != null
		DCS_ASSERT(p_vmm_,
				   DCS_EXCEPTION_THROW(::std::logic_error,
									   "Not connected to VMM"));
		// pre: p_dom_ != null
		DCS_ASSERT(p_dom_,
				   DCS_EXCEPTION_THROW(::std::logic_error,
									   "Not attached to a domain"));

		detail::current_memory(p_vmm_->connection(), p_dom_, value);
	}

	private: uint_type do_memory() const
	{
		// pre: p_vmm_ != null
		DCS_ASSERT(p_vmm_,
				   DCS_EXCEPTION_THROW(::std::logic_error,
									   "Not connected to VMM"));
		// pre: p_dom_ != null
		DCS_ASSERT(p_dom_,
				   DCS_EXCEPTION_THROW(::std::logic_error,
									   "Not attached to a domain"));

		const unsigned long cur_mem = detail::current_memory(p_vmm_->connection(), p_dom_);

		return static_cast<uint_type>(cur_mem);
	}

	private: void do_memory_cap(real_type cap)
	{
		// pre: p_vmm_ != null
		DCS_ASSERT(p_vmm_,
				   DCS_EXCEPTION_THROW(::std::logic_error,
									   "Not connected to VMM"));
		// pre: p_dom_ != null
		DCS_ASSERT(p_dom_,
				   DCS_EXCEPTION_THROW(::std::logic_error,
									   "Not attached to a domain"));

/*
		const unsigned long max_mem = detail::config_max_memory(p_vmm_->connection(), p_dom_);
		const unsigned long mem = std::min(static_cast<unsigned long>(cap), max_mem);

#if 0
		detail::max_memory(p_vmm_->connection(), p_dom_, mem);
#endif
		detail::current_memory(p_vmm_->connection(), p_dom_, mem);
*/
		detail::memory_cap(p_vmm_->connection(), p_dom_, cap);
	}

	private: real_type do_memory_cap() const
	{
		// pre: p_vmm_ != null
		DCS_ASSERT(p_vmm_,
				   DCS_EXCEPTION_THROW(::std::logic_error,
									   "Not connected to VMM"));
		// pre: p_dom_ != null
		DCS_ASSERT(p_dom_,
				   DCS_EXCEPTION_THROW(::std::logic_error,
									   "Not attached to a domain"));

/*
		//return static_cast<real_type>(detail::current_memory(p_vmm_->connection(), p_dom_));
#if 0
		return static_cast<real_type>(detail::max_memory(p_vmm_->connection(), p_dom_));
#else
		return static_cast<real_type>(detail::current_memory(p_vmm_->connection(), p_dom_));
#endif
*/
		return detail::memory_cap(p_vmm_->connection(), p_dom_);
	}

	private: void do_memory_share(real_type share)
	{
		// pre: p_vmm_ != null
		DCS_ASSERT(p_vmm_,
				   DCS_EXCEPTION_THROW(::std::logic_error,
									   "Not connected to VMM"));
		// pre: p_dom_ != null
		DCS_ASSERT(p_dom_,
				   DCS_EXCEPTION_THROW(::std::logic_error,
									   "Not attached to a domain"));

/*
		const unsigned long max_mem = detail::config_max_memory(p_vmm_->connection(), p_dom_);
		const unsigned long mem = std::min(static_cast<unsigned long>(share*max_mem), max_mem);

DCS_DEBUG_TRACE("Setting Memory: " << mem << " (share: " << share << ")");//XXX
#if 0
		detail::max_memory(p_vmm_->connection(), p_dom_, mem);
#endif
		detail::current_memory(p_vmm_->connection(), p_dom_, mem);
*/
		detail::memory_share(p_vmm_->connection(), p_dom_, share);
	}

	private: real_type do_memory_share() const
	{
		// pre: p_vmm_ != null
		DCS_ASSERT(p_vmm_,
				   DCS_EXCEPTION_THROW(::std::logic_error,
									   "Not connected to VMM"));
		// pre: p_dom_ != null
		DCS_ASSERT(p_dom_,
				   DCS_EXCEPTION_THROW(::std::logic_error,
									   "Not attached to a domain"));

/*
//		const unsigned long max_mem = detail::config_max_memory(p_vmm_->connection(), p_dom_);
//		const unsigned long cur_mem = detail::current_memory(p_vmm_->connection(), p_dom_);
//
//DCS_DEBUG_TRACE("Getting Memory: " << cur_mem << " (share: " << (cur_mem/static_cast<real_type>(max_mem)) << ")");//XXX
//		return cur_mem/static_cast<real_type>(max_mem);
		const unsigned long cfg_max_mem = detail::config_max_memory(p_vmm_->connection(), p_dom_);
#if 0
		const unsigned long cur_max_mem = detail::max_memory(p_vmm_->connection(), p_dom_);

DCS_DEBUG_TRACE("Getting Memory: " << cur_max_mem << " (share: " << (cur_max_mem/static_cast<real_type>(cfg_max_mem)) << ")");//XXX
		return cur_max_mem/static_cast<real_type>(cfg_max_mem);
#else
		const unsigned long cur_mem = detail::current_memory(p_vmm_->connection(), p_dom_);

DCS_DEBUG_TRACE("Getting Memory: " << cur_mem << " (share: " << (cur_mem/static_cast<real_type>(cfg_max_mem)) << ")");//XXX
		return cur_mem/static_cast<real_type>(cfg_max_mem);
#endif
*/
		return detail::memory_share(p_vmm_->connection(), p_dom_);
	}

	private: void do_network_average_inbound_bandwidth(const std::string& interface, uint_type value)
	{
		// pre: p_vmm_ != null
		DCS_ASSERT(p_vmm_,
				   DCS_EXCEPTION_THROW(::std::logic_error,
									   "Not connected to VMM"));
		// pre: p_dom_ != null
		DCS_ASSERT(p_dom_,
				   DCS_EXCEPTION_THROW(::std::logic_error,
									   "Not attached to a domain"));

		detail::domain_network_bandwidth(p_vmm_->connection(), p_dom_, interface.c_str(), VIR_DOMAIN_BANDWIDTH_IN_AVERAGE, value, VIR_DOMAIN_AFFECT_CURRENT);
	}

	private: uint_type do_network_average_inbound_bandwidth(const std::string& interface) const
	{
		// pre: p_vmm_ != null
		DCS_ASSERT(p_vmm_,
				   DCS_EXCEPTION_THROW(::std::logic_error,
									   "Not connected to VMM"));
		// pre: p_dom_ != null
		DCS_ASSERT(p_dom_,
				   DCS_EXCEPTION_THROW(::std::logic_error,
									   "Not attached to a domain"));

		return detail::domain_network_bandwidth(p_vmm_->connection(), p_dom_, interface.c_str(), VIR_DOMAIN_BANDWIDTH_IN_AVERAGE, VIR_DOMAIN_AFFECT_CURRENT);
	}

	private: void do_network_average_outbound_bandwidth(const std::string& interface, uint_type value)
	{
		// pre: p_vmm_ != null
		DCS_ASSERT(p_vmm_,
				   DCS_EXCEPTION_THROW(::std::logic_error,
									   "Not connected to VMM"));
		// pre: p_dom_ != null
		DCS_ASSERT(p_dom_,
				   DCS_EXCEPTION_THROW(::std::logic_error,
									   "Not attached to a domain"));

		detail::domain_network_bandwidth(p_vmm_->connection(), p_dom_, interface.c_str(), VIR_DOMAIN_BANDWIDTH_OUT_AVERAGE, value, VIR_DOMAIN_AFFECT_CURRENT);
	}

	private: uint_type do_network_average_outbound_bandwidth(const std::string& interface) const
	{
		// pre: p_vmm_ != null
		DCS_ASSERT(p_vmm_,
				   DCS_EXCEPTION_THROW(::std::logic_error,
									   "Not connected to VMM"));
		// pre: p_dom_ != null
		DCS_ASSERT(p_dom_,
				   DCS_EXCEPTION_THROW(::std::logic_error,
									   "Not attached to a domain"));

		return detail::domain_network_bandwidth(p_vmm_->connection(), p_dom_, interface.c_str(), VIR_DOMAIN_BANDWIDTH_OUT_AVERAGE, VIR_DOMAIN_AFFECT_CURRENT);
	}

	private: void do_start()
	{
		// pre: p_vmm_ != null
		DCS_ASSERT(p_vmm_,
				   DCS_EXCEPTION_THROW(::std::logic_error,
									   "Not connected to VMM"));
		// pre: p_dom_ != null
		DCS_ASSERT(p_dom_,
				   DCS_EXCEPTION_THROW(::std::logic_error,
									   "Not attached to a domain"));

		detail::domain_create(p_vmm_->connection(), p_dom_);
	}

	private: bool do_running() const
	{
		// pre: p_vmm_ != null
		DCS_ASSERT(p_vmm_,
				   DCS_EXCEPTION_THROW(::std::logic_error,
									   "Not connected to VMM"));
		// pre: p_dom_ != null
		DCS_ASSERT(p_dom_,
				   DCS_EXCEPTION_THROW(::std::logic_error,
									   "Not attached to a domain"));

		return detail::domain_is_active(p_vmm_->connection(), p_dom_);
	}

	private: void do_suspend()
	{
		// pre: p_vmm_ != null
		DCS_ASSERT(p_vmm_,
				   DCS_EXCEPTION_THROW(::std::logic_error,
									   "Not connected to VMM"));
		// pre: p_dom_ != null
		DCS_ASSERT(p_dom_,
				   DCS_EXCEPTION_THROW(::std::logic_error,
									   "Not attached to a domain"));

		detail::domain_suspend(p_vmm_->connection(), p_dom_);
	}

	private: void do_resume()
	{
		// pre: p_vmm_ != null
		DCS_ASSERT(p_vmm_,
				   DCS_EXCEPTION_THROW(::std::logic_error,
									   "Not connected to VMM"));
		// pre: p_dom_ != null
		DCS_ASSERT(p_dom_,
				   DCS_EXCEPTION_THROW(::std::logic_error,
									   "Not attached to a domain"));

		detail::domain_resume(p_vmm_->connection(), p_dom_);
	}

	private: void do_reboot()
	{
		// pre: p_vmm_ != null
		DCS_ASSERT(p_vmm_,
				   DCS_EXCEPTION_THROW(::std::logic_error,
									   "Not connected to VMM"));
		// pre: p_dom_ != null
		DCS_ASSERT(p_dom_,
				   DCS_EXCEPTION_THROW(::std::logic_error,
									   "Not attached to a domain"));

		detail::domain_reboot(p_vmm_->connection(), p_dom_);
	}

	private: void do_shutdown()
	{
		// pre: p_vmm_ != null
		DCS_ASSERT(p_vmm_,
				   DCS_EXCEPTION_THROW(::std::logic_error,
									   "Not connected to VMM"));
		// pre: p_dom_ != null
		DCS_ASSERT(p_dom_,
				   DCS_EXCEPTION_THROW(::std::logic_error,
									   "Not attached to a domain"));

		detail::domain_shutdown(p_vmm_->connection(), p_dom_);
	}

	private: void do_poweroff()
	{
		// pre: p_vmm_ != null
		DCS_ASSERT(p_vmm_,
				   DCS_EXCEPTION_THROW(::std::logic_error,
									   "Not connected to VMM"));
		// pre: p_dom_ != null
		DCS_ASSERT(p_dom_,
				   DCS_EXCEPTION_THROW(::std::logic_error,
									   "Not attached to a domain"));

		detail::domain_reset(p_vmm_->connection(), p_dom_);
	}

	private: void do_migrate(vmm_pointer p_dest_vmm)
	{
		// pre: p_vmm_ != null
		DCS_ASSERT(p_vmm_,
				   DCS_EXCEPTION_THROW(::std::logic_error,
									   "Not connected to VMM"));
		// pre: p_dom_ != null
		DCS_ASSERT(p_dom_,
				   DCS_EXCEPTION_THROW(::std::logic_error,
									   "Not attached to a domain"));
		// pre: p_dest_vmm != null
		DCS_ASSERT(p_dest_vmm,
				   DCS_EXCEPTION_THROW(::std::logic_error,
									   "Destination VMM is not valid"));

		vmm_impl_pointer p_impl_dest_vmm = dynamic_cast<vmm_impl_pointer>(p_dest_vmm);

		// pre: p_dest_vmm != null
		DCS_ASSERT(p_impl_dest_vmm,
				   DCS_EXCEPTION_THROW(::std::logic_error,
									   "Incompatible destination VMM"));
		// pre: p_vmm_ != null
		DCS_ASSERT(p_impl_dest_vmm->connection(),
				   DCS_EXCEPTION_THROW(::std::logic_error,
									   "Not connected to destination VMM"));

		::virDomainPtr p_dest_dom = detail::domain_migrate(p_vmm_->connection(), p_dom_, p_impl_dest_vmm->connection());

		DCS_ASSERT(p_dest_dom != 0,
				   DCS_EXCEPTION_THROW(::std::runtime_error,
									   "Unable to migrate the VM"));

		detail::disconnect_domain(p_vmm_->connection(), p_dom_);
		p_dom_ = p_dest_dom;
		p_vmm_ = p_impl_dest_vmm;
	}

	private: sensor_pointer do_sensor(virtual_machine_performance_category cat) const
	{
		switch (cat)
		{
			case cpu_util_virtual_machine_performance:
				return ::boost::make_shared< cpu_utilization_sensor<traits_type> >(p_vmm_->connection(), p_dom_);
			case memory_util_virtual_machine_performance:
//#if DCS_TESTBED_SENSOR_HAVE_MEMINFO_SERVER
//				return ::boost::make_shared< memory_utilization_sensor<traits_type> >(p_vmm_->connection(), p_dom_, true);
//#else // DCS_TESTBED_SENSOR_HAVE_MEMINFO_SERVER
//				return ::boost::make_shared< memory_utilization_sensor<traits_type> >(p_vmm_->connection(), p_dom_, false);
//#endif // DCS_TESTBED_SENSOR_HAVE_MEMINFO_SERVER
				return ::boost::make_shared< memory_utilization_sensor<traits_type> >(p_vmm_->connection(), p_dom_, true);
		}

		DCS_EXCEPTION_THROW(::std::runtime_error, "Sensor not available");
	}


	private: ::std::string name_;
	private: mutable ::std::string id_;
	private: vmm_impl_pointer p_vmm_;
	private: ::virDomainPtr p_dom_;
}; // virtual_machine

}}} // Namespace dcs::testbed::libvirt

#endif // DCS_TESTBED_LIBVIRT_VIRTUAL_MACHINE_HPP
