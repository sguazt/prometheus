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


/**
 * \brief Base class for VMs.
 *
 * \tparam TraitsT Traits type.
 *
 * \author Marco Guazzone (marco.guazzone@gmail.com)
 */
template <typename TraitsT>
class base_virtual_machine
{
	public: typedef TraitsT traits_type;
	public: typedef typename traits_type::real_type real_type;
	public: typedef typename traits_type::uint_type uint_type;
	public: typedef ::std::string identifier_type;
	public: typedef base_virtual_machine_manager<traits_type>* vmm_pointer;
	public: typedef ::boost::shared_ptr< base_sensor<traits_type> > sensor_pointer;


	protected: base_virtual_machine()
	{
		// Empty
	}

	public: virtual ~base_virtual_machine()
	{
		// Empty
	}

	/// Gets the VM name
	public: ::std::string name() const
	{
		return do_name();
	}

	/// Gets the VM identifier
	public: identifier_type id() const
	{
		return do_id();
	}

	/// Retuns a points to the VM manager that currently runs this VM
	public: vmm_pointer vmm() const
	{
		return do_vmm();
	}

	/// Retuns a points to the VM manager that currently runs this VM
	public: vmm_pointer vmm()
	{
		return do_vmm();
	}

	/// Gets the CPU cap
	public: real_type cpu_cap() const
	{
		return do_cpu_cap();
	}

	/// Sets the CPU cap
	public: void cpu_cap(real_type value)
	{
		do_cpu_cap(value);
	}

	/// Gets the CPU share
	public: real_type cpu_share() const
	{
		return do_cpu_share();
	}

	/// Sets the CPU share
	public: void cpu_share(real_type value)
	{
		do_cpu_share(value);
	}

	/// Gets the total number of virtual CPUs
	public: uint_type max_num_vcpus() const
	{
		return do_max_num_vcpus();
	}

	/// Sets the current number of virtual CPUs
	public: void num_vcpus(uint_type value)
	{
		do_num_vcpus(value);
	}

	/// Gets the current number of virtual CPUs
	public: uint_type num_vcpus() const
	{
		return do_num_vcpus();
	}

	/// Gets the memory cap
	public: real_type memory_cap() const
	{
		return do_memory_cap();
	}

	/// Sets the memory cap
	public: void memory_cap(real_type value)
	{
		do_memory_cap(value);
	}

	/// Gets the memory share
	public: real_type memory_share() const
	{
		return do_memory_share();
	}

	/// Sets the memory share
	public: void memory_share(real_type value)
	{
		do_memory_share(value);
	}

	/// Sets the maximum amount of memory (in kB) that can be allocated to this VM
	public: void max_memory(uint_type value)
	{
		do_max_memory(value);
	}

	/// Gets the maximum amount of memory (in kB) that can be allocated to this VM
	public: uint_type max_memory() const
	{
		return do_max_memory();
	}

	/// Sets the amount of memory (in kB) to allocate to this VM
	public: void memory(uint_type value)
	{
		do_memory(value);
	}

	/// Gets the amount of memory (in kB) to allocate this VM
	public: uint_type memory() const
	{
		return do_memory();
	}

	/// Sets the desired average incoming bit rate for the given network interface (specified either as device name of as MAC address) being shaped (in kilobytes/second)
	public: void network_average_inbound_bandwidth(const std::string& interface, uint_type value)
	{
		do_network_average_inbound_bandwidth(interface, value);
	}

	/// Gets the desired average incoming bit rate for the given network interface (specified either as device name of as MAC address) being shaped (in kilobytes/second)
	public: uint_type network_average_inbound_bandwidth(const std::string& interface)
	{
		return do_network_average_inbound_bandwidth(interface);
	}

	/// Starts this VM
	public: void start()
	{
		do_start();
	}

	public: bool running() const
	{
		return do_running();
	}

	/// Suspends this VM
	public: void suspend()
	{
		do_suspend();
	}

	/// Resumes this VM
	public: void resume()
	{
		do_resume();
	}

	/// Reboots this VM
	public: void reboot()
	{
		do_reboot();
	}

	/// Shuts down this VM
	public: void shutdown()
	{
		do_shutdown();
	}

	/// Power off this VM (unlike the shutdown method, this method emulates the power reset button thus abruptly powering off the VM)
	public: void poweroff()
	{
		do_poweroff();
	}

	public: void migrate(vmm_pointer p_dest_vmm)
	{
		do_migrate(p_dest_vmm);
	}

	/// Returns the sensor associated with the given category
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

	private: virtual void do_num_vcpus(uint_type value) = 0;

	private: virtual uint_type do_num_vcpus() const = 0;

	private: virtual real_type do_memory_cap() const = 0;

	private: virtual void do_memory_cap(real_type value) = 0;

	private: virtual real_type do_memory_share() const = 0;

	private: virtual void do_memory_share(real_type value) = 0;

	private: virtual void do_max_memory(uint_type value) = 0;

	private: virtual uint_type do_max_memory() const = 0;

	private: virtual void do_memory(uint_type value) = 0;

	private: virtual uint_type do_memory() const = 0;

	private: virtual void do_network_average_inbound_bandwidth(const std::string& interface, uint_type value) = 0;

	private: virtual uint_type do_network_average_inbound_bandwidth(const std::string& interface) const = 0;

	private: virtual void do_start() = 0;

	private: virtual bool do_running() const = 0;

	private: virtual void do_suspend() = 0;

	private: virtual void do_resume() = 0;

	private: virtual void do_reboot() = 0;

	private: virtual void do_shutdown() = 0;

	private: virtual void do_poweroff() = 0;

	private: virtual void do_migrate(vmm_pointer p_dest_vmm) = 0;

	private: virtual sensor_pointer do_sensor(virtual_machine_performance_category cat) const = 0;
}; // base_virtual_machine

}} // Namespace dcs::testbed

#endif // DCS_TESTBED_BASE_VIRTUAL_MACHINE_HPP
