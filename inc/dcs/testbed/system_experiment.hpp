/**
 * \file dcs/testbed/system_experiment.hpp
 *
 * \brief Performs system experiment experiments.
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

#ifndef DCS_TESTBED_SYSTEM_EXPERIMENT_HPP
#define DCS_TESTBED_SYSTEM_EXPERIMENT_HPP

//#include <boost/chrono.hpp>
#include <boost/signals2/signal.hpp>
#include <boost/smart_ptr.hpp>
#include <boost/thread.hpp>
#include <dcs/assert.hpp>
#include <dcs/debug.hpp>
#include <dcs/exception.hpp>
//#include <dcs/testbed/base_application.hpp>
//#include <dcs/testbed/base_application_manager.hpp>
//#include <dcs/testbed/base_experiment_monitor.hpp>
//#include <dcs/testbed/base_workload_driver.hpp>
//#include <dcs/testbed/detail/application_experiment_runner.hpp>
#include <dcs/testbed/application_experiment.hpp>
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
	private: typedef system_experiment<TraitsT> self_type;
	public: typedef TraitsT traits_type;
	public: typedef typename traits_type::rng_type rng_type;
	//public: typedef typename traits_type::real_type real_type;
//	private: typedef base_application<traits_type> app_type;
//	public: typedef ::boost::shared_ptr<app_type> app_pointer;
//	private: typedef base_application_manager<traits_type> app_manager_type;
//	public: typedef ::boost::shared_ptr<app_manager_type> app_manager_pointer;
//	private: typedef base_workload_driver<traits_type> app_driver_type;
//	public: typedef ::boost::shared_ptr<app_driver_type> app_driver_pointer;
	private: typedef application_experiment<traits_type> app_experiment_type;
	private: typedef ::boost::shared_ptr<app_experiment_type> app_experiment_pointer;
	private: typedef ::std::vector<app_experiment_pointer> app_experiment_container;
	private: typedef ::boost::signals2::signal<void (self_type const&)> signal_type;
	private: typedef ::boost::shared_ptr<signal_type> signal_pointer;


	public: system_experiment()
	: running_(false),
	  p_sta_sig_(new signal_type()),
	  p_sto_sig_(new signal_type()),
	  p_rng_(new rng_type())
	{
	}

	public: void add_app_experiment(app_experiment_pointer const& p_exp)
	{
		app_exps_.push_back(p_exp);
	}

	public: ::std::vector<app_experiment_pointer> experiments() const
	{
		return app_exps_;
	}

	public: template <typename FuncT>
			void add_on_start_handler(FuncT f)
	{
		p_sta_sig_->connect(f);
	}

	public: template <typename FuncT>
			void add_on_stop_handler(FuncT f)
	{
		p_sto_sig_->connect(f);
	}

//	public: void add_app(app_pointer const& p_app,
//						 app_driver_pointer const& p_drv,
//						 app_manager_pointer const& p_mgr,
//						 monitor_pointer const& p_mon)
//	{
//		app_exps_.push_back(::boost::make_shared<app_experiment_type>(p_app, p_drv, p_mgr, p_mon));
//	}

//	public: template <typename MonIterT>
//			void add_app(app_pointer const& p_app,
//						 app_driver_pointer const& p_drv,
//						 app_manager_pointer const& p_mgr,
//						 MonIterT first_mon,
//						 MonIterT last_mon)
//	{
//		app_exps_.push_back(::boost::make_shared<app_experiment_type>(p_app, p_drv, p_mgr, first_mon, last_mon));
//	}

	public: void rng(boost::shared_ptr<rng_type> const& p_rng)
	{
		p_rng_ = p_rng;
	}

	public: boost::shared_ptr<rng_type> const& rng_ptr() const
	{
		return p_rng_;
	}

	/**
	 * \brief Perform system experiment.
	 */
	public: void run()
	{
		typedef typename app_experiment_container::const_iterator app_experiment_iterator;
//		typedef typename monitor_container::const_iterator monitor_iterator;

		DCS_DEBUG_TRACE( "BEGIN Execution of System EXPERIMENT" );

		app_experiment_iterator app_exp_end_it(app_exps_.end());
		app_experiment_iterator app_exp_beg_it(app_exps_.begin());

		(*p_sta_sig_)(*this);

		if (app_exp_beg_it != app_exp_end_it)
		{
			// There is some experiment to run

			::boost::thread_group exp_thds;
			// Create threads for application experiments
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
//			// Create threads for experiment monitors
//			system_experiment_context<traits_type> ctx(this);
//			monitor_iterator mon_end_it(mons_.end());
//			for (monitor_iterator mon_it = mons_.begin();
//				 mon_it != mon_end_it;
//				 ++mon_it)
//			{
//				monitor_pointer p_mon(*mon_it);
//
//				detail::monitor_runnable<monitor_type> mon_runner(p_mon, ctx);
//				exp_thds.create_thread(mon_runner);
//			}
			running_ = true;
			exp_thds.join_all();
			running_ = false;
		}

		(*p_sto_sig_)(*this);

		DCS_DEBUG_TRACE( "END Execution of System EXPERIMENT" );
	}

	public: bool running() const
	{
		return running_;
	}


	private: bool running_; ///< Tells if this system experiment is running
	private: app_experiment_container app_exps_; ///< Application experiments container
//	private: monitor_container mons_; ///< Experiment monitors container
	private: signal_pointer p_sta_sig_;
	private: signal_pointer p_sto_sig_;
	private: boost::shared_ptr<rng_type> p_rng_;
}; // system_experiment

}} // Namespace dcs::testbed

#endif // DCS_TESTBED_SYSTEM_EXPERIMENT_HPP
