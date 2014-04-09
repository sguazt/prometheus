/**
 * \file dcs/testbed/sysid_application_manager.hpp
 *
 * \brief A special application manager that performs system identification.
 *
 * \author Marco Guazzone (marco.guazzone@gmail.com)
 *
 * <hr/>
 *
 * Copyright 2014   Marco Guazzone (marco.guazzone@gmail.com)
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

#ifndef DCS_TESTBED_SYSID_APPLICATION_MANAGER_HPP
#define DCS_TESTBED_SYSID_APPLICATION_MANAGER_HPP


#include <boost/smart_ptr.hpp>
#include <cstddef>
#include <ctime>
#include <dcs/assert.hpp>
#include <dcs/debug.hpp>
#include <dcs/exception.hpp>
#include <dcs/math/traits/float.hpp>
#include <dcs/testbed/application_performance_category.hpp>
#include <dcs/testbed/base_application_manager.hpp>
#include <dcs/testbed/base_signal_generator.hpp>
#include <dcs/testbed/virtual_machine_performance_category.hpp>
#include <fstream>
#include <limits>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>


namespace dcs { namespace testbed {

/**
 * \brief Application manager to perform system identification for an application.
 *
 * \tparam TraitsT Traits type
 *
 * \author Marco Guazzone (marco.guazzone@gmail.com)
 */
template <typename TraitsT>
class sysid_application_manager: public base_application_manager<TraitsT>
{
	private: typedef base_application_manager<TraitsT> base_type;
	public: typedef typename base_type::traits_type traits_type;
	public: typedef typename traits_type::real_type real_type;
	private: typedef typename base_type::app_type app_type;
	private: typedef typename base_type::app_pointer app_pointer;
	private: typedef typename base_type::vm_identifier_type vm_identifier_type;
	private: typedef typename app_type::sensor_type sensor_type;
	private: typedef typename app_type::sensor_pointer sensor_pointer;
	private: typedef ::std::vector<real_type> observation_container;
	private: typedef ::std::map<application_performance_category,observation_container> observation_map;
	private: typedef ::std::map<application_performance_category,sensor_pointer> app_sensor_map;
	private: typedef ::std::map<virtual_machine_performance_category,::std::map<vm_identifier_type,sensor_pointer> > vm_sensor_map;
	public: typedef base_signal_generator<real_type> signal_generator_type;
	public: typedef ::boost::shared_ptr<signal_generator_type> signal_generator_pointer;


	private: static const real_type default_sampling_time;
	private: static const real_type default_control_time;


	/// Default constructor.
	public: sysid_application_manager()
	: out_ext_fmt_(false)
	{
		this->sampling_time(default_sampling_time);
		this->control_time(default_control_time);
		//this->data_estimator(cpu_util_virtual_machine_performance, boost::make_shared< testbed::mean_estimator<real_type> >());
		//this->data_smoother(cpu_util_virtual_machine_performance, ::boost::make_shared< testbed::brown_single_exponential_smoother<real_type> >(beta_));
	}

	/// A constructor.
	public: explicit sysid_application_manager(signal_generator_pointer const& p_sig_gen)
	: p_sig_gen_(p_sig_gen),
	  out_ext_fmt_(false)
	{
		this->sampling_time(default_sampling_time);
		this->control_time(default_control_time);
		//this->data_estimator(cpu_util_virtual_machine_performance, boost::make_shared< testbed::mean_estimator<real_type> >());
		//this->data_smoother(cpu_util_virtual_machine_performance, ::boost::make_shared< testbed::brown_single_exponential_smoother<real_type> >(beta_));
	}

	/// Set the path of the output data file.
	public: void export_data_to(::std::string const& s)
	{
		// pre: s != ""
		DCS_ASSERT(!s.empty(),
				   DCS_EXCEPTION_THROW(::std::invalid_argument,
									   "Cannot use empty string as output data file name"));

		dat_fname_ = s;
	}

	/// Enabled or disable the extended format of the output data file.
	public: void output_extended_format(bool val)
	{
		out_ext_fmt_ = val;
	}

	public: template <typename IterT>
			void initial_shares(IterT first_share, IterT last_share)
	{
		init_shares_.assign(first_share, last_share);
	}

	private: void do_reset()
	{
		typedef typename base_type::target_value_map::const_iterator target_iterator;
		typedef typename app_type::vm_pointer vm_pointer;

		::std::vector<vm_pointer> vms = this->app().vms();
		const ::std::size_t nvms = vms.size();

		// Initialize initial shares
		for (::std::size_t i = init_shares_.size(); i < nvms; ++i)
		{
			init_shares_.push_back(1);
		}
		// Set initial shares and write output file header
		for (::std::size_t i = 0; i < nvms; ++i)
		{
			// Set share
			vms[i]->cpu_share(init_shares_[i]);
		}

		// Reset app perf sensors
		app_sensors_.clear();
		const target_iterator tgt_end_it = this->target_values().end();
		for (target_iterator tgt_it = this->target_values().begin();
			 tgt_it != tgt_end_it;
			 ++tgt_it)
		{
			const application_performance_category cat = tgt_it->first;

			app_sensors_[cat] = this->app().sensor(cat);
		}

		// Reset VM perf sensors
		vm_sensors_.clear();
		for (::std::size_t i = 0; i < nvms; ++i)
		{
			const virtual_machine_performance_category cat = cpu_util_virtual_machine_performance;
			const vm_pointer p_vm = vms[i];

			vm_sensors_[cat][p_vm->id()] = p_vm->sensor(cat);
		}

		// Reset counters
		ctl_count_ = ctl_skip_count_
				   = ctl_fail_count_
				   = 0;

		// Reset timers
		//::std::time(&t0_);
		t0_ = -1;

		// Reset estimators and smoothers
		//this->data_estimator(cpu_util_virtual_machine_performance).reset();
		//this->data_smoother(cpu_util_virtual_machine_performance).reset();
//		for (::std::size_t i = 0; i < nvms; ++i)
//		{
//			this->data_smoother(cpu_util_virtual_machine_performance, vms[i]->id(), ::boost::make_shared< testbed::brown_single_exponential_smoother<real_type> >(beta_));
//			//this->data_estimator(cpu_util_virtual_machine_performance, vms[i]->id(), ::boost::make_shared< testbed::mean_estimator<real_type> >());
//		}

		// Reset output data file
		if (p_dat_ofs_ && p_dat_ofs_->is_open())
		{
			p_dat_ofs_->close();
		}
		p_dat_ofs_.reset();
		if (!dat_fname_.empty())
		{
			p_dat_ofs_ = ::boost::make_shared< ::std::ofstream >(dat_fname_.c_str());
			if (!p_dat_ofs_->good())
			{
				::std::ostringstream oss;
				oss << "Cannot open output data file '" << dat_fname_ << "'";

				DCS_EXCEPTION_THROW(::std::runtime_error, oss.str());
			}

			// Write first part of header to output file
			*p_dat_ofs_ << "\"Sampling Time\"";

			for (::std::size_t i = 0; i < nvms; ++i)
			{
				const vm_pointer p_vm = vms[i];

				// Write VM-related stuff of header to output file
				*p_dat_ofs_ << ",\"" << p_vm->name() << " CPU Share\",\"" << p_vm->name() << " CPU Utilization\"";
			}

			// Write last part of header to output file
			*p_dat_ofs_ << ",\"Operation Time\",\"Operation Name\",\"Performance Index\",\"# Controls\",\"# Control Skips\",\"# Control Fails\",\"Entry Type\"" << ::std::endl;
		}
	}

	private: void do_sample()
	{
		typedef typename app_type::vm_pointer vm_pointer;
		typedef typename vm_sensor_map::const_iterator vm_sensor_iterator;
		typedef typename app_sensor_map::const_iterator app_sensor_iterator;
		typedef ::std::vector<typename sensor_type::observation_type> obs_container;
		typedef typename obs_container::const_iterator obs_iterator;

		DCS_DEBUG_TRACE("(" << this << ") BEGIN Do SAMPLE - Count: " << ctl_count_ << "/" << ctl_skip_count_ << "/" << ctl_fail_count_);

		const ::std::vector<vm_pointer> vms = this->app().vms();
		const ::std::size_t nvms = vms.size();

		::std::map<vm_identifier_type,obs_container> vm_obs;
		obs_container app_obs;

		::std::size_t max_nobs = 0;

		// Compute the elapsed time
		::std::time_t ts = -1;
		::std::time(&ts);

		// Collect VM perf measures
		const vm_sensor_iterator vm_sens_end_it = vm_sensors_.end();
		for (vm_sensor_iterator vm_sens_it = vm_sensors_.begin();
			 vm_sens_it != vm_sens_end_it;
			 ++vm_sens_it)
		{
			const virtual_machine_performance_category cat = vm_sens_it->first;

//			const ::std::size_t n = vm_sens_it->second.size();
//			for (::std::size_t i = 0; i < n; ++i)
//			{
//				sensor_pointer p_sens = vm_sens_it->second.at(i);
            const typename vm_sensor_map::mapped_type::const_iterator vm_end_it = vm_sens_it->second.end();
            for (typename vm_sensor_map::mapped_type::const_iterator vm_it = vm_sens_it->second.begin();
                 vm_it != vm_end_it;
                 ++vm_it)
            {
                const vm_identifier_type vm_id = vm_it->first;
                sensor_pointer p_sens = vm_it->second;

				// check: p_sens != null
				DCS_DEBUG_ASSERT( p_sens );

				p_sens->sense();
				if (p_sens->has_observations())
				{
					const obs_container obs = p_sens->observations();
					const obs_iterator end_it = obs.end();
					for (obs_iterator it = obs.begin();
						 it != end_it;
						 ++it)
					{
						this->data_estimator(cat, vm_id).collect(it->value());

						if (out_ext_fmt_)
						{
							vm_obs[vm_id].push_back(*it);
						}
					}

					max_nobs = ::std::max(max_nobs, obs.size());
				}
			}
		}

		// Collect app perf measures
		const app_sensor_iterator app_sens_end_it = app_sensors_.end();
		for (app_sensor_iterator app_sens_it = app_sensors_.begin();
			 app_sens_it != app_sens_end_it;
			 ++app_sens_it)
		{
			const application_performance_category cat = app_sens_it->first;

			sensor_pointer p_sens = app_sens_it->second;

			// check: p_sens != null
			DCS_DEBUG_ASSERT( p_sens );

			p_sens->sense();
			if (p_sens->has_observations())
			{
				const obs_container obs = p_sens->observations();
				const obs_iterator end_it = obs.end();
				for (obs_iterator it = obs.begin();
					 it != end_it;
					 ++it)
				{
					this->data_estimator(cat).collect(it->value());

					if (out_ext_fmt_)
					{
						app_obs.push_back(*it);
					}
				}

				max_nobs = ::std::max(max_nobs, obs.size());
			}
		}

		// Write output data
		if (p_dat_ofs_ && out_ext_fmt_)
		{
			::std::ostringstream oss;

			// Cache VM shares (they will not change until the next control interval)
			::std::map<vm_identifier_type,real_type> vm_shares;
			for (::std::size_t j = 0; j < nvms; ++j)
			{
				const vm_pointer p_vm = vms[j];
				vm_shares[p_vm->id()] = p_vm->cpu_share();
			}

			for (::std::size_t i = 0; i < max_nobs; ++i)
			{
				*p_dat_ofs_ << ts;
				// Write VM data
				for (::std::size_t j = 0; j < nvms; ++j)
				{
					const vm_pointer p_vm = vms[j];
					const obs_container& obs = vm_obs.at(p_vm->id());
					const ::std::size_t nobs = obs.size();
					const real_type share = vm_shares.at(p_vm->id());

					if (i < nobs)
					{
						*p_dat_ofs_ << "," << share << "," << obs[i].value();
					}
					else if (nobs > 0)
					{
						*p_dat_ofs_ << "," << share << "," << obs.back().value();
					}
					else
					{
						*p_dat_ofs_ << ",na,na";
					}
				}
				// Write App data
				const ::std::size_t nobs = app_obs.size();
				if (i < nobs)
				{
					*p_dat_ofs_ << "," << app_obs[i].timestamp() << "," << "\"" << app_obs[i].label() << "\"" << "," << app_obs[i].value();
				}
				else if (nobs > 0)
				{
					*p_dat_ofs_ << "," << app_obs.back().timestamp() << "," << "\"" << app_obs.back().label() << "\"" << "," << app_obs.back().value();
				}
				else
				{
					*p_dat_ofs_ << ",na,na,na";
				}

				*p_dat_ofs_ << "," << "\"na\",\"na\",\"na\""; // Control counters not available here
				*p_dat_ofs_ << "," << "\"[DATA]\"" << ::std::endl;
			}
		}

		DCS_DEBUG_TRACE("(" << this << ") END Do SAMPLE - Count: " << ctl_count_ << "/" << ctl_skip_count_ << "/" << ctl_fail_count_);
	}

	private: void do_control()
	{
		typedef typename app_type::vm_pointer vm_pointer;
		typedef typename signal_generator_type::vector_type share_container;
		typedef typename base_type::target_value_map::const_iterator target_iterator;

		DCS_DEBUG_TRACE("(" << this << ") BEGIN Do CONTROL - Count: " << ctl_count_ << "/" << ctl_skip_count_ << "/" << ctl_fail_count_);

		++ctl_count_;

		// Compute the elapsed time
		::std::time_t ts = -1;
		::std::time(&ts);

	   ::std::vector<vm_pointer> vms = this->app().vms();
		const ::std::size_t nvms = vms.size();

		// Set shares according to the given signal

		// Generate new shares
		share_container old_shares(nvms);
		share_container new_shares = (*p_sig_gen_)();

		// check: consistency
		DCS_DEBUG_ASSERT( new_shares.size() == nvms );

		//DCS_DEBUG_TRACE( "   Generated shares: " << dcs::debug::to_string(new_shares.begin(), new_shares.end()) );

		// Set new shares to every VM
		for (::std::size_t i = 0; i < nvms; ++i)
		{
			vm_pointer p_vm = vms[i];

			// check: not null
			DCS_DEBUG_ASSERT( p_vm );

			old_shares[i] = p_vm->cpu_share();
			if (!::dcs::math::float_traits<real_type>::essentially_equal(old_shares[i], new_shares[i]))
			{
				p_vm->cpu_share(new_shares[i]);
			}

			DCS_DEBUG_TRACE( "   VM '" << p_vm->name() << "' :: Old CPU share: " << old_shares[i] << " :: New CPU share: " << new_shares[i] );
		}

		// Write output data
		if (p_dat_ofs_)
		{
			*p_dat_ofs_ << ts;
			for (::std::size_t i = 0; i < nvms; ++i)
			{
				const virtual_machine_performance_category cat = cpu_util_virtual_machine_performance;
				const vm_pointer p_vm = vms[i];

				*p_dat_ofs_ << "," << old_shares[i];

				if (this->data_estimator(cat, p_vm->id()).count() > 0)
				{
					*p_dat_ofs_ << "," << this->data_estimator(cat, p_vm->id()).estimate();
				}
				else
				{
					*p_dat_ofs_ << ",na";
				}

				this->data_estimator(cat, p_vm->id()).reset();
			}
            const target_iterator tgt_end_it = this->target_values().end();
            for (target_iterator tgt_it = this->target_values().begin();
                 tgt_it != tgt_end_it;
                 ++tgt_it)
            {
                const application_performance_category cat = tgt_it->first;

				if (this->data_estimator(cat).count() > 0)
				{
					*p_dat_ofs_ << "," << this->data_estimator(cat).estimate();
				}
				else
				{
					*p_dat_ofs_ << ",na";
				}
//				*p_dat_ofs_ << "," << tgt_it->second;

#ifdef DDCS_TESTBED_EXP_RESET_ESTIMATION_EVERY_INTERVAL
				this->data_estimator(cat).reset();
#endif // DDCS_TESTBED_EXP_RESET_ESTIMATION_EVERY_INTERVAL
			}
			*p_dat_ofs_ << "," << ctl_count_ << "," << ctl_skip_count_ << "," << ctl_fail_count_;
			if (out_ext_fmt_)
			{
				*p_dat_ofs_ << ",\"[SUMMARY]\"";
			}
			*p_dat_ofs_ << ::std::endl;
		}

		DCS_DEBUG_TRACE("(" << this << ") END Do CONTROL - Count: " << ctl_count_ << "/" << ctl_skip_count_ << "/" << ctl_fail_count_);
	}


	private: signal_generator_pointer p_sig_gen_; ///< Ptr to signal generator used to excite VMs
	private: ::std::string dat_fname_; ///< The path to the output data file
	private: ::boost::shared_ptr< ::std::ofstream > p_dat_ofs_; ///< The output stream for the output data file
	private: bool out_ext_fmt_; ///< Flag to control whether to produce an output data file with extended format
	private: ::std::size_t ctl_count_; ///< Number of times control function has been invoked
	private: ::std::size_t ctl_skip_count_; ///< Number of times control has been skipped
	private: ::std::size_t ctl_fail_count_; ///< Number of times control has failed
	private: ::std::vector<real_type> init_shares_; ///< The initial shares to assign to each VM
	private: vm_sensor_map vm_sensors_;
	private: app_sensor_map app_sensors_;
	private: ::std::time_t t0_;
}; // sysid_application_manager

template <typename T>
const typename sysid_application_manager<T>::real_type sysid_application_manager<T>::default_sampling_time = 1;

template <typename T>
const typename sysid_application_manager<T>::real_type sysid_application_manager<T>::default_control_time = 5;

}} // Namespace dcs::testbed

#endif // DCS_TESTBED_SYSID_APPLICATION_MANAGER_HPP
