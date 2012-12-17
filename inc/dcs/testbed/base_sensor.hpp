/**
 * \file dcs/testbed/base_sensor.hpp
 *
 * \brief Collect observations.
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

#ifndef DCS_TESTBED_BASE_SENSOR_HPP
#define DCS_TESTBED_BASE_SENSOR_HPP


#include <dcs/testbed/sensor_observation.hpp>
#include <vector>


namespace dcs { namespace testbed {

template <typename TraitsT>
class base_sensor
{
	public: typedef TraitsT traits_type;
	public: typedef sensor_observation<traits_type> observation_type;


	public: void sense()
	{
		do_sense();
	}

	public: bool has_observations() const
	{
		return do_has_observations();
	}

	public: ::std::vector<observation_type> observations() const
	{
		return do_observations();
	}

	private: virtual void do_sense() = 0;

	private: virtual void do_reset() = 0;

	private: virtual bool do_has_observations() const = 0;

	private: virtual ::std::vector<observation_type> do_observations() const = 0;
}; // base_sensor

}} // Namespace dcs::testbed

#endif // DCS_TESTBED_BASE_SENSOR_HPP
