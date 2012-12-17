/**
 * \file dcs/testbed/application.hpp
 *
 * \brief Generic application.
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

#ifndef DCS_TESTBED_APPLICATION_HPP
#define DCS_TESTBED_APPLICATION_HPP


#include <cstddef>
#include <dcs/assert.hpp>
#include <dcs/exception.hpp>
#include <dcs/testbed/application_performance_category.hpp>
#include <dcs/testbed/base_application.hpp>
#include <dcs/testbed/base_virtual_machine.hpp>
#include <map>
#include <stdexcept>
#include <vector>


namespace dcs { namespace testbed {

template <typename TraitsT>
class application: public base_application<TraitsT>
{
	private: typedef base_application<TraitsT> base_type;
	public: typedef typename base_type::traits_type traits_type;
	public: typedef typename base_type::vm_pointer vm_pointer;
	public: typedef typename base_type::sensor_pointer sensor_pointer;
	private: typedef ::std::vector<vm_pointer> vm_container;
	private: typedef ::std::map<application_performance_category,sensor_pointer> sensor_map;


	public: application()
	{
	}

	public: template <typename IterT>
			application(IterT vm_first, IterT vm_last)
	: vms_(vm_first, vm_last)
	{
	}

//	public: vm_pointer vm(size_type pos)
//	{
//		DCS_ASSERT(pos < vms_.size(),
//				   DCS_EXCEPTION_THROW(::std::invalid_argument,
//									   "Invalid number of VM"));
//
//		return vms_.at(pos);
//	}

	private: ::std::size_t do_num_vms() const
	{
		return vms_.size();
	}

	private: vm_container do_vms() const
	{
		return vms_;
	}

	private: void do_register_sensor(application_performance_category cat, sensor_pointer const& p_sens)
	{
		DCS_ASSERT(p_sens,
				   DCS_EXCEPTION_THROW(::std::invalid_argument,
									   "Invalid sensor"));

		sensors_[cat] = p_sens;
	}

	private: void do_deregister_sensor(application_performance_category cat)
	{
		DCS_ASSERT(sensors_.count(cat) > 0,
				   DCS_EXCEPTION_THROW(::std::invalid_argument,
									   "Invalid category: sensor not found"));

		sensors_.erase(cat);
	}

	private: sensor_pointer do_sensor(application_performance_category cat)
	{
		DCS_ASSERT(sensors_.count(cat) > 0,
				   DCS_EXCEPTION_THROW(::std::invalid_argument,
									   "Invalid category: sensor not found"));

		return sensors_.at(cat);
	}

	private: sensor_pointer do_sensor(application_performance_category cat) const
	{
		DCS_ASSERT(sensors_.count(cat) > 0,
				   DCS_EXCEPTION_THROW(::std::invalid_argument,
									   "Invalid category: sensor not found"));

		return sensors_.at(cat);
	}


	private: vm_container vms_;
	private: sensor_map sensors_;
}; // application

}} // Namespace dcs::testbed

#endif // DCS_TESTBED_APPLICATION_HPP
