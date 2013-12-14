/**
 * \file dcs/testbed/base_experiment_tracker.hpp
 *
 * \brief Base class for experiment trackers
 *
 * \author Marco Guazzone (marco.guazzone@gmail.com)
 *
 * <hr/>
 *
 * Copyright (C) 2013       Marco Guazzone (marco.guazzone@gmail.com)
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

#ifndef DCS_TESTBED_BASE_EXPERIMENT_TRACKER_HPP
#define DCS_TESTBED_BASE_EXPERIMENT_TRACKER_HPP


#include <boost/bind.hpp>
#include <boost/smart_ptr.hpp>
#include <dcs/assert.hpp>
#include <dcs/exception.hpp>
#include <dcs/testbed/application_experiment.hpp>
#include <dcs/testbed/system_experiment.hpp>
#include <stdexcept>


namespace dcs { namespace testbed {

/// Base class for experiment trackers
template <typename TraitsT>
class base_experiment_tracker
{
	private: typedef base_experiment_tracker self_type;
	public: typedef TraitsT traits_type;
	public: typedef typename traits_type::real_type real_type;
	public: typedef system_experiment<traits_type> sys_experiment_type;
	public: typedef ::boost::shared_ptr<sys_experiment_type> sys_experiment_pointer;
	public: typedef application_experiment<traits_type> app_experiment_type;
	public: typedef ::boost::shared_ptr<app_experiment_type> app_experiment_pointer;


	public: virtual ~base_experiment_tracker()
	{
	}

	public: void track(sys_experiment_type& exp)
	{
		exp.add_on_start_handler(::boost::bind(&self_type::on_start, this, ::_1));
		::std::vector<app_experiment_pointer> app_exps(exp.experiments());
		for (::std::size_t i = 0; i < app_exps.size(); ++i)
		{
			app_experiment_pointer p_app_exp(app_exps[i]);
			p_app_exp->add_on_start_handler(::boost::bind(&self_type::on_app_start, this, ::_1));
			p_app_exp->manager().add_on_sample_handler(::boost::bind(&self_type::on_app_sample, this, *p_app_exp, ::_1));
			p_app_exp->manager().add_on_control_handler(::boost::bind(&self_type::on_app_control, this, *p_app_exp, ::_1));
			p_app_exp->add_on_stop_handler(::boost::bind(&self_type::on_app_stop, this, ::_1));
		}
		exp.add_on_stop_handler(::boost::bind(&self_type::on_stop, this, ::_1));
	}

	public: void reset()
	{
		this->do_reset();
	}

	private: void on_start(sys_experiment_type const& exp)
	{
		this->do_on_start(exp);
	}

	private: void on_app_start(app_experiment_type const& exp)
	{
		this->do_on_app_start(exp);
	}

	private: void on_app_sample(app_experiment_type const& exp, base_application_manager<traits_type> const&)
	{
		this->do_on_app_sample(exp);
	}

	private: void on_app_control(app_experiment_type const& exp, base_application_manager<traits_type> const&)
	{
		this->do_on_app_control(exp);
	}

	private: void on_app_stop(app_experiment_type const& exp)
	{
		this->do_on_app_stop(exp);
	}

	private: void on_stop(sys_experiment_type const& exp)
	{
		this->do_on_stop(exp);
	}

	private: virtual void do_reset() = 0;

	private: virtual void do_on_start(sys_experiment_type const& exp) = 0;

	private: virtual void do_on_app_start(app_experiment_type const& exp) = 0;

	private: virtual void do_on_app_sample(app_experiment_type const& exp) = 0;

	private: virtual void do_on_app_control(app_experiment_type const& exp) = 0;

	private: virtual void do_on_app_stop(app_experiment_type const& exp) = 0;

	private: virtual void do_on_stop(sys_experiment_type const& exp) = 0;
}; // base_experiment_tracker

/*
template <typename TraitsT>
class base_application_experiment_tracker: public base_experiment_tracker<TraitsT>
{
	private: base_experiment_tracker<TraitsT> base_type;
	public: typedef typename base_type::traits_type traits_type;
	private: typedef base_application<traits_type> app_type;
	public: typedef ::boost::weak_ptr<app_type> app_pointer;
//	private: typedef base_application_manager<traits_type> manager_type;
//	public: typedef ::boost::weak_ptr<manager_type> manager_pointer;
	public: typedef base_sensor<traits_type> sensor_type;
	public: typedef ::boost::shared_ptr<sensor_type> sensor_pointer;
	protected: typedef ::std::map< virtual_machine_performance_category, ::std::map<typename base_virtual_machine<traits_type>::identifier_type>,sensor_pointer> > vm_sensor_container;
	protected: typedef ::std::map<application_performance_category, sensor_pointer> app_sensor_container;


	public: base_application_experiment_tracker()
	: init_(true)
	{
	}

	public: virtual ~base_application_experiment_tracker()
	{
	}

	public: void app(app_pointer const& p_app)
	{
		p_app_ = p_app;
	}

	public: app_type& app()
	{
		return *p_app_;
	}

	public: app_type const& app() const
	{
		return *p_app_;
	}

//	private: void do_tracker()
//	{
//		typedef typename app_type::vm_pointer vm_pointer;
//		typedef ::std::vector<vm_pointer> vm_container;
//		typedef typename vm_container::const_iterator vm_iterator;
//
//		vm_container vms = p_app_->vms();
//		vm_iterator vm_end_it = vms.end();
//		for (vm_iterator vm_it = vms.begin();
//			 vm_it != vm_end_it;
//			 ++vm_it)
//		{
//			vm_sensors_[cpu_util_virtual_machine_performance][vm_it->id()].sense();
//		}
//
//	}

	protected: app_sensor_container& app_sensors()
	{
		return app_sensor_;
	}

	protected: app_sensor_container const& app_sensors() const
	{
		return app_sensor_;
	}

	protected: vm_sensor_container& vm_sensors()
	{
		return vm_sensor_;
	}

	protected: vm_sensor_container const& vm_sensors() const
	{
		return vm_sensor_;
	}

	private: void do_reset()
	{
		typedef typename app_type::vm_pointer vm_pointer;
		typedef ::std::vector<vm_pointer> vm_container;
		typedef typename vm_container::const_iterator vm_iterator;

		//FIXME: add methods to allow the user to choose what application performance and VM performance categories to tracker

		if (init_)
		{
			app_sensors_[response_time_application_performance] = p_app_->sensor(response_time_application_performance);
		}
		app_sensors_[response_time_application_performance]->reset();

		vm_container vms = p_app_->vms();
		vm_iterator vm_end_it = vms.end();
		for (vm_iterator vm_it = vms.begin();
			 vm_it != vm_end_it;
			 ++vm_it)
		{
			if (init_)
			{
				vm_sensors_[cpu_util_virtual_machine_performance][vm_it->id()] = vm_it->sensor(cpu_util_virtual_machine_performance);
			}
			vm_sensors_[cpu_util_virtual_machine_performance][vm_it->id()].reset();
		}

		if (init_)
		{
			init_ = false;
		}
	}


	private: app_pointer p_app_; ///< The trackered application
	private: vm_sensor_container vm_sensors_;
	private: app_sensor_container app_sensors_;
	private: bool init_;
}; // base_application_experiment_tracker

template <typename TraitsT>
class application_experiment_stats_gatherer: public base_application_experiment_tracker<TraitsT>
{
	private: base_application_experiment_tracker<TraitsT> base_type;
	public: typedef typename base_type::traits_type traits_type;
	private: typedef base_application<traits_type> app_type;
	public: typedef ::boost::weak_ptr<app_type> app_pointer;
//	private: typedef base_application_manager<traits_type> manager_type;
//	public: typedef ::boost::weak_ptr<manager_type> manager_pointer;
	public: typedef base_sensor<traits_type> sensor_type;
	public: typedef ::boost::shared_ptr<sensor_type> sensor_pointer;
	public: typedef base_estimator<traits_type> estimator_type;
	public: typedef ::boost::shared_ptr<estimator_type> estimator_pointer


	public: void add_estimator(application_performance_category cat, estimator_pointer const& p_est)
	{
		app_stats_[cat] = p_est;
	}

	public: void add_estimator(vm_type const& vm, virtual_machine_performance_category cat, estimator_pointer const& p_est)
	{
		vm_stats_[cat][vm.id()] = p_est;
	}

	private: void do_tracker()
	{
		typedef typename app_type::vm_pointer vm_pointer;
		typedef ::std::vector<vm_pointer> vm_container;
		typedef typename vm_container::const_iterator vm_iterator;

		app_sensor_container& app_sensors(this->app_sensors());
		vm_sensor_container& vm_sensors(this->vm_sensors());

		sensor_pointer p_sens;

		p_sens = app_sensors[response_time_application_performance];
		p_sens->sense();
		if (p_sens->has_observations())
		{
			::std::vector<real_type> obs(p_sens->observations());
			app_stats_[response_time_application_performance]->collect(obs.begin(), obs.end());
		}

		vm_container vms = this->app().vms();
		vm_iterator vm_end_it = vms.end();
		for (vm_iterator vm_it = vms.begin();
			 vm_it != vm_end_it;
			 ++vm_it)
		{
			p_sens = vm_sensors[cpu_util_virtual_machine_performance][vm_it->id()];
			p_sens->sense();
			if (p_sens->has_observations())
			{
				::std::vector<real_type> obs(p_sens->observations());
				vm_stats_[cpu_util_virtual_machine_performance][vm_it->id()]->collect(obs.begin(), obs.end());
			}
		}
	}


	private: ::std::map<application_performance_category,estimator_pointer> app_stats_;
	private: ::std::map<virtual_machine_performance_category,estimator_pointer> vm_stats_;
}; // application_experiment_stats_tracker
*/

}} // Namespace dcs::testbed


#endif // DCS_TESTBED_BASE_EXPERIMENT_TRACKER_HPP
