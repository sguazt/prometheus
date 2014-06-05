/**
 * \file dcs/testbed/base_virtual_machine_manager.hpp
 *
 * \brief Base class for VM managers.
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

#ifndef DCS_TESTBED_BASE_VIRTUAL_MACHINE_MANAGER_HPP
#define DCS_TESTBED_BASE_VIRTUAL_MACHINE_MANAGER_HPP


#include <boost/shared_ptr.hpp>
#include <dcs/testbed/base_virtual_machine.hpp>
#include <string>


namespace dcs { namespace testbed {

template <typename TraitsT>
class base_virtual_machine_manager
{
	public: typedef TraitsT traits_type;
	public: typedef base_virtual_machine<traits_type> vm_type;
	public: typedef ::boost::shared_ptr<vm_type> vm_pointer;
	public: typedef ::std::string identifier_type;
	public: typedef typename vm_type::identifier_type vm_identifier_type;
	public: typedef typename traits_type::uint_type uint_type;


	public: base_virtual_machine_manager()
	{
	}

	public: virtual ~base_virtual_machine_manager()
	{
		// empty
	}

	public: identifier_type id() const
	{
		return do_id();
	}

	/// Get the VM associated to the given id
	public: vm_pointer vm(vm_identifier_type const& id)
	{
		return do_vm(id);
	}

	/// Get the VM associated to the given id
	public: vm_pointer vm(vm_identifier_type const& id) const
	{
		return do_vm(id);
	}

	public: bool alive() const
	{
		return do_alive();
	}

	public: uint_type max_supported_num_vcpus() const
	{
		return do_max_supported_num_vcpus();
	}

	private: virtual identifier_type do_id() const = 0;

	private: virtual vm_pointer do_vm(vm_identifier_type const& id) = 0;

	private: virtual vm_pointer do_vm(vm_identifier_type const& id) const = 0;

	private: virtual bool do_alive() const = 0;

	private: virtual uint_type do_max_supported_num_vcpus() const = 0;
}; // base_virtual_machine_manager

}} // Namespace dcs::testbed

#endif // DCS_TESTBED_BASE_VIRTUAL_MACHINE_MANAGER_HPP
