/**
 * \file dcs/testbed/base_application_manager.hpp
 *
 * \brief Base class for application managers.
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

#ifndef DCS_TESTBED_BASE_APPLICATION_MANAGER_HPP
#define DCS_TESTBED_BASE_APPLICATION_MANAGER_HPP


#include <boost/shared_ptr.hpp>
#include <dcs/testbed/base_application.hpp>
#include <vector>


namespace dcs { namespace testbed {

template <typename TraitsT>
class base_application_manager
{
	public: typedef TraitsT traits_type;
	public: typedef typename traits_type::uint_type uint_type;
	protected: typedef base_application<traits_type> app_type;
	public: typedef ::boost::shared_ptr<app_type> app_pointer;


	/// Set the sampling time (in secs)
	public: void sampling_time(uint_type val)
	{
		do_sampling_time(val);
	}

	/// Get the sampling time (in secs)
	public: uint_type sampling_time() const
	{
		return do_sampling_time();
	}

	/// Get the control time (in secs)
	public: uint_type control_time() const
	{
		return do_control_time();
	}

	/// Set the control time (in secs)
	public: void control_time(uint_type val)
	{
		do_control_time(val);
	}

	public: void app(app_pointer const& p_app)
	{
		do_app(p_app);
	}

	public: app_pointer app()
	{
		return do_app();
	}

	public: app_pointer app() const
	{
		return do_app();
	}

	public: void reset()
	{
		this->do_reset();
	}

	public: void sample()
	{
		this->do_sample();
	}

	public: void control()
	{
		this->do_control();
	}

	private: virtual void do_sampling_time(uint_type val) = 0;

	private: virtual uint_type do_sampling_time() const = 0;

	private: virtual void do_control_time(uint_type val) = 0;

	private: virtual uint_type do_control_time() const = 0;

	private: virtual void do_app(app_pointer const& p_app) = 0;

	private: virtual app_pointer do_app() = 0;

	private: virtual app_pointer do_app() const = 0;

	private: virtual void do_reset() = 0;

	private: virtual void do_sample() = 0;

	private: virtual void do_control() = 0;
};

}} // Namespace dcs::testbed

#endif // DCS_TESTBED_BASE_APPLICATION_MANAGER_HPP
