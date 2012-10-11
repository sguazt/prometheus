/**
 * \file dcs/testbed/base_workload_driver.hpp
 *
 * \brief Base class for workload drivers.
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
 * along with dcsxx-testbed. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef DCS_TESTBED_BASE_WORKLOAD_DRIVER_HPP
#define DCS_TESTBED_BASE_WORKLOAD_DRIVER_HPP


#include <string>
#include <vector>


namespace dcs { namespace testbed {

class base_workload_driver
{
	public: virtual ~base_workload_driver()
	{
	}

	public: void start()
	{
		this->do_start();
	}

	public: void stop()
	{
		this->do_stop();
	}

	public: bool alive() const
	{
		return do_alive();
	}

	public: bool ready() const
	{
		return do_ready();
	}

	public: bool has_observation() const
	{
		return do_has_observation();
	}

	public: double observation() const
	{
		return do_observation();
	}

	private: virtual void do_start() = 0;

	private: virtual void do_stop() = 0;

	private: virtual bool do_alive() const = 0;

	private: virtual bool do_ready() const = 0;

	private: virtual bool do_has_observation() const = 0;

	private: virtual double do_observation() const = 0;

}; // base_workload_driver

}} // Namespace dcs::testbed

#endif // DCS_TESTBED_BASE_WORKLOAD_DRIVER_HPP
