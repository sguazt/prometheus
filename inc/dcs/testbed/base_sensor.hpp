/**
 * \file dcs/testbed/base_sensor.hpp
 *
 * \brief Collect observations.
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
