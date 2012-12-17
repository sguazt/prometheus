/**
 * \file dcs/testbed/sensor_observation.hpp
 *
 * \brief Observation sampled from a sensor.
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
