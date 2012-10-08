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


#include <string>


namespace dcs { namespace testbed {

template <typename RealT>
class base_virtual_machine
{
	public: typedef RealT real_type;


	public: virtual ~base_virtual_machine()
	{
		// empty
	}

	/// Get the VM name
	public: ::std::string name() const
	{
		return do_name();
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

	private: virtual ::std::string do_name() const = 0;

	private: virtual real_type do_cpu_share() const = 0;

	private: virtual void do_cpu_share(real_type value) = 0;
}; // base_virtual_machine

}} // Namespace dcs::testbed

#endif // DCS_TESTBED_BASE_VIRTUAL_MACHINE_HPP
