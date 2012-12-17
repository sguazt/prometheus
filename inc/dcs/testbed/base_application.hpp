/**
 * \file dcs/testbed/base_application.hpp
 *
 * \brief Base class for applications.
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

#ifndef DCS_TESTBED_BASE_APPLICATION_HPP
#define DCS_TESTBED_BASE_APPLICATION_HPP


#include <boost/shared_ptr.hpp>
#include <cstddef>
#include <dcs/testbed/application_performance_category.hpp>
#include <dcs/testbed/base_sensor.hpp>
#include <dcs/testbed/base_virtual_machine.hpp>


namespace dcs { namespace testbed {

template <typename TraitsT>
class base_application
{
	public: typedef TraitsT traits_type;
	public: typedef base_virtual_machine<traits_type> vm_type;
	public: typedef ::boost::shared_ptr<vm_type> vm_pointer;
	public: typedef base_sensor<traits_type> sensor_type;
	public: typedef ::boost::shared_ptr<sensor_type> sensor_pointer;


	public: virtual ~base_application()
	{
	}

	public: ::std::size_t num_vms() const
	{
		return do_num_vms();
	}

	public: ::std::vector<vm_pointer> vms() const
	{
		return do_vms();
	}

	public: void register_sensor(application_performance_category cat, sensor_pointer const& p_sens)
	{
		do_register_sensor(cat, p_sens);
	}

	public: void deregister_sensor(application_performance_category cat)
	{
		do_deregister_sensor(cat);
	}

	public: sensor_pointer sensor(application_performance_category cat)
	{
		return do_sensor(cat);
	}

	public: sensor_pointer sensor(application_performance_category cat) const
	{
		return do_sensor(cat);
	}

	private: virtual ::std::size_t do_num_vms() const = 0;

	private: virtual ::std::vector<vm_pointer> do_vms() const = 0;

	private: virtual void do_register_sensor(application_performance_category cat, sensor_pointer const& p_sens) = 0;

	private: virtual void do_deregister_sensor(application_performance_category cat) = 0;

	private: virtual sensor_pointer do_sensor(application_performance_category cat) = 0;

	private: virtual sensor_pointer do_sensor(application_performance_category cat) const = 0;
}; // base_application

}} // Namespace dcs::testbed

#endif // DCS_TESTBED_BASE_APPLICATION_HPP
