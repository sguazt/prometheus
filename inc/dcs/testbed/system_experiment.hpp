/**
 * \file dcs/testbed/system_experiment.hpp
 *
 * \brief Performs system experiment experiments.
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
 * along with dcsxx-testbed.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef DCS_TESTBED_SYSTEM_EXPERIMENT_HPP
#define DCS_TESTBED_SYSTEM_EXPERIMENT_HPP

//#include <boost/chrono.hpp>
#include <boost/smart_ptr.hpp>
#include <boost/thread.hpp>
#include <dcs/assert.hpp>
#include <dcs/debug.hpp>
#include <dcs/exception.hpp>
#include <dcs/testbed/base_application.hpp>
#include <dcs/testbed/base_application_manager.hpp>
#include <dcs/testbed/base_workload_driver.hpp>
#include <dcs/testbed/detail/application_experiment.hpp>
#include <dcs/testbed/detail/runnable.hpp>
#include <stdexcept>
#include <vector>

//#ifdef DCS_DEBUG
//# include <boost/numeric/ublas/io.hpp>
//#endif // DCS_DEBUG


namespace dcs { namespace testbed {

template <typename TraitsT>
class system_experiment
{
	public: typedef TraitsT traits_type;
	//public: typedef typename traits_type::real_type real_type;
	private: typedef base_application<traits_type> app_type;
	public: typedef ::boost::shared_ptr<app_type> app_pointer;
	private: typedef base_application_manager<traits_type> app_manager_type;
	public: typedef ::boost::shared_ptr<app_manager_type> app_manager_pointer;
	private: typedef base_workload_driver<traits_type> app_driver_type;
	public: typedef ::boost::shared_ptr<app_driver_type> app_driver_pointer;
	private: typedef detail::application_experiment<traits_type> app_experiment_type;
	private: typedef ::boost::shared_ptr<app_experiment_type> app_experiment_pointer;
	private: typedef ::std::vector<app_experiment_pointer> app_experiment_container;


	public: void add_app(app_pointer const& p_app,
						 app_driver_pointer const& p_drv,
						 app_manager_pointer const& p_mgr)
	{
		app_exps_.push_back(::boost::make_shared<app_experiment_type>(p_app, p_drv, p_mgr));
	}

	/**
	 * \brief Perform system experiment.
	 */
	public: void run()
	{
		typedef typename app_experiment_container::const_iterator app_experiment_iterator;

		DCS_DEBUG_TRACE( "BEGIN Execution of System EXPERIMENT" );

		app_experiment_iterator app_exp_end_it(app_exps_.end());
		app_experiment_iterator app_exp_beg_it(app_exps_.begin());

		if (app_exp_beg_it == app_exp_end_it)
		{
			// No experiment -> don't run anything
			return;
		}

		::boost::thread_group exp_thds;
		for (app_experiment_iterator app_exp_it = app_exp_beg_it;
			 app_exp_it != app_exp_end_it;
			 ++app_exp_it)
		{
			app_experiment_pointer p_app_exp(*app_exp_it);

			detail::runnable<app_experiment_type> exp_runner(*app_exp_it);
			//::boost::thread exp_thd(exp_runner);
			exp_thds.create_thread(exp_runner);
			//::boost::this_thread::sleep_for(::boost::chrono::seconds(2));
		}
		exp_thds.join_all();

		DCS_DEBUG_TRACE( "END Execution of System EXPERIMENT" );
	}


	private: app_experiment_container app_exps_; ///< Application experiments container
}; // system_experiment

}} // Namespace dcs::testbed

#endif // DCS_TESTBED_SYSTEM_EXPERIMENT_HPP
