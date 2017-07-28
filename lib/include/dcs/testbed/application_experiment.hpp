/**
 * \file dcs/testbed/application_experiment.hpp
 *
 * \brief Represents an experiment for a single application.
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

#ifndef DCS_TESTBED_APPLICATION_EXPERIMENT_HPP
#define DCS_TESTBED_APPLICATION_EXPERIMENT_HPP


#include <boost/chrono.hpp>
#include <boost/signals2/signal.hpp>
#include <boost/smart_ptr.hpp>
#include <boost/thread.hpp>
#include <dcs/assert.hpp>
#include <dcs/debug.hpp>
#include <dcs/exception.hpp>
#include <dcs/testbed/base_application.hpp>
#include <dcs/testbed/base_application_manager.hpp>
#include <dcs/testbed/base_workload_driver.hpp>
#include <dcs/testbed/detail/runnable.hpp>
#include <map>
#include <sstream>
#include <stdexcept>
#include <vector>


namespace dcs { namespace testbed {

namespace detail { namespace /*<unnamed>*/ {

/**
 * \brief Class used by the sampler thread.
 *
 * \author Marco Guazzone (marco.guazzone@gmail.com)
 */
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

		// Transform secs into millisecs so that the conversion real->uint keeps some decimal
		typename T::traits_type::uint_type ts = 1000.0*sp->sampling_time();
 
		// Loop forever until we get interrupted
		while (true)
		{
			{
				::boost::lock_guard< ::boost::mutex > lock(mtx_);

				sp->sample();
			}

			::boost::this_thread::sleep_for(::boost::chrono::milliseconds(ts));
		}

		DCS_DEBUG_TRACE("SAMPLER THREAD: Leaving...");
	}

	::boost::weak_ptr<T> wp_;
	MT& mtx_;
}; // sampler_runnable

/**
 * \brief Class used by the controller thread.
 *
 * \author Marco Guazzone (marco.guazzone@gmail.com)
 */
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

		// Transform secs into millisecs so that the conversion real-> uint keeps some decimal
		typename T::traits_type::uint_type ts = 1000.0*sp->control_time();
 
		// Loop forever until we get interrupted
		while (true)
		{
			{
				::boost::lock_guard< ::boost::mutex > lock(mtx_);

				sp->control();
			}

			::boost::this_thread::sleep_for(::boost::chrono::milliseconds(ts));
		}

		DCS_DEBUG_TRACE("CONTROLLER THREAD: Leaving...");
	}

	::boost::weak_ptr<T> wp_;
	MT& mtx_;
}; // controller_runnable

/*
template <typename T, typename MT>
struct monitor_runnable
{
	monitor_runnable(::boost::weak_ptr<T> const& ptr, MT& mutex)
	: wp_(ptr),
	  mtx_(mutex)
	{
	}

	void operator()()
	{
		DCS_DEBUG_TRACE("MONITOR THREAD: Entering...");

		::boost::shared_ptr<T> sp(wp_.lock());

		// Transform secs into millisecs so that the conversion real -> uint keeps some decimal
		typename T::traits_type::uint_type ts = 1000.0*sp->monitoring_time();

		// Loop forever until we get interrupted
		while (true)
		{
			{
				::boost::lock_guard< ::boost::mutex > lock(mtx_);

				sp->monitor();
			}

			::boost::this_thread::sleep_for(::boost::chrono::milliseconds(ts));
		}

		DCS_DEBUG_TRACE("MONITOR THREAD: Leaving...");
	}


	::boost::weak_ptr<T> wp_;
	MT& mtx_;
}; // monitor_runnable
*/
 }} // Namespace detail::<unnamed>

/**
 * \brief Represents an experiment for a single application.
 *
 * \tparam TraitsT Traits type.
 *
 * \author Marco Guazzone (marco.guazzone@gmail.com)
 */
template <typename TraitsT>
class application_experiment
{
	private: typedef application_experiment<TraitsT> self_type;
	public: typedef TraitsT traits_type;
	public: typedef typename traits_type::real_type real_type;
	public: typedef unsigned long identifier_type;
	private: typedef base_application<traits_type> app_type;
	public: typedef ::boost::shared_ptr<app_type> app_pointer;
	private: typedef base_workload_driver<traits_type> driver_type;
	public: typedef ::boost::shared_ptr<driver_type> driver_pointer;
	private: typedef base_application_manager<traits_type> manager_type;
	public: typedef ::boost::shared_ptr<manager_type> manager_pointer;
	private: typedef ::boost::signals2::signal<void (self_type const&)> signal_type;
	private: typedef ::boost::shared_ptr<signal_type> signal_pointer;


	//private: static ::boost::mutex mtx_;
	private: static identifier_type next_id_;


	/// Generates a unique identifier for an instance of this class
	private: static identifier_type make_id()
	{
		//::boost::lock_guard< ::boost::mutex > lock(mtx_);

		return next_id_++;
	}

	/// Generates a name for an instance of this class
	private: static std::string make_name(identifier_type id)
	{
		std::ostringstream oss;
		oss << "Experiment " << id;
		return oss.str();
	}

	public: application_experiment()
	: id_(make_id()),
	  name_(make_name(id_)),
	  restore_state_(true),
	  p_sta_sig_(new signal_type()),
	  p_sto_sig_(new signal_type())
	{
	}

	public: application_experiment(app_pointer const& p_app,
								   driver_pointer const& p_drv,
								   manager_pointer const& p_mgr)
	: id_(make_id()),
	  name_(make_name(id_)),
	  p_app_(p_app),
	  p_drv_(p_drv),
	  p_mgr_(p_mgr),
	  restore_state_(true),
	  p_sta_sig_(new signal_type()),
	  p_sto_sig_(new signal_type())
	{
	}

	public: ~application_experiment()
	{
		try
		{
			restore_app_state();
		}
		catch (...)
		{
		}
	}

	/// Gets the unique identifier associated with this experiment
	public: identifier_type id() const
	{
		return id_;
	}

	/// Gets the name associated with this experiment
	public: std::string name() const
	{
		return name_;
	}

	/// Sets the name to associate with this experiment
	public: void name(std::string const& s)
	{
		name_ = s;
	}

	/// Sets the application under test
	public: void app(app_pointer const& p_app)
	{
		p_app_ = p_app;
	}

	/// Gets the application under test
	public: app_type& app()
	{
		return *p_app_;
	}

	/// Gets the application under test
	public: app_type const& app() const
	{
		return *p_app_;
	}

	/// Sets the workload generator for the application under test
	public: void driver(driver_pointer const& p_drv)
	{
		p_drv_ = p_drv;
	}

	/// Gets the workload generator for the application under test
	public: driver_type& driver()
	{
		return *p_drv_;
	}

	/// Gets the workload generator for the application under test
	public: driver_type const& driver() const
	{
		return *p_drv_;
	}

	/// Sets the manager for the application under test
	public: void manager(manager_pointer const& p_mgr)
	{
		p_mgr_ = p_mgr;
	}

	/// Gets the manager for the application under test
	public: manager_type& manager()
	{
		return *p_mgr_;
	}

	/// Gets the manager for the application under test
	public: manager_type const& manager() const
	{
		return *p_mgr_;
	}

	/// Controls if at the end of the experiment the state of the application VMs must be restored to the one before the experiment started
	public: void restore_state(bool value)
	{
		restore_state_ = value;
	}

	/// Tells if at the end of the experiment the state of the application VMs must be restored to the one before the experiment started
	public: bool restore_state() const
	{
		return restore_state_;
	}

	/// Adds a callback for the on-start event
	public: template <typename FuncT>
			void add_on_start_handler(FuncT f)
	{
		p_sta_sig_->connect(f);
	}

	/// Adds a callback for the on-stop event
	public: template <typename FuncT>
			void add_on_stop_handler(FuncT f)
	{
		p_sto_sig_->connect(f);
	}

	/// Runs the experiment
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

//		typedef typename monitor_container::iterator monitor_iterator;

		const unsigned long zzz_time = 5;

		running_ = true;

		save_app_state();

		p_mgr_->app(p_app_);
		p_mgr_->reset();
		p_drv_->app(p_app_);
		p_drv_->reset();
//		monitor_iterator mon_beg_it(mons_.begin());
//		monitor_iterator mon_end_it(mons_.end());
//		for (monitor_iterator mon_it = mon_beg_it;
//			 mon_it != mon_end_it;
//			 ++mon_it)
//		{
//			mon_it->app(p_app_);
//			mon_it->reset();
//		}

		(*p_sta_sig_)(*this);

		p_drv_->start();

		::boost::thread_group mgr_thd_grp;
		::boost::mutex mgr_mtx;
		bool mgr_run = false;
		while (!p_drv_->done())
		{
			if (!mgr_run && p_drv_->ready())
			{
				detail::sampler_runnable<manager_type,::boost::mutex> mgr_smp_runner(p_mgr_, mgr_mtx);
				mgr_thd_grp.create_thread(mgr_smp_runner);

				detail::controller_runnable<manager_type,::boost::mutex> mgr_ctl_runner(p_mgr_, mgr_mtx);
				mgr_thd_grp.create_thread(mgr_ctl_runner);

//				for (monitor_iterator mon_it = mon_beg_it;
//					 mon_it != mon_end_it;
//					 ++mon_it)
//				{
//					detail::monitor_runnable<monitor_type,::boost::mutex> mgr_mon_runner(*mon_it, mgr_mtx);
//					mgr_thd_grp.create_thread(mgr_mon_runner);
//				}

				mgr_run = true;
			}

			::boost::this_thread::sleep_for(::boost::chrono::seconds(zzz_time));
		}

		mgr_thd_grp.interrupt_all();
		mgr_thd_grp.join_all();

		p_drv_->stop();

		restore_app_state();

		(*p_sto_sig_)(*this);

		running_ = false;
	}

	protected: app_pointer app_ptr()
	{
		return p_app_;
	}

	protected: app_pointer const& app_ptr() const
	{
		return p_app_;
	}

	protected: driver_pointer driver_ptr()
	{
		return p_drv_;
	}

	protected: driver_pointer const& driver_ptr() const
	{
		return p_drv_;
	}

	protected: manager_pointer manager_ptr()
	{
		return p_mgr_;
	}

	protected: manager_pointer const& manager_ptr() const
	{
		return p_mgr_;
	}

	private: void save_app_state()
	{
		if (!restore_state_ || !p_app_ || !running_)
		{
			return;
		}

		typedef typename app_type::vm_pointer vm_pointer;
		typedef ::std::vector<vm_pointer> vm_container;
		typedef typename vm_container::const_iterator vm_iterator;
		vm_container vms = this->app().vms();
		vm_iterator vm_end_it = vms.end();
		for (vm_iterator vm_it = vms.begin();
			 vm_it != vm_end_it;
			 ++vm_it)
		{
			vm_pointer p_vm(*vm_it);

			// check: p_vm != null
			DCS_DEBUG_ASSERT( p_vm );

			//TODO: add a new class (e.g., vm_state) and new methods in the VM
			//      class so that we can simply call "old_state = vm->state()"
			//      and "vm->state(old_state)" and all the internals are
			//      managed by the VM class

			vm_states_[p_vm->id()].push_back(p_vm->cpu_share());
			vm_states_[p_vm->id()].push_back(p_vm->memory_share());
		}
	}

	private: void restore_app_state()
	{
		if (!restore_state_ || !p_app_ || !running_)
		{
			return;
		}

		typedef typename app_type::vm_pointer vm_pointer;
		typedef ::std::vector<vm_pointer> vm_container;
		typedef typename vm_container::const_iterator vm_iterator;
		vm_container vms = this->app().vms();
		vm_iterator vm_end_it = vms.end();
		for (vm_iterator vm_it = vms.begin();
			 vm_it != vm_end_it;
			 ++vm_it)
		{
			vm_pointer p_vm(*vm_it);

			// check: p_vm != null
			DCS_DEBUG_ASSERT( p_vm );

			if (vm_states_.count(p_vm->id()) > 0)
			{
				p_vm->memory_share(vm_states_.at(p_vm->id()).back());
				vm_states_.at(p_vm->id()).pop_back();
				p_vm->cpu_share(vm_states_.at(p_vm->id()).back());
				vm_states_.at(p_vm->id()).pop_back();
			}
		}
	}


	private: const identifier_type id_; ///< The unique experiment identifier
	private: std::string name_; ///< A mnemonic name for the experiment
	private: app_pointer p_app_; ///< Pointer to the application
	private: driver_pointer p_drv_; ///< Pointer to the application driver
	private: manager_pointer p_mgr_; ///< Pointer to the application manager
	private: bool restore_state_; ///< Tell if the state of the application should or should not be restored after experiment's completion
	private: signal_pointer p_sta_sig_;
	private: signal_pointer p_sto_sig_;
	private: ::std::map< typename app_type::vm_type::identifier_type,std::vector<real_type> > vm_states_;
	private: bool running_;
}; // application_experiment

template <typename T>
typename application_experiment<T>::identifier_type application_experiment<T>::next_id_ = 0;

}} // Namespace dcs::testbed

#endif // DCS_TESTBED_APPLICATION_EXPERIMENT_HPP
