/**
 * \file dcs/testbed/base_workload_driver.hpp
 *
 * \brief Base class for workload drivers.
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

#ifndef DCS_TESTBED_BASE_WORKLOAD_DRIVER_HPP
#define DCS_TESTBED_BASE_WORKLOAD_DRIVER_HPP


#include <boost/smart_ptr.hpp>
#include <dcs/testbed/base_application.hpp>
#include <dcs/testbed/workload_generator_category.hpp>


namespace dcs { namespace testbed {

template <typename TraitsT>
class base_workload_driver
{
	public: typedef TraitsT traits_type;
	public: typedef typename traits_type::real_type real_type;
	public: typedef base_application<traits_type> app_type;
	public: typedef ::boost::shared_ptr<app_type> app_pointer;


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

	private: virtual workload_generator_category do_category() const = 0;

	private: virtual void do_app(app_pointer const& p_app) = 0;

	private: virtual app_pointer do_app() = 0;

	private: virtual app_pointer do_app() const = 0;

	private: virtual void do_reset() = 0;

	private: virtual void do_start() = 0;

	private: virtual void do_stop() = 0;

	private: virtual bool do_done() const = 0;

	private: virtual bool do_ready() const = 0;
}; // base_workload_driver

}} // Namespace dcs::testbed

#endif // DCS_TESTBED_BASE_WORKLOAD_DRIVER_HPP
