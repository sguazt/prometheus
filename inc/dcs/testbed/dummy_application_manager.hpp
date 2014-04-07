/**
 * \file dcs/testbed/dummy_application_manager.hpp
 *
 * \brief A 'do-nothing' Application Manager component.
 *
 * \author Marco Guazzone (marco.guazzone@gmail.com)
 *
 * <hr/>
 *
 * Copyright (C) 2014       Marco Guazzone (marco.guazzone@gmail.com)
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

template <typename TraitsT>
class dummy_application_manager: public base_application_manager<TraitsT>
{
	private: typedef base_application_manager<TraitsT> base_type;
	public: typedef typename base_type::traits_type traits_type;
	public: typedef typename traits_type::real_type real_type;
	private: typedef typename base_type::app_type app_type;
	private: typedef typename base_type::app_pointer app_pointer;
	private: typedef typename app_type::sensor_type sensor_type;
	private: typedef typename app_type::sensor_pointer sensor_pointer;
	private: typedef ::std::map<application_performance_category,sensor_pointer> out_sensor_map;


	private: static const real_type default_sampling_time;
	private: static const real_type default_control_time;


	public: dummy_application_manager()
	: ctl_count_(0),
      ctl_skip_count_(0),
      ctl_fail_count_(0)
	{
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

		// Reset output sensors
		out_sensors_.clear();
		const target_iterator tgt_end_it = this->target_values().end();
		for (target_iterator tgt_it = this->target_values().begin();
			 tgt_it != tgt_end_it;
			 ++tgt_it)
		{
			const application_performance_category cat = tgt_it->first;

			out_sensors_[cat] = this->app().sensor(cat);
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
				*p_dat_ofs_ << ",\"Cap_{" << i << "}\",\"Share_{" << i << "}\"";
			}
			for (target_iterator tgt_it = this->target_values().begin();
				 tgt_it != tgt_end_it;
				 ++tgt_it)
			{
				const application_performance_category cat = tgt_it->first;

				*p_dat_ofs_ << ",\"y_{" << cat << "}\",\"yn_{" << cat << "}\",\"r_{" << cat << "}\"";
			}
			*p_dat_ofs_ << ",\"# Controls\",\"# Skip Controls\",\"# Fail Controls\"";
			*p_dat_ofs_ << ::std::endl;
		}
	}

	private: void do_sample()
	{
		typedef typename out_sensor_map::const_iterator out_sensor_iterator;
		typedef ::std::vector<typename sensor_type::observation_type> obs_container;
		typedef typename obs_container::const_iterator obs_iterator;

		DCS_DEBUG_TRACE("(" << this << ") BEGIN Do SAMPLE - Count: " << ctl_count_ << "/" << ctl_skip_count_ << "/" << ctl_fail_count_);

		// Collect output values
		const out_sensor_iterator out_sens_end_it = out_sensors_.end();
		for (out_sensor_iterator out_sens_it = out_sensors_.begin();
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

		++ctl_count_;

		bool skip_ctl = false;

		::std::map<virtual_machine_performance_category,::std::vector<real_type> > cress;
		::std::map<application_performance_category,real_type> rgains;

		::std::vector<vm_pointer> vms = this->app().vms();
		const ::std::size_t nvms = vms.size();

		const target_iterator tgt_end_it = this->target_values().end();
		for (target_iterator tgt_it = this->target_values().begin();
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
						rgains[cat] = (yr-yh)/yr;
						break;
					case throughput_application_performance:
						rgains[cat] = (yh-yr)/yr;
						break;
				}
DCS_DEBUG_TRACE("APP Performance Category: " << cat << " - Yhat(k): " << yh << " - R: " << yr << " -> Rgain(k+1): " << rgains.at(cat));//XXX
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
        if (skip_ctl)
		{
			++ctl_skip_count_;
		}

		// Export to file
		if (p_dat_ofs_)
		{
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
				*p_dat_ofs_ << p_vm->cpu_cap() << "," << p_vm->cpu_share();
			}
			*p_dat_ofs_ << ",";
			const target_iterator tgt_end_it = this->target_values().end();
			for (target_iterator tgt_it = this->target_values().begin();
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
				const real_type yn = yh/yr;
				*p_dat_ofs_ << yh << "," << yn << "," << yr;
			}
			*p_dat_ofs_ << "," << ctl_count_ << "," << ctl_skip_count_ << "," << ctl_fail_count_;
			*p_dat_ofs_ << ::std::endl;
		}

		DCS_DEBUG_TRACE("(" << this << ") END Do CONTROL - Count: " << ctl_count_ << "/" << ctl_skip_count_ << "/" << ctl_fail_count_);
	}


	private: ::std::size_t ctl_count_; ///< Number of times control function has been invoked
	private: ::std::size_t ctl_skip_count_; ///< Number of times control has been skipped
	private: ::std::size_t ctl_fail_count_; ///< Number of times control has failed
	private: out_sensor_map out_sensors_;
	private: ::std::string dat_fname_;
	private: ::boost::shared_ptr< ::std::ofstream > p_dat_ofs_;
}; // dummy_application_manager

}} // Namespace dcs::testbed

#endif // DCS_TESTBED_DUMMY_APPLICATION_MANAGER_HPP
