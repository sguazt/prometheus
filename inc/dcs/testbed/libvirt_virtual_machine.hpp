/**
 * \file dcs/testbed/libvirt_virtual_machine.hpp
 *
 * \brief Manages VMs by means of libvirt toolkit.
 *
 * \author Marco Guazzone (marco.guazzone@gmail.com)
 *
 * <hr/>
 *
 * Copyright (C) 2012       Marco Guazzone
 *                          [Distributed Computing System (DCS) Group,
 *                           Computer Science Institute,
 *                           Department of Science and Technological Innovation,
 *                           University of Piemonte Orientale,
 *                           Alessandria (Italy)]
 *
 * This file is part of dcsxx-testbed.
 *
 * dcsxx-testbed is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * dcsxx-testbed is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with dcsxx-testbed.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef DCS_TESTBED_LIBVIRT_VIRTUAL_MACHINE_HPP
#define DCS_TESTBED_LIBVIRT_VIRTUAL_MACHINE_HPP


#include <dcs/assert.hpp>
#include <dcs/debug.hpp>
#include <dcs/logging.hpp>
#include <dcs/testbed/base_virtual_machine.hpp>
#include <dcs/testbed/detail/libvirt.hpp>
#include <iostream>
#include <libvirt/libvirt.h>
#include <sstream>
#include <stdexcept>
#include <string>


namespace dcs { namespace testbed {

template <typename RealT>
class libvirt_virtual_machine: public base_virtual_machine<RealT>
{
	private: typedef base_virtual_machine<RealT> base_type;
	public: typedef RealT real_type;


	public: libvirt_virtual_machine(::std::string const& vmm_uri, ::std::string const& name)
	: uri_(vmm_uri),
	  name_(name),
	  conn_(0)
	{
		init();
	}

	public: ~libvirt_virtual_machine()
	{
		// According to http://www.stlport.org/doc/exception_safety.html we avoid to throw any exception inside the destructor
		try
		{
			detail::libvirt::disconnect(conn_);
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

	private: ::std::string do_name() const
	{
		return name_;
	}

	private: int do_num_vcpus() const
	{
		// pre: conn_ not null
		DCS_ASSERT(0 != conn_,
				   DCS_EXCEPTION_THROW(::std::runtime_error,
									   "Not connected"));

		virDomainPtr dom = detail::libvirt::connect_domain(conn_, name_);

		int nvcpus = detail::libvirt::num_vcpus(conn_, dom, VIR_DOMAIN_AFFECT_CURRENT);

		detail::libvirt::disconnect_domain(conn_, dom);

		return nvcpus;
	}

	private: void do_cpu_share(real_type share)
	{
		// pre: conn_ not null
		DCS_ASSERT(0 != conn_,
				   DCS_EXCEPTION_THROW(::std::runtime_error,
									   "Not connected"));

		virDomainPtr dom = detail::libvirt::connect_domain(conn_, name_);

		int nvcpus(0);
		nvcpus = detail::libvirt::num_vcpus(conn_, dom, VIR_DOMAIN_AFFECT_CURRENT);

		//FIXME: This is a Xen-related stuff. What for other hypervisors?
		//FIXME: Actually we assume that weight is 256 (its default value)
		int cap(share < 1.0 ? share*nvcpus*100 : 0); //Note: cap == 0 ==> No upper cap
		detail::libvirt::sched_param<int>(conn_, dom, "cap", cap, VIR_DOMAIN_AFFECT_CURRENT);

		detail::libvirt::disconnect_domain(conn_, dom);
	}

	private: real_type do_cpu_share() const
	{
		// pre: conn_ not null
		DCS_ASSERT(0 != conn_,
				   DCS_EXCEPTION_THROW(::std::runtime_error,
									   "Not connected"));

		virDomainPtr dom = detail::libvirt::connect_domain(conn_, name_);

		int cap(0);
		cap = detail::libvirt::sched_param<int>(conn_, dom, "cap", VIR_DOMAIN_AFFECT_CURRENT);

		int nvcpus(0);
		nvcpus = detail::libvirt::num_vcpus(conn_, dom, VIR_DOMAIN_AFFECT_CURRENT);

		detail::libvirt::disconnect_domain(conn_, dom);

		//FIXME: This is a Xen-related stuff. What for other hypervisors?
		//FIXME: Actually we assume that weight is 256 (its default value)
		real_type share(cap/(nvcpus*100.0));

		return share > 0 ? share : 1; //Note: cap == 0 ==> No upper cap
	}

	private: void init()
	{
		// Connect to libvirtd daemon
		conn_ = detail::libvirt::connect(uri_);
	}


	private: ::std::string uri_;
	private: ::std::string name_;
	private: virConnectPtr conn_;
}; // libvirt_virtual_machine

}} // Namespace dcs::testbed

#endif // DCS_TESTBED_LIBVIRT_VIRTUAL_MACHINE_HPP
