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


#include <boost/smart_ptr.hpp>
#include <ctime>
#include <dcs/testbed/base_application.hpp>
#include <dcs/testbed/workload_generator_category.hpp>
#include <string>
#include <vector>


namespace dcs { namespace testbed {

template <typename TraitsT>
class base_workload_driver
{
	public: class observation;


	public: typedef TraitsT traits_type;
	public: typedef typename traits_type::real_type real_type;
	public: typedef observation observation_type;
	public: typedef base_application<traits_type> app_type;
	public: typedef ::boost::shared_ptr<app_type> app_pointer;


	public: class observation
	{
		friend class base_workload_driver;


		public: ::std::time_t timestamp() const
		{
			return ts_;
		}

		public: ::std::string operation() const
		{
			return op_;
		}

		public: real_type value() const
		{
			return val_;
		}

		protected: void timestamp(::std::time_t val)
		{
			ts_ = val;
		}

		protected: void operation(::std::string const& val)
		{
			op_ = val;
		}

		protected: void value(real_type val)
		{
			val_ = val;
		}


		private: ::std::time_t ts_;
		private: ::std::string op_;
		private: real_type val_;
	}; // observation


	protected: static observation_type make_observation(::std::time_t ts,
														::std::string const& op,
														real_type val)
	{
		observation_type obs;
		obs.timestamp(ts);
		obs.operation(op);
		obs.value(val);

		return obs;
	}


	public: virtual ~base_workload_driver()
	{
	}

	public: workload_generator_category category() const
	{
		return do_category();
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
		do_reset();
	}

	public: void start()
	{
		do_start();
	}

	public: void stop()
	{
		do_stop();
	}

	public: bool done() const
	{
		return do_done();
	}

	public: bool ready() const
	{
		return do_ready();
	}

	public: bool has_observation() const
	{
		return do_has_observation();
	}

	public: ::std::vector<observation> observations() const
	{
		return do_observations();
	}

	private: virtual workload_generator_category do_category() const = 0;

	private: virtual void do_app(app_pointer const& p_app) = 0;

	private: virtual app_pointer do_app() = 0;

	private: virtual app_pointer do_app() const = 0;

	private: virtual void do_reset() = 0;

	private: virtual void do_start() = 0;

	private: virtual void do_stop() = 0;

	private: virtual bool do_done() const = 0;

	private: virtual bool do_ready() const = 0;

	private: virtual bool do_has_observation() const = 0;

	private: virtual ::std::vector<observation> do_observations() const = 0;

}; // base_workload_driver

}} // Namespace dcs::testbed

#endif // DCS_TESTBED_BASE_WORKLOAD_DRIVER_HPP
