/**
 * \file dcs/testbed/base_system_manager.hpp
 *
 * \brief Base class for system managers.
 *
 * \author Marco Guazzone (marco.guazzone@gmail.com)
 *
 * <hr/>
 *
 * Copyright (C) 2012       Marco Guazzone (marco.guazzone@gmail.com)
 *                          [Distributed Computing System (DCS) Group,
 *                           Computer Science Institute,
 *                           Department of Science and Technological Innovation,
 *                           University of Piemonte Orientale,
 *                           Alessandria (Italy)]
 *
 * This file is part of dcsxx-testbed (below referred to as "this program").
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef DCS_TESTBED_BASE_SYSTEM_MANAGER_HPP
#define DCS_TESTBED_BASE_SYSTEM_MANAGER_HPP


#include <boost/shared_ptr.hpp>
#include <dcs/testbed/base_virtual_machine.hpp>
#include <vector>


namespace dcs { namespace testbed {

template <typename TraitsT>
class base_system_manager
{
	public: typedef TraitsT traits_type;
	protected: typedef base_virtual_machine<traits_type> vm_type;
	protected: typedef ::boost::shared_ptr<vm_type> vm_pointer;


	public: template <typename IterT>
			void manage(IterT first, IterT last)
	{
		::std::vector<vm_pointer> vms(first, last);

		this->do_manage(vms);
	}

	private: void do_manage(::std::vector<vm_pointer> const& vms) = 0;
};

}} // Namespace dcs::testbed

#endif // DCS_TESTBED_BASE_SYSTEM_MANAGER_HPP
