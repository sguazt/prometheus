/**
 * \file dcs/testbed/base_virtual_machine.hpp
 *
 * \brief Base class for VMs.
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

#ifndef DCS_TESTBED_BASE_VIRTUAL_MACHINE_HPP
#define DCS_TESTBED_BASE_VIRTUAL_MACHINE_HPP


#include <boost/smart_ptr.hpp>
#include <dcs/testbed/base_sensor.hpp>
#include <dcs/testbed/virtual_machine_performance_category.hpp>
#include <string>


namespace dcs { namespace testbed {

// Forward declarations
template <typename TraitsT>
class base_virtual_machine_manager;


template <typename TraitsT>
class base_virtual_machine
{
	public: typedef TraitsT traits_type;
	public: typedef typename traits_type::real_type real_type;
	public: typedef typename traits_type::uint_type uint_type;
	public: typedef ::std::string identifier_type;
	public: typedef base_virtual_machine_manager<traits_type>* vmm_pointer;
	public: typedef ::boost::shared_ptr< base_sensor<traits_type> > sensor_pointer;


	public: virtual ~base_virtual_machine()
	{
		// empty
	}

	/// Get the VM name
	public: ::std::string name() const
	{
		return do_name();
	}

	/// Get the VM identifier
	public: identifier_type id() const
	{
		return do_id();
	}

	public: vmm_pointer vmm() const
	{
		return do_vmm();
	}

	public: vmm_pointer vmm()
	{
		return do_vmm();
	}

	/// Get the CPU cap
	public: real_type cpu_cap() const
	{
		return do_cpu_cap();
	}

	/// Set the CPU cap
	public: void cpu_cap(real_type value)
	{
		do_cpu_cap(value);
	}

	/// Get the CPU share
	public: real_type cpu_share() const
	{
		return do_cpu_share();
	}

	/// Set the CPU share
	public: void cpu_share(real_type value)
	{
		do_cpu_share(value);
	}

	// Get the total number of virtual CPUs
	public: uint_type max_num_vcpus() const
	{
		return do_max_num_vcpus();
	}

	// Get the current number of virtual CPUs
	public: uint_type num_vcpus() const
	{
		return do_num_vcpus();
	}

	public: sensor_pointer sensor(virtual_machine_performance_category cat) const
	{
		return do_sensor(cat);
	}

	private: virtual ::std::string do_name() const = 0;

	private: virtual identifier_type do_id() const = 0;

	private: virtual vmm_pointer do_vmm() const = 0;

	private: virtual vmm_pointer do_vmm() = 0;

	private: virtual real_type do_cpu_cap() const = 0;

	private: virtual void do_cpu_cap(real_type value) = 0;

	private: virtual real_type do_cpu_share() const = 0;

	private: virtual void do_cpu_share(real_type value) = 0;

	private: virtual uint_type do_max_num_vcpus() const = 0;

	private: virtual uint_type do_num_vcpus() const = 0;

	public: virtual sensor_pointer do_sensor(virtual_machine_performance_category cat) const = 0;
}; // base_virtual_machine

}} // Namespace dcs::testbed

#endif // DCS_TESTBED_BASE_VIRTUAL_MACHINE_HPP
