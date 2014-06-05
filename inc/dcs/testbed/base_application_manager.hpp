/**
 * \file dcs/testbed/base_application_manager.hpp
 *
 * \brief Base class for application managers.
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

#ifndef DCS_TESTBED_BASE_APPLICATION_MANAGER_HPP
#define DCS_TESTBED_BASE_APPLICATION_MANAGER_HPP


#include <boost/shared_ptr.hpp>
#include <boost/signals2.hpp>
#include <dcs/assert.hpp>
#include <dcs/exception.hpp>
#include <dcs/testbed/application_performance_category.hpp>
#include <dcs/testbed/base_application.hpp>
#include <dcs/testbed/base_virtual_machine.hpp>
#include <dcs/testbed/data_estimators.hpp>
#include <dcs/testbed/data_smoothers.hpp>
#include <dcs/testbed/virtual_machine_performance_category.hpp>
#include <stdexcept>
#include <map>
#include <vector>


namespace dcs { namespace testbed {

template <typename TraitsT>
class base_application_manager
{
	private: typedef base_application_manager<TraitsT> self_type;
	public: typedef TraitsT traits_type;
	public: typedef typename traits_type::real_type real_type;
	protected: typedef base_application<traits_type> app_type;
	protected: typedef base_estimator<real_type> data_estimator_type;
	protected: typedef base_smoother<real_type> data_smoother_type;
	protected: typedef ::std::map<application_performance_category,real_type> target_value_map;
	public: typedef ::boost::shared_ptr<app_type> app_pointer;
	public: typedef ::boost::shared_ptr<data_estimator_type> data_estimator_pointer;
	public: typedef ::boost::shared_ptr<data_smoother_type> data_smoother_pointer;
	public: typedef typename base_virtual_machine<TraitsT>::identifier_type vm_identifier_type;
	protected: typedef ::std::map<application_performance_category,data_estimator_pointer> app_data_estimator_map;
	protected: typedef ::std::map< virtual_machine_performance_category, ::std::map<vm_identifier_type,data_estimator_pointer> > vm_data_estimator_map;
	protected: typedef ::std::map<application_performance_category,data_smoother_pointer> app_data_smoother_map;
	protected: typedef ::std::map< virtual_machine_performance_category, ::std::map<vm_identifier_type,data_smoother_pointer> > vm_data_smoother_map;
	private: typedef ::boost::signals2::signal<void (self_type const&)> signal_type;
	private: typedef ::boost::shared_ptr<signal_type> signal_pointer;


	public: base_application_manager()
	: ts_(1),
	  tc_(1),
	  p_rst_sig_(new signal_type()),
	  p_smp_sig_(new signal_type()),
	  p_ctl_sig_(new signal_type())
	{
	}

	public: virtual ~base_application_manager()
	{
	}

	/// Set the sampling time (in millisecs)
	public: void sampling_time(real_type val)
	{
		// pre: val > 0
		DCS_ASSERT(val > 0,
				   DCS_EXCEPTION_THROW(::std::invalid_argument,
									   "Invalid sampling time: non-positive value"));

		ts_ = val;
	}

	/// Get the sampling time (in millisecs)
	public: real_type sampling_time() const
	{
		return ts_;
	}

	/// Get the control time (in millisecs)
	public: void control_time(real_type val)
	{
		// pre: val > 0
		DCS_ASSERT(val > 0,
				   DCS_EXCEPTION_THROW(::std::invalid_argument,
									   "Invalid control time: non-positive value"));

		tc_ = val;
	}

	/// Set the control time (in millisecs)
	public: real_type control_time() const
	{
		return tc_;
	}

	/// Set the pointer to the managed application
	public: void app(app_pointer const& p_app)
	{
		p_app_ = p_app;
	}

	/// Get the pointer to the managed application
	public: app_type& app()
	{
		return *p_app_;
	}

	/// Get the pointer to the managed application
	public: app_type const& app() const
	{
		return *p_app_;
	}

	public: void data_estimator(application_performance_category cat, data_estimator_pointer const& p_estimator)
	{
		// pre: p_estimator != null
		DCS_ASSERT(p_estimator,
				   DCS_EXCEPTION_THROW(::std::invalid_argument,
									   "Invalid pointer to data estimator"));

        app_estimators_[cat] = p_estimator;
	}

	public: data_estimator_type& data_estimator(application_performance_category cat)
	{
		// pre: exists(app_estimators_[cat]) && app_estimators_[cat] != null
		DCS_ASSERT(app_estimators_.count(cat) > 0 && app_estimators_.at(cat),
				   DCS_EXCEPTION_THROW(::std::invalid_argument,
									   "Invalid category for data estimator"));

		return *(app_estimators_[cat]);
	}

	public: data_estimator_type const& data_estimator(application_performance_category cat) const
	{
		// pre: exists(app_estimators_[cat]) && app_estimators_[cat] != null
		DCS_ASSERT(app_estimators_.count(cat) > 0 && app_estimators_.at(cat),
				   DCS_EXCEPTION_THROW(::std::invalid_argument,
									   "Invalid category for data estimator"));

		return *(app_estimators_.at(cat));
	}

	public: void data_estimator(virtual_machine_performance_category cat, vm_identifier_type const& vm_id, data_estimator_pointer const& p_estimator)
	{
		// pre: p_estimator != null
		DCS_ASSERT(p_estimator,
				   DCS_EXCEPTION_THROW(::std::invalid_argument,
									   "Invalid pointer to data estimator"));

        vm_estimators_[cat][vm_id] = p_estimator;
	}

	public: data_estimator_type& data_estimator(virtual_machine_performance_category cat, vm_identifier_type const& vm_id)
	{
		// pre: exists(vm_estimators_[cat])
		DCS_ASSERT(vm_estimators_.count(cat) > 0,
				   DCS_EXCEPTION_THROW(::std::invalid_argument,
									   "Invalid category for data estimator"));
		// pre: exists(vm_estimators_[cat][vm_id]) && vm_estimators_[cat][vm_id] != null
		DCS_ASSERT(vm_estimators_.at(cat).count(vm_id) > 0 && vm_estimators_.at(cat).at(vm_id),
				   DCS_EXCEPTION_THROW(::std::invalid_argument,
									   "Invalid category for data estimator"));

		return *(vm_estimators_[cat][vm_id]);
	}

	public: data_estimator_type const& data_estimator(virtual_machine_performance_category cat, vm_identifier_type const& vm_id) const
	{
		// pre: exists(vm_estimators_[cat])
		DCS_ASSERT(vm_estimators_.count(cat) > 0,
				   DCS_EXCEPTION_THROW(::std::invalid_argument,
									   "Invalid category for data estimator"));
		// pre: exists(vm_estimators_[cat][vm_id]) && vm_estimators_[cat][vm_id] != null
		DCS_ASSERT(vm_estimators_.at(cat).count(vm_id) > 0 && vm_estimators_.at(cat).at(vm_id),
				   DCS_EXCEPTION_THROW(::std::invalid_argument,
									   "Invalid category for data estimator"));

		return *(vm_estimators_.at(cat).at(vm_id));
	}

	public: void data_smoother(application_performance_category cat, data_smoother_pointer const& p_smoother)
	{
		// pre: p_estimator != null
		DCS_ASSERT(p_smoother,
				   DCS_EXCEPTION_THROW(::std::invalid_argument, "Invalid pointer to data smoother"));

		app_smoothers_[cat] = p_smoother;
	}

	public: data_smoother_type& data_smoother(application_performance_category cat)
	{
		return *(app_smoothers_[cat]);
	}

	public: data_smoother_type const& data_smoother(application_performance_category cat) const
	{
		// pre: exists(smoothers_[cat]) && smoothers_[cat] != null
		DCS_ASSERT(app_smoothers_.count(cat) > 0 && app_smoothers_.at(cat),
				   DCS_EXCEPTION_THROW(::std::invalid_argument,
									   "Invalid category for data smoothers"));

		return *(app_smoothers_.at(cat));
	}

	public: void data_smoother(virtual_machine_performance_category cat, vm_identifier_type const& vm_id, data_smoother_pointer const& p_smoother)
	{
		// pre: p_estimator != null
		DCS_ASSERT(p_smoother,
				   DCS_EXCEPTION_THROW(::std::invalid_argument, "Invalid pointer to data smoother"));

		vm_smoothers_[cat][vm_id] = p_smoother;
	}

	public: data_smoother_type& data_smoother(virtual_machine_performance_category cat, vm_identifier_type const& vm_id)
	{
		// pre: exists(vm_smoothers_[cat])
		DCS_ASSERT(vm_smoothers_.count(cat) > 0,
				   DCS_EXCEPTION_THROW(::std::invalid_argument,
									   "Invalid category for data smoothers"));
		// pre: exists(vm_smoothers_[cat][vm_id]) && vm_smoothers_[cat][vm_id] != null
		DCS_ASSERT(vm_smoothers_.at(cat).count(vm_id) > 0 && vm_smoothers_.at(cat).at(vm_id),
				   DCS_EXCEPTION_THROW(::std::invalid_argument,
									   "Invalid category for data smoothers"));

		return *(vm_smoothers_[cat][vm_id]);
	}

	public: data_smoother_type const& data_smoother(virtual_machine_performance_category cat, vm_identifier_type const& vm_id) const
	{
		// pre: exists(vm_smoothers_[cat])
		DCS_ASSERT(vm_smoothers_.count(cat) > 0,
				   DCS_EXCEPTION_THROW(::std::invalid_argument,
									   "Invalid category for data smoothers"));
		// pre: exists(vm_smoothers_[cat][vm_id]) && vm_smoothers_[cat][vm_id] != null
		DCS_ASSERT(vm_smoothers_.at(cat).count(vm_id) > 0 && vm_smoothers_.at(cat).at(vm_id),
				   DCS_EXCEPTION_THROW(::std::invalid_argument,
									   "Invalid category for data smoothers"));

		return *(vm_smoothers_.at(cat).at(vm_id));
	}

	public: ::std::vector<application_performance_category> target_metrics() const
	{
		typedef typename ::std::map<application_performance_category,real_type>::const_iterator iterator;

		::std::vector<application_performance_category> metrics;
		const iterator end_it = target_values_.end();
		for (iterator it = target_values_.begin();
			 it != end_it;
			 ++it)
		{
			metrics.push_back(it->first);
		}

		return metrics;
	}

	public: void target_value(application_performance_category cat, real_type val)
	{
		target_values_[cat] = val;
	}
 
	public: real_type target_value(application_performance_category cat) const
	{
		DCS_ASSERT(target_values_.count(cat),
				   DCS_EXCEPTION_THROW(::std::invalid_argument,
									   "Invalid category for target value"));

		return target_values_.at(cat);
	}

	public: template <typename FuncT>
			void add_on_sample_handler(FuncT f)
	{
		p_smp_sig_->connect(f);
	}

	public: template <typename FuncT>
			void add_on_control_handler(FuncT f)
	{
		p_ctl_sig_->connect(f);
	}

	public: template <typename FuncT>
			void add_on_reset_handler(FuncT f)
	{
		p_rst_sig_->connect(f);
	}

	public: void reset()
	{
		DCS_ASSERT(p_app_,
				   DCS_EXCEPTION_THROW(::std::runtime_error,
									   "Application is not set"));

		this->do_reset();

		// Reset app estimators
		const typename app_data_estimator_map::iterator app_est_end_it = app_estimators_.end();
		for (typename app_data_estimator_map::iterator it = app_estimators_.begin();
			 it != app_est_end_it;
			 ++it)
		{
			it->second->reset();
		}

		// Reset VM estimators
		const typename vm_data_estimator_map::iterator vm_est_end_it = vm_estimators_.end();
		for (typename vm_data_estimator_map::iterator it = vm_estimators_.begin();
			 it != vm_est_end_it;
			 ++it)
		{
			const typename vm_data_estimator_map::mapped_type::iterator vm_end_it = it->second.end();
			for (typename vm_data_estimator_map::mapped_type::iterator vm_it = it->second.begin();
				 vm_it != vm_end_it;
				 ++vm_it)
			{
				vm_it->second->reset();
			}
		}

		// Reset app smoothers
		const typename app_data_smoother_map::iterator app_smo_end_it = app_smoothers_.end();
		for (typename app_data_smoother_map::iterator it = app_smoothers_.begin();
			 it != app_smo_end_it;
			 ++it)
		{
			it->second->reset();
		}

		// Reset VM smoothers
		const typename vm_data_smoother_map::iterator vm_smo_end_it = vm_smoothers_.end();
		for (typename vm_data_smoother_map::iterator it = vm_smoothers_.begin();
			 it != vm_smo_end_it;
			 ++it)
		{
			const typename vm_data_smoother_map::mapped_type::iterator vm_end_it = it->second.end();
			for (typename vm_data_smoother_map::mapped_type::iterator vm_it = it->second.begin();
				 vm_it != vm_end_it;
				 ++vm_it)
			{
				vm_it->second->reset();
			}
		}

		// Emit signal
		(*p_rst_sig_)(*this);
	}

	public: void sample()
	{
		this->do_sample();

		// Emit signal
		(*p_smp_sig_)(*this);
	}

	public: void control()
	{
		this->do_control();

		// Emit signal
		(*p_ctl_sig_)(*this);
	}

	protected: app_pointer app_ptr()
	{
		return p_app_;
	}

	protected: app_pointer app_ptr() const
	{
		return p_app_;
	}

	protected: app_data_estimator_map& app_data_estimators()
	{
		return app_estimators_;
	}

	protected: app_data_estimator_map const& app_data_estimators() const
	{
		return app_estimators_;
	}

	protected: app_data_smoother_map& app_data_smoothers()
	{
		return app_smoothers_;
	}

	protected: app_data_smoother_map const& app_data_smoothers() const
	{
		return app_smoothers_;
	}

	protected: target_value_map& target_values()
	{
		return target_values_;
	}

	protected: target_value_map const& target_values() const
	{
		return target_values_;
	}

	private: virtual void do_reset() = 0;

	private: virtual void do_sample() = 0;

	private: virtual void do_control() = 0;


	private: real_type ts_; ///< Sampling time (in ms)
	private: real_type tc_; ///< Control time (in ms)
	private: app_pointer p_app_; ///< Pointer to the managed application
	private: target_value_map target_values_; ///< Mapping between application performance categories and target values
	private: app_data_estimator_map app_estimators_; ///< Mapping between application performance categories and data estimator pointers
	private: vm_data_estimator_map vm_estimators_; ///< Mapping between VM performance categories and data estimator pointers
	private: app_data_smoother_map app_smoothers_; ///< Mapping between application performance categories and data smoother pointers
	private: vm_data_smoother_map vm_smoothers_; ///< Mapping between VM performance categories and data smoother pointers
	private: signal_pointer p_rst_sig_; ///< Signal emitter for reset event
	private: signal_pointer p_smp_sig_; ///< Signal emitter for sample event
	private: signal_pointer p_ctl_sig_; ///< Signal emitter for control event
};

}} // Namespace dcs::testbed

#endif // DCS_TESTBED_BASE_APPLICATION_MANAGER_HPP
