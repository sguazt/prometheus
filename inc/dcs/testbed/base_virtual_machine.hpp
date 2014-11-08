/**
 * \file dcs/testbed/base_virtual_machine.hpp
 *
 * \brief Base class for VMs.
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

	/// Get the memory cap
	public: real_type memory_cap() const
	{
		return do_memory_cap();
	}

	/// Set the memory cap
	public: void memory_cap(real_type value)
	{
		do_memory_cap(value);
	}

	/// Get the memory share
	public: real_type memory_share() const
	{
		return do_memory_share();
	}

	/// Set the memory share
	public: void memory_share(real_type value)
	{
		do_memory_share(value);
	}

	// Get the maximum amount of memory (in kB) that can be allocated to this VM
	public: uint_type max_memory() const
	{
		return do_max_memory();
	}

	// Get the amount of memory (in kB) that can be used by this VM
	public: uint_type memory() const
	{
		return do_memory();
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

	private: virtual real_type do_memory_cap() const = 0;

	private: virtual void do_memory_cap(real_type value) = 0;

	private: virtual real_type do_memory_share() const = 0;

	private: virtual void do_memory_share(real_type value) = 0;

	private: virtual uint_type do_max_memory() const = 0;

	private: virtual uint_type do_memory() const = 0;

	public: virtual sensor_pointer do_sensor(virtual_machine_performance_category cat) const = 0;
}; // base_virtual_machine

}} // Namespace dcs::testbed

#endif // DCS_TESTBED_BASE_VIRTUAL_MACHINE_HPP
