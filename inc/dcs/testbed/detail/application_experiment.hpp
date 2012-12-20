/**
 * \file dcs/testbed/detail/application_experiment.hpp
 *
 * \brief Represents an experiment for a single application.
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

#ifndef DCS_TESTBED_DETAIL_APPLICATION_EXPERIMENT_HPP
#define DCS_TESTBED_DETAIL_APPLICATION_EXPERIMENT_HPP


#include <boost/chrono.hpp>
#include <boost/smart_ptr.hpp>
#include <boost/thread.hpp>
#include <dcs/assert.hpp>
#include <dcs/debug.hpp>
#include <dcs/exception.hpp>
#include <dcs/testbed/base_application_manager.hpp>
#include <dcs/testbed/detail/runnable.hpp>
#include <stdexcept>


namespace dcs { namespace testbed { namespace detail {

template <typename T, typename MT>
struct sampler_runnable
{
	sampler_runnable(::boost::weak_ptr<T> const& ptr, MT& mutex)
	: wp_(ptr),
	  mtx_(mutex)
	{
	}

	void operator()()
	{
		DCS_DEBUG_TRACE("SAMPLER THREAD: Entering...");

		::boost::shared_ptr<T> sp(wp_.lock());

		typename T::traits_type::uint_type ts = sp->sampling_time();
 
		// Loop forever until we get interrupted
		while (true)
		{
			{
				::boost::lock_guard< ::boost::mutex > lock(mtx_);

				sp->sample();
			}

			::boost::this_thread::sleep_for(::boost::chrono::seconds(ts));
		}

		DCS_DEBUG_TRACE("SAMPLER THREAD: Leaving...");
	}

	::boost::weak_ptr<T> wp_;
	MT& mtx_;
}; // sampler_runnable

template <typename T, typename MT>
struct controller_runnable
{
	controller_runnable(::boost::weak_ptr<T> const& ptr, MT& mutex)
	: wp_(ptr),
	  mtx_(mutex)
	{
	}

	void operator()()
	{
		DCS_DEBUG_TRACE("CONTROLLER THREAD: Entering...");

		::boost::shared_ptr<T> sp(wp_.lock());

		typename T::traits_type::uint_type ts = sp->control_time();
 
		// Loop forever until we get interrupted
		while (true)
		{
			{
				::boost::lock_guard< ::boost::mutex > lock(mtx_);

				sp->control();
			}

			::boost::this_thread::sleep_for(::boost::chrono::seconds(ts));
		}

		DCS_DEBUG_TRACE("CONTROLLER THREAD: Leaving...");
	}

	::boost::weak_ptr<T> wp_;
	MT& mtx_;
}; // controller_runnable

template <typename TraitsT>
class application_experiment
{
	public: typedef TraitsT traits_type;
	public: typedef typename traits_type::real_type real_type;
	private: typedef base_application<traits_type> app_type;
	public: typedef ::boost::shared_ptr<app_type> app_pointer;
	private: typedef base_workload_driver<traits_type> driver_type;
	public: typedef ::boost::shared_ptr<driver_type> driver_pointer;
	private: typedef base_application_manager<traits_type> manager_type;
	public: typedef ::boost::shared_ptr<manager_type> manager_pointer;


	public: application_experiment(app_pointer const& p_app,
								   driver_pointer const& p_drv,
								   manager_pointer const& p_mgr)
	: p_app_(p_app),
	  p_drv_(p_drv),
	  p_mgr_(p_mgr)
	{
	}

	public: void app(app_pointer const& p_app)
	{
		p_app_ = p_app;
	}

	public: void driver(driver_pointer const& p_drv)
	{
		p_drv_ = p_drv;
	}

	public: void manager(manager_pointer const& p_mgr)
	{
		p_mgr_ = p_mgr;
	}

	public: void run()
	{
		DCS_ASSERT(p_app_,
				   DCS_EXCEPTION_THROW(::std::runtime_error,
									   "Application not set"));
		DCS_ASSERT(p_drv_,
				   DCS_EXCEPTION_THROW(::std::runtime_error,
									   "Driver not set"));
		DCS_ASSERT(p_mgr_,
				   DCS_EXCEPTION_THROW(::std::runtime_error,
									   "Manager not set"));

		const unsigned long zzz_time(5);

		p_mgr_->app(p_app_);
		p_mgr_->reset();
		p_drv_->app(p_app_);
		p_drv_->reset();

		p_drv_->start();

		::boost::thread_group mgr_thd_grp;
		::boost::mutex mgr_mtx;
		bool mgr_run(false);
		while (!p_drv_->done())
		{
			if (!mgr_run && p_drv_->ready())
			{
				sampler_runnable<manager_type,::boost::mutex> mgr_smp_runner(p_mgr_, mgr_mtx);
				mgr_thd_grp.create_thread(mgr_smp_runner);

				controller_runnable<manager_type,::boost::mutex> mgr_ctl_runner(p_mgr_, mgr_mtx);
				mgr_thd_grp.create_thread(mgr_ctl_runner);

				mgr_run = true;
			}

			::boost::this_thread::sleep_for(::boost::chrono::seconds(zzz_time));
		}

		mgr_thd_grp.interrupt_all();
		mgr_thd_grp.join_all();
	}


	private: app_pointer p_app_; ///< Pointer to the application
	private: driver_pointer p_drv_; ///< Pointer to the application driver
	private: manager_pointer p_mgr_; ///< Pointer to the application manager
}; // application_experiment

}}} // Namespace dcs::testbed::detail

#endif // DCS_TESTBED_DETAIL_APPLICATION_EXPERIMENT_HPP
