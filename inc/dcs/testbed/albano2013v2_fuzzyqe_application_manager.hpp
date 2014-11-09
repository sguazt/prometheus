/**
 * \file dcs/testbed/albano2013v2_fuzzyqe_application_manager.hpp
 *
 * \brief Application manager based on a variation of the work by (Albano et al., 2013)
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

#ifndef DCS_TESTBED_ALBANO2013V2_FUZZYQE_APPLICATION_MANAGER_HPP
#define DCS_TESTBED_ALBANO2013V2_FUZZYQE_APPLICATION_MANAGER_HPP


#include <boost/smart_ptr.hpp>
#include <cmath>
#include <cstddef>
#include <dcs/assert.hpp>
#include <dcs/debug.hpp>
#include <dcs/exception.hpp>
#include <dcs/logging.hpp>
#include <dcs/math/traits/float.hpp>
#include <dcs/testbed/application_performance_category.hpp>
#include <dcs/testbed/base_application_manager.hpp>
#include <dcs/testbed/data_estimators.hpp>
#include <dcs/testbed/data_smoothers.hpp>
#include <dcs/testbed/virtual_machine_performance_category.hpp>
#include <fl/Headers.h>
#include <fstream>
#include <limits>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>


namespace dcs { namespace testbed {

/**
 * \brief Application Manager based on the work by (Albano et al., 2013)
 *
 * This class implements the Fuzzy-Q&E fuzzy controller proposed in [1].
 *
 * References:
 * -# L. Albano, C. Anglano, M. Canonico, and M. Guazzone,
 *    "Fuzzy-Q&E: achieving QoS guarantees and energy savings for cloud applications with fuzzy control,"
 *    Proc. of the 3rd International Conference on Cloud and Green Computing (CGC 2013), 2013. 
 * .
 *
 * \author Marco Guazzone (marco.guazzone@gmail.com)
 */
template <typename TraitsT>
class albano2013v2_fuzzyqe_application_manager: public base_application_manager<TraitsT>
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
	private: typedef ::std::map<virtual_machine_performance_category,::std::map<vm_identifier_type,sensor_pointer> > in_sensor_map;


	private: static const ::std::string rgain_fuzzy_var_name;
	private: static const ::std::string cres_fuzzy_var_name;
	private: static const ::std::string deltac_fuzzy_var_name;


	public: albano2013v2_fuzzyqe_application_manager()
	: beta_(0.9),
	  p_fuzzy_eng_(new fl::Engine()),
	  ctl_count_(0),
	  ctl_skip_count_(0),
	  ctl_fail_count_(0)
	{
		init();
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

	private: void init()
	{
		DCS_DEBUG_ASSERT( p_fuzzy_eng_ );

		fl::InputVariable* p_iv = 0;

		p_iv = new fl::InputVariable();
		p_iv->setEnabled(true);
		p_iv->setName(cres_fuzzy_var_name);
		p_iv->setRange(0.0, 1.0);
		p_iv->addTerm(new fl::Ramp("LOW", 0.30, 0.10));
		p_iv->addTerm(new fl::Triangle("FINE", 0.10, 0.25, 0.40));
		p_iv->addTerm(new fl::Ramp("HIGH", 0.30, 0.65));
		p_fuzzy_eng_->addInputVariable(p_iv);

		p_iv = new fl::InputVariable();
		p_iv->setEnabled(true);
		p_iv->setName(rgain_fuzzy_var_name);
		p_iv->setRange(-1, 1);
		p_iv->addTerm(new fl::Ramp("LOW", 0.20, -0.40));
		p_iv->addTerm(new fl::Triangle("FINE", 0.10, 0.20, 0.30));
		p_iv->addTerm(new fl::Ramp("HIGH", 0.30, 0.65));
		p_fuzzy_eng_->addInputVariable(p_iv);

		fl::OutputVariable* p_ov = 0;
		p_ov = new fl::OutputVariable();
		p_ov->setEnabled(true);
		p_ov->setName(deltac_fuzzy_var_name);
		p_ov->setRange(-1, 1);
		p_ov->fuzzyOutput()->setAccumulation(new fl::AlgebraicSum());
		p_ov->setDefuzzifier(new fl::Centroid());
		p_ov->setDefaultValue(fl::nan);
		p_ov->setPreviousOutputValue(false);
		//p_ov->addTerm(new fl::Ramp("BDW", -0.10, -0.20));
		p_ov->addTerm(new fl::Ramp("BDW", -0.10, -0.55));
		p_ov->addTerm(new fl::Triangle("DWN", -0.20, -0.125, -0.05));
		p_ov->addTerm(new fl::Triangle("STY", -0.10, 0.0, 0.10));
		p_ov->addTerm(new fl::Triangle("UP", 0.05, 0.125, 0.20));
		//p_ov->addTerm(new fl::Ramp("BUP", 0.10, 0.20));
		p_ov->addTerm(new fl::Ramp("BUP", 0.10, 0.55));
		p_fuzzy_eng_->addOutputVariable(p_ov);

		fl::RuleBlock* p_rules = new fl::RuleBlock();
		p_rules->setEnabled(true);
		p_rules->setConjunction(new fl::Minimum());
		p_rules->setDisjunction(new fl::Maximum());
		p_rules->setActivation(new fl::AlgebraicProduct());
		p_rules->addRule(fl::Rule::parse("if " + cres_fuzzy_var_name + " is LOW and " + rgain_fuzzy_var_name + " is LOW then " + deltac_fuzzy_var_name + " is BUP", p_fuzzy_eng_.get()));
		p_rules->addRule(fl::Rule::parse("if " + cres_fuzzy_var_name + " is LOW and " + rgain_fuzzy_var_name + " is FINE then " + deltac_fuzzy_var_name + " is UP", p_fuzzy_eng_.get()));
		p_rules->addRule(fl::Rule::parse("if " + cres_fuzzy_var_name + " is LOW and " + rgain_fuzzy_var_name + " is HIGH then " + deltac_fuzzy_var_name + " is UP", p_fuzzy_eng_.get()));
		p_rules->addRule(fl::Rule::parse("if " + cres_fuzzy_var_name + " is FINE and " + rgain_fuzzy_var_name + " is LOW then " + deltac_fuzzy_var_name + " is UP", p_fuzzy_eng_.get()));
		p_rules->addRule(fl::Rule::parse("if " + cres_fuzzy_var_name + " is FINE and " + rgain_fuzzy_var_name + " is FINE then " + deltac_fuzzy_var_name + " is STY", p_fuzzy_eng_.get()));
		p_rules->addRule(fl::Rule::parse("if " + cres_fuzzy_var_name + " is FINE and " + rgain_fuzzy_var_name + " is HIGH then " + deltac_fuzzy_var_name + " is DWN", p_fuzzy_eng_.get()));
		p_rules->addRule(fl::Rule::parse("if " + cres_fuzzy_var_name + " is HIGH and " + rgain_fuzzy_var_name + " is LOW then " + deltac_fuzzy_var_name + " is STY", p_fuzzy_eng_.get()));
		p_rules->addRule(fl::Rule::parse("if " + cres_fuzzy_var_name + " is HIGH and " + rgain_fuzzy_var_name + " is FINE then " + deltac_fuzzy_var_name + " is DWN", p_fuzzy_eng_.get()));
		p_rules->addRule(fl::Rule::parse("if " + cres_fuzzy_var_name + " is HIGH and " + rgain_fuzzy_var_name + " is HIGH then " + deltac_fuzzy_var_name + " is BDW", p_fuzzy_eng_.get()));
		p_fuzzy_eng_->addRuleBlock(p_rules);
	}

	private: void do_reset()
	{
		typedef typename base_type::target_value_map::const_iterator target_iterator;
		typedef typename app_type::vm_pointer vm_pointer;

		const ::std::vector<vm_pointer> vms = this->app().vms();
		const ::std::size_t nvms = this->app().num_vms();

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

		// Reset input sensors
		in_sensors_.clear();
		for (::std::size_t i = 0; i < nvms; ++i)
		{
			const virtual_machine_performance_category cat = cpu_util_virtual_machine_performance;
			vm_pointer p_vm = vms[i];

			in_sensors_[cat][p_vm->id()] = p_vm->sensor(cat);
		}

		// Reset counters
		ctl_count_ = ctl_skip_count_
				   = ctl_fail_count_
				   = 0;

		// Reset fuzzy controller
		p_fuzzy_eng_->restart();

		//// Reset Cres estimator and smoother
		//this->data_estimator(cpu_util_virtual_machine_performance).reset();
		//this->data_smoother(cpu_util_virtual_machine_performance).reset();
		for (::std::size_t i = 0; i < nvms; ++i)
		{
			this->data_smoother(cpu_util_virtual_machine_performance, vms[i]->id(), ::boost::make_shared< testbed::brown_single_exponential_smoother<real_type> >(beta_));
			//this->data_estimator(cpu_util_virtual_machine_performance, vms[i]->id(), ::boost::make_shared< testbed::mean_estimator<real_type> >());
		}

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
		typedef typename in_sensor_map::const_iterator in_sensor_iterator;
		typedef typename out_sensor_map::const_iterator out_sensor_iterator;
		typedef ::std::vector<typename sensor_type::observation_type> obs_container;
		typedef typename obs_container::const_iterator obs_iterator;

		DCS_DEBUG_TRACE("(" << this << ") BEGIN Do SAMPLE - Count: " << ctl_count_ << "/" << ctl_skip_count_ << "/" << ctl_fail_count_);

		// Collect input values
		const in_sensor_iterator in_sens_end_it = in_sensors_.end();
		for (in_sensor_iterator in_sens_it = in_sensors_.begin();
			 in_sens_it != in_sens_end_it;
			 ++in_sens_it)
		{
			const virtual_machine_performance_category cat = in_sens_it->first;

			//const ::std::size_t n = in_sens_it->second.size();
			//for (::std::size_t i = 0; i < n; ++i)
			//{
			//	sensor_pointer p_sens = in_sens_it->second.at(i);
			const typename in_sensor_map::mapped_type::const_iterator vm_end_it = in_sens_it->second.end();
			for (typename in_sensor_map::mapped_type::const_iterator vm_it = in_sens_it->second.begin();
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

		for (::std::size_t i = 0; i < nvms; ++i)
		{
			const virtual_machine_performance_category cat = cpu_util_virtual_machine_performance;
			const vm_pointer p_vm = vms[i];

//			if (this->data_estimator(cat, vms[i]->id()).count() > 0)
//			{
//				//const real_type uh = this->data_estimator(cat, p_vm->id()).estimate();
//				const real_type uh = this->data_smoother(cat, p_vm->id()).forecast(0);
//				const real_type c = p_vm->cpu_share();
//
//				cress[cat].push_back(c-uh);
//			}
//			else
//			{
//				// No observation collected during the last control interval
//				DCS_DEBUG_TRACE("No input observation collected during the last control interval -> Skip control");
//				skip_ctl = true;
//				break;
//			}
			const real_type uh = this->data_smoother(cat, p_vm->id()).forecast(0);
			const real_type c = p_vm->cpu_share();

			cress[cat].push_back(c-uh);
DCS_DEBUG_TRACE("VM " << p_vm->id() << " - Performance Category: " << cat << " - Uhat(k): " << uh << " - C(k): " << c << " -> Cres(k+1): " << cress.at(cat).at(i));//XXX
		}

		if (!skip_ctl)
		{
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
		}
        if (!skip_ctl)
        {
			//FIXME: actually we only handle SISO systems
			DCS_ASSERT(cress.size() == 1 && rgains.size() == 1,
					   DCS_EXCEPTION_THROW(::std::runtime_error,
					   "Only SISO system are currently managed"));

			// Perform fuzzy control
			bool ok = false;
			::std::vector<real_type> deltacs(nvms);
			try
			{
				for (::std::size_t i = 0; i < nvms; ++i)
				{
					const real_type cres = cress.begin()->second.at(i);
					const real_type rgain = rgains.begin()->second;

					p_fuzzy_eng_->setInputValue(cres_fuzzy_var_name, cres);
					p_fuzzy_eng_->setInputValue(rgain_fuzzy_var_name, rgain);

					p_fuzzy_eng_->process();

					const real_type deltac = p_fuzzy_eng_->getOutputValue(deltac_fuzzy_var_name);

					deltacs[i] = deltac;
DCS_DEBUG_TRACE("VM " << vms[i]->id() << " -> DeltaC(k+1): " << deltacs.at(i));//XXX
				}

				ok = true;
			}
			catch (fl::Exception const& fe)
			{
				DCS_DEBUG_TRACE( "Caught exception: " << fe.what() );

				::std::ostringstream oss;
				oss << "Unable to compute optimal control: " << fe.what();
				::dcs::log_warn(DCS_LOGGING_AT, oss.str());
			}
			catch (::std::exception const& e)
			{
				DCS_DEBUG_TRACE( "Caught exception: " << e.what() );

				::std::ostringstream oss;
				oss << "Unable to compute optimal control: " << e.what();
				::dcs::log_warn(DCS_LOGGING_AT, oss.str());
			}

			// Apply fuzzy control results
			if (ok)
			{
				for (::std::size_t i = 0; i < nvms; ++i)
				{
					vm_pointer p_vm = vms[i];

					const real_type old_share = p_vm->cpu_share();
					const real_type new_share = ::std::max(::std::min(old_share+deltacs[i], 1.0), 0.0);

					DCS_DEBUG_TRACE("VM '" << p_vm->id() << "' - old-share: " << old_share << " - new-share: " << new_share);

					if (::std::isfinite(new_share) && !::dcs::math::float_traits<real_type>::essentially_equal(old_share, new_share))
					{
						p_vm->cpu_share(new_share);
DCS_DEBUG_TRACE("VM " << vms[i]->id() << " -> C(k+1): " << new_share);//XXX
					}
				}
DCS_DEBUG_TRACE("Optimal control applied");//XXX
			}
			else
			{
				++ctl_fail_count_;

				::std::ostringstream oss;
				oss << "Control not applied: failed to solve the control problem";
				::dcs::log_warn(DCS_LOGGING_AT, oss.str());
			}
		}
		else
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


	private: real_type beta_; ///< The EWMA smoothing factor for Cres
	private: ::boost::shared_ptr<fl::Engine> p_fuzzy_eng_; ///< The fuzzy control engine
	private: ::std::size_t ctl_count_; ///< Number of times control function has been invoked
	private: ::std::size_t ctl_skip_count_; ///< Number of times control has been skipped
	private: ::std::size_t ctl_fail_count_; ///< Number of times control has failed
	private: in_sensor_map in_sensors_;
	private: out_sensor_map out_sensors_;
	private: ::std::string dat_fname_;
	private: ::boost::shared_ptr< ::std::ofstream > p_dat_ofs_;
}; // albano2013v2_fuzzyqe_application_manager

template <typename T>
const ::std::string albano2013v2_fuzzyqe_application_manager<T>::rgain_fuzzy_var_name = "Rgain";

template <typename T>
const ::std::string albano2013v2_fuzzyqe_application_manager<T>::cres_fuzzy_var_name = "Cres";

template <typename T>
const ::std::string albano2013v2_fuzzyqe_application_manager<T>::deltac_fuzzy_var_name = "DeltaC";

}} // Namespace dcs::testbed

#endif // DCS_TESTBED_ALBANO2013V2_FUZZYQE_APPLICATION_MANAGER_HPP
