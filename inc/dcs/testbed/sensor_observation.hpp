/**
 * \file dcs/testbed/sensor_observation.hpp
 *
 * \brief Observation sampled from a sensor.
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

#ifndef DCS_TESTBED_SENSOR_OBSERVATION_HPP
#define DCS_TESTBED_SENSOR_OBSERVATION_HPP


#include <ctime>
#include <string>


namespace dcs { namespace testbed {

template <typename TraitsT>
class sensor_observation
{
	public: typedef TraitsT traits_type;
	public: typedef typename traits_type::real_type real_type;


	protected: sensor_observation()
	{
	}

	public: sensor_observation(::std::time_t ts, ::std::string const& lbl, real_type val)
	: ts_(ts),
	  lbl_(lbl),
	  val_(val)
	{
	}

	public: ::std::time_t timestamp() const
	{
		return ts_;
	}

	public: ::std::string label() const
	{
		return lbl_;
	}

	public: real_type value() const
	{
		return val_;
	}

	protected: void timestamp(::std::time_t val)
	{
		ts_ = val;
	}

	protected: void label(::std::string const& val)
	{
		lbl_ = val;
	}

	protected: void value(real_type val)
	{
		val_ = val;
	}


	private: ::std::time_t ts_;
	private: ::std::string lbl_;
	private: real_type val_;
}; // observation

}} // Namespace dcs::testbed

#endif // DCS_TESTBED_SENSOR_OBSERVATION_HPP
