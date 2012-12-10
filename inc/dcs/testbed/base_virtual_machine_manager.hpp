/**
 * \file dcs/testbed/base_virtual_machine_manager.hpp
 *
 * \brief Base class for VM managers.
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

	private: virtual identifier_type do_id() const = 0;

	private: virtual vm_pointer do_vm(vm_identifier_type const& id) = 0;

	private: virtual vm_pointer do_vm(vm_identifier_type const& id) const = 0;

	private: virtual bool do_alive() const = 0;
}; // base_virtual_machine_manager

}} // Namespace dcs::testbed

#endif // DCS_TESTBED_BASE_VIRTUAL_MACHINE_MANAGER_HPP
