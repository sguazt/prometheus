/**
 * \file dcs/testbed/dummy_application_manager.hpp
 *
 * \brief A 'do-nothing' Application Manager component.
 *
 * \author Marco Guazzone (marco.guazzone@gmail.com)
 *
 * <hr/>
 *
 * Copyright 2014 Marco Guazzone (marco.guazzone@gmail.com)
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

#ifndef DCS_TESTBED_DUMMY_APPLICATION_MANAGER_HPP
#define DCS_TESTBED_DUMMY_APPLICATION_MANAGER_HPP


#include <boost/smart_ptr.hpp>
#include <cstddef>
#include <dcs/debug.hpp>
#include <dcs/exception.hpp>
#include <dcs/testbed/application_performance_category.hpp>
#include <dcs/testbed/base_application_manager.hpp>
#include <fstream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <vector>


namespace dcs { namespace testbed {

/**
 * \brief A do-nothing application manager.
 *
 * This application does not act on the application.
 * It only observes the behavior of the monitored application, together with
 * its resource usage, and dump statistics.
 *
 * \author Marco Guazzone (marco.guazzone@gmail.com)
 */
template <typename TraitsT>
class dummy_application_manager: public base_application_manager<TraitsT>
{
	private: typedef base_application_manager<TraitsT> base_type;
	public: typedef typename base_type::traits_type traits_type;
	public: typedef typename traits_type::real_type real_type;
	private: typedef typename base_type::app_type app_type;
	private: typedef typename base_type::app_pointer app_pointer;
	private: typedef typename base_type::vm_identifier_type vm_identifier_type;
	private: typedef typename app_type::sensor_type sensor_type;
	private: typedef typename app_type::sensor_pointer sensor_pointer;
	private: typedef ::std::map<application_performance_category,sensor_pointer> out_sensor_map;
	private: typedef std::map<virtual_machine_performance_category,std::map<vm_identifier_type,sensor_pointer> > in_sensor_map;


	private: static const real_type default_sampling_time;
	private: static const real_type default_control_time;


	public: dummy_application_manager()
	: beta_(0.9),
	  ctl_count_(0),
      ctl_skip_count_(0),
      ctl_fail_count_(0)
	{
	}

	public: void smoothing_factor(real_type value)
	{
		beta_ = value;
	}

	public: real_type smoothing_factor() const
	{
		return beta_;
	}

	public: void export_data_to(::std::string const& fname)
	{
		dat_fname_ = fname;
	}

	private: void do_reset()
	{
		typedef typename base_type::target_value_map::const_iterator target_iterator;
		typedef typename app_type::vm_pointer vm_pointer;

		const ::std::vector<vm_pointer> vms = this->app().vms();
		const std::size_t nvms = this->app().num_vms();

		// Reset output sensors
		out_sensors_.clear();
		for (target_iterator tgt_it = this->target_values().begin(),
							 tgt_end_it = this->target_values().end();
			 tgt_it != tgt_end_it;
			 ++tgt_it)
		{
			const application_performance_category cat = tgt_it->first;

			out_sensors_[cat] = this->app().sensor(cat);
		}

		// Reset input sensors
		in_sensors_.clear();
		for (::std::size_t i = 0; i < nvms; ++i)
		{
			vm_pointer p_vm = vms[i];

			in_sensors_[cpu_util_virtual_machine_performance][p_vm->id()] = p_vm->sensor(cpu_util_virtual_machine_performance);
			in_sensors_[memory_util_virtual_machine_performance][p_vm->id()] = p_vm->sensor(memory_util_virtual_machine_performance);
		}

		// Reset resource utilization smoothers
		for (::std::size_t i = 0; i < nvms; ++i)
		{
			this->data_smoother(cpu_util_virtual_machine_performance, vms[i]->id(), ::boost::make_shared< testbed::brown_single_exponential_smoother<real_type> >(beta_));
			this->data_smoother(memory_util_virtual_machine_performance, vms[i]->id(), ::boost::make_shared< testbed::brown_single_exponential_smoother<real_type> >(beta_));
		}

		// Reset counters
		ctl_count_ = ctl_skip_count_
				   = ctl_fail_count_
				   = 0;

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

			*p_dat_ofs_ << "\"ts\"";

			const ::std::size_t nvms = this->app().num_vms();
			for (::std::size_t i = 0; i < nvms; ++i)
			{
				*p_dat_ofs_ << ",\"CPUCap_{" << i << "}(k)\",\"CPUShare_{" << i << "}(k)\""
							<< ",\"MemCap_{" << i << "}(k)\",\"MemShare_{" << i << "}(k)\"";
			}
			for (::std::size_t i = 0; i < nvms; ++i)
			{
				*p_dat_ofs_ << ",\"CPUShare_{" << vms[i]->id() << "}(k-1)\",\"MemShare_{" << vms[i]->id() << "}(k-1)\"";
			}
			for (::std::size_t i = 0; i < nvms; ++i)
			{
				*p_dat_ofs_ << ",\"CPUUtil_{" << vms[i]->id() << "}(k-1)\",\"MemUtil_{" << vms[i]->id() << "}(k-1)\"";
			}
			for (target_iterator tgt_it = this->target_values().begin(),
								 tgt_end_it = this->target_values().end();
				 tgt_it != tgt_end_it;
				 ++tgt_it)
			{
				const application_performance_category cat = tgt_it->first;

				*p_dat_ofs_ << ",\"ReferenceOutput_{" << cat << "}(k-1)\",\"MeasuredOutput_{" << cat << "}(k-1)\",\"RelativeOutputError_{" << cat << "}(k-1)\"";
			}
			*p_dat_ofs_ << ",\"# Controls\",\"# Skip Controls\",\"# Fail Controls\"";
            *p_dat_ofs_ << ",\"Elapsed Time\"";
			*p_dat_ofs_ << ::std::endl;
		}
	}

	private: void do_sample()
	{
		typedef typename in_sensor_map::const_iterator in_sensor_iterator;
		typedef typename out_sensor_map::const_iterator out_sensor_iterator;
		typedef ::std::vector<typename sensor_type::observation_type> obs_container;
		typedef typename obs_container::const_iterator obs_iterator;

		DCS_DEBUG_TRACE("(" << this << ") BEGIN Do SAMPLE - Count: " << ctl_count_ << "/" << ctl_skip_count_ << "/" << ctl_fail_count_);

		// Collect input values
		for (in_sensor_iterator in_sens_it = in_sensors_.begin(),
								in_sens_end_it = in_sensors_.end();
			 in_sens_it != in_sens_end_it;
			 ++in_sens_it)
		{
			const virtual_machine_performance_category cat = in_sens_it->first;

			for (typename in_sensor_map::mapped_type::const_iterator vm_it = in_sens_it->second.begin(),
																	 vm_end_it = in_sens_it->second.end();
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
						//this->data_estimator(cat, vm_id).collect(it->value());
						this->data_smoother(cat, vm_id).smooth(it->value());
					}
				}
			}
		}

		// Collect output values
		for (out_sensor_iterator out_sens_it = out_sensors_.begin(),
								 out_sens_end_it = out_sensors_.end();
			 out_sens_it != out_sens_end_it;
			 ++out_sens_it)
		{
			const application_performance_category cat = out_sens_it->first;

			sensor_pointer p_sens = out_sens_it->second;

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
				}
			}
		}

        DCS_DEBUG_TRACE("(" << this << ") END Do SAMPLE - Count: " << ctl_count_ << "/" << ctl_skip_count_ << "/" << ctl_fail_count_);
	}

    private: void do_control()
    {
		typedef typename base_type::target_value_map::const_iterator target_iterator;
		typedef typename app_type::vm_pointer vm_pointer;

		DCS_DEBUG_TRACE("(" << this << ") BEGIN Do CONTROL - Count: " << ctl_count_ << "/" << ctl_skip_count_ << "/" << ctl_fail_count_);

        boost::timer::cpu_timer cpu_timer;

		++ctl_count_;

		bool skip_ctl = false;

		::std::map<virtual_machine_performance_category,::std::vector<real_type> > xshares;
		::std::map<virtual_machine_performance_category,::std::vector<real_type> > xutils;
		::std::map<application_performance_category,real_type> perf_errs;

		::std::vector<vm_pointer> vms = this->app().vms();
		const ::std::size_t nvms = vms.size();

		for (::std::size_t i = 0; i < nvms; ++i)
		{
			const virtual_machine_performance_category cat = cpu_util_virtual_machine_performance;
			const vm_pointer p_vm = vms[i];

			const real_type uh = this->data_smoother(cat, p_vm->id()).forecast(0);
			const real_type c = p_vm->cpu_share();

			xshares[cat].push_back(c);
			xutils[cat].push_back(uh);
DCS_DEBUG_TRACE("VM " << p_vm->id() << " - Performance Category: " << cat << " - Uhat(k): " << uh << " - C(k): " << c);//XXX
		}

		if (!skip_ctl)
		{
			for (target_iterator tgt_it = this->target_values().begin(),
							 	 tgt_end_it = this->target_values().end();
				 tgt_it != tgt_end_it;
				 ++tgt_it)
			{
				const application_performance_category cat(tgt_it->first);

				// Compute a summary statistics of collected observation
				if (this->data_estimator(cat).count() > 0)
				{
					const real_type yh = this->data_estimator(cat).estimate();
					const real_type yr = this->target_value(cat);

					switch (cat)
					{
						case response_time_application_performance:
							perf_errs[cat] = (yr-yh)/yr;
							break;
						case throughput_application_performance:
							perf_errs[cat] = (yh-yr)/yr;
							break;
					}
DCS_DEBUG_TRACE("APP Performance Category: " << cat << " - Yhat(k): " << yh << " - R: " << yr << " -> RelatiiveError(k+1): " << perf_errs.at(cat));//XXX
				}
				else
				{
					// No observation collected during the last control interval
					DCS_DEBUG_TRACE("No output observation collected during the last control interval -> Skip control");
					skip_ctl = true;
					break;
				}
#ifdef DCSXX_TESTBED_EXP_APP_MGR_RESET_ESTIMATION_EVERY_INTERVAL
				this->data_estimator(cat).reset();
#endif // DCSXX_TESTBED_EXP_APP_MGR_RESET_ESTIMATION_EVERY_INTERVAL
			}
		}
        if (skip_ctl)
		{
			++ctl_skip_count_;
		}

        cpu_timer.stop();

		// Export to file
		if (p_dat_ofs_)
		{
			if (xshares.size() == 0)
			{
				for (::std::size_t i = 0; i < nvms; ++i)
				{
					const vm_pointer p_vm = vms[i];

					// check: p_vm != null
					DCS_DEBUG_ASSERT( p_vm );

					xshares[cpu_util_virtual_machine_performance].push_back(p_vm->cpu_share());
					xshares[memory_util_virtual_machine_performance].push_back(p_vm->memory_share());
				}
			}
			if (xutils.size() == 0)
			{
				xutils[cpu_util_virtual_machine_performance].assign(nvms, std::numeric_limits<real_type>::quiet_NaN());
				xutils[memory_util_virtual_machine_performance].assign(nvms, std::numeric_limits<real_type>::quiet_NaN());
			}

			*p_dat_ofs_ << ::std::time(0) << ",";
			for (::std::size_t i = 0; i < nvms; ++i)
			{
				const vm_pointer p_vm = vms[i];

				// check: p_vm != null
				DCS_DEBUG_ASSERT( p_vm );

				if (i != 0)
				{
					*p_dat_ofs_ << ",";
				}
				*p_dat_ofs_ << p_vm->cpu_cap() << "," << p_vm->cpu_share()
							<< "," << p_vm->memory_cap() << "," << p_vm->memory_share();
			}
			*p_dat_ofs_ << ",";
            for (std::size_t i = 0; i < nvms; ++i)
            {
                if (i != 0)
                {
                    *p_dat_ofs_ << ",";
                }
                *p_dat_ofs_ << xshares.at(cpu_util_virtual_machine_performance)[i]
                            << "," << xshares.at(memory_util_virtual_machine_performance)[i];
            }
			*p_dat_ofs_ << ",";
            for (std::size_t i = 0; i < nvms; ++i)
            {
                const vm_pointer p_vm = vms[i];

                // check: p_vm != null
                DCS_DEBUG_ASSERT( p_vm );

                if (i != 0)
                {
                    *p_dat_ofs_ << ",";
                }

				*p_dat_ofs_ << xutils.at(cpu_util_virtual_machine_performance)[i]
							<< "," << xutils.at(memory_util_virtual_machine_performance)[i];
            }
            *p_dat_ofs_ << ",";
			for (target_iterator tgt_it = this->target_values().begin(),
								 tgt_end_it = this->target_values().end();
				 tgt_it != tgt_end_it;
				 ++tgt_it)
			{
				const application_performance_category cat = tgt_it->first;

				if (tgt_it != this->target_values().begin())
				{
					*p_dat_ofs_ << ",";
				}
				const real_type yh = this->data_estimator(cat).estimate();
				const real_type yr = tgt_it->second;
				*p_dat_ofs_ << yr << "," << yh << "," << perf_errs.at(cat);
			}
			*p_dat_ofs_ << "," << ctl_count_ << "," << ctl_skip_count_ << "," << ctl_fail_count_;
            *p_dat_ofs_ << "," << (cpu_timer.elapsed().user+cpu_timer.elapsed().system);
			*p_dat_ofs_ << ::std::endl;
		}

		DCS_DEBUG_TRACE("(" << this << ") END Do CONTROL - Count: " << ctl_count_ << "/" << ctl_skip_count_ << "/" << ctl_fail_count_);
	}


	private: real_type beta_; ///< The EWMA smoothing factor for resource utilization
	private: ::std::size_t ctl_count_; ///< Number of times control function has been invoked
	private: ::std::size_t ctl_skip_count_; ///< Number of times control has been skipped
	private: ::std::size_t ctl_fail_count_; ///< Number of times control has failed
	private: in_sensor_map in_sensors_;
	private: out_sensor_map out_sensors_;
	private: ::std::string dat_fname_;
	private: ::boost::shared_ptr< ::std::ofstream > p_dat_ofs_;
}; // dummy_application_manager

}} // Namespace dcs::testbed

#endif // DCS_TESTBED_DUMMY_APPLICATION_MANAGER_HPP
