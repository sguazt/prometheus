/**
 * \file dcs/testbed/anglano2014_fc2q_mimo_v2_application_manager.hpp
 *
 * \brief Application manager based on the work by (Anglano et al., 2014)
 *
 * This class implements the FC2Q fuzzy controller proposed in [1].
 *
 * References:
 * -# C. Anglano, M. Canonico and M. Guazzone,
 *    "FC2Q: Exploiting Fuzzy Control in Server Consolidation for Cloud Applications with SLA Constraints,"
 *    Concurrency and Computation: Practice and Experience, Accepted for publication, 2014.
 * .
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

#ifndef DCS_TESTBED_ANGLANO2014_FC2Q_MIMO_V2_APPLICATION_MANAGER_HPP
#define DCS_TESTBED_ANGLANO2014_FC2Q_MIMO_V2_APPLICATION_MANAGER_HPP


#include <boost/smart_ptr.hpp>
#include <boost/timer/timer.hpp>
#include <cmath>
#include <cstddef>
#include <dcs/assert.hpp>
#include <dcs/debug.hpp>
#include <dcs/exception.hpp>
#include <dcs/logging.hpp>
#include <dcs/math/function/clamp.hpp>
#include <dcs/math/traits/float.hpp>
#include <dcs/testbed/application_performance_category.hpp>
#include <dcs/testbed/base_application_manager.hpp>
//#include <dcs/testbed/data_estimators.hpp>
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
 * \brief Application manager based on the work by (Anglano et al., 2014)
 *
 * This class implements the FC2Q fuzzy controller proposed in [1].
 *
 * References:
 * -# C. Anglano, M. Canonico and M. Guazzone,
 *    "FC2Q: Exploiting Fuzzy Control in Server Consolidation for Cloud Applications with SLA Constraints,"
 *    Future Generation Computer Systems, Submitted for publication, 2014.
 * .
 *
 * \author Marco Guazzone (marco.guazzone@gmail.com)
 */
template <typename TraitsT>
class anglano2014_fc2q_mimo_v2_application_manager: public base_application_manager<TraitsT>
{
	private: typedef base_application_manager<TraitsT> base_type;
	public: typedef typename base_type::traits_type traits_type;
	public: typedef typename traits_type::real_type real_type;
	private: typedef typename base_type::app_type app_type;
	private: typedef typename base_type::app_pointer app_pointer;
	private: typedef typename base_type::vm_identifier_type vm_identifier_type;
	private: typedef typename app_type::sensor_type sensor_type;
	private: typedef typename app_type::sensor_pointer sensor_pointer;
	private: typedef std::map<application_performance_category,sensor_pointer> out_sensor_map;
	private: typedef std::map<virtual_machine_performance_category,std::map<vm_identifier_type,sensor_pointer> > in_sensor_map;


	private: static const std::size_t control_warmup_size;
	private: static const float resource_share_lb_scale_factor;

	private: static const std::string err_fuzzy_var_name;
	private: static const std::string deltaerr_fuzzy_var_name;
	private: static const std::string cres_fuzzy_var_name;
	private: static const std::string deltac_fuzzy_var_name;
	private: static const std::string mres_fuzzy_var_name;
	private: static const std::string deltam_fuzzy_var_name;


	public: anglano2014_fc2q_mimo_v2_application_manager()
	: beta_(0.9),
	  p_fuzzy_eng_(new fl::Engine()),
	  last_err_(std::numeric_limits<real_type>::quiet_NaN()),
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

	public: void export_data_to(std::string const& fname)
	{
		dat_fname_ = fname;
	}

	private: void init()
	{
		DCS_DEBUG_ASSERT( p_fuzzy_eng_ );

		vm_perf_cats_.push_back(cpu_util_virtual_machine_performance);
		vm_perf_cats_.push_back(memory_util_virtual_machine_performance);

		//app_perf_cats_.push_back(cpu_util_virtual_machine_performance);

		fl::InputVariable* p_iv = 0;

		p_iv = new fl::InputVariable();
		p_iv->setEnabled(true);
		p_iv->setName(cres_fuzzy_var_name);
		p_iv->setRange(0.0, 1.0);
		p_iv->addTerm(new fl::Ramp("LOW", 0.30, 0.00));
		//p_iv->addTerm(new fl::Ramp("LOW", 0.20, 0.00));
		//p_iv->addTerm(new fl::Ramp("LOW", 0.15, 0.00));
		p_iv->addTerm(new fl::Triangle("FINE", 0.10, 0.25, 0.40));
		//p_iv->addTerm(new fl::Triangle("FINE", 0.10, 0.20, 0.30));
		//p_iv->addTerm(new fl::Triangle("FINE", 0.10, 0.15, 0.20));
		p_iv->addTerm(new fl::Ramp("HIGH", 0.30, 1.00));
		//p_iv->addTerm(new fl::Ramp("HIGH", 0.20, 1.00));
		//p_iv->addTerm(new fl::Ramp("HIGH", 0.15, 1.00));
		p_fuzzy_eng_->addInputVariable(p_iv);

		p_iv = new fl::InputVariable();
		p_iv->setEnabled(true);
		p_iv->setName(mres_fuzzy_var_name);
		p_iv->setRange(0.0, 1.0);
		p_iv->addTerm(new fl::Ramp("LOW", 0.30, 0.00));
		//p_iv->addTerm(new fl::Ramp("LOW", 0.20, 0.00));
		//p_iv->addTerm(new fl::Ramp("LOW", 0.15, 0.00));
		p_iv->addTerm(new fl::Triangle("FINE", 0.10, 0.25, 0.40));
		//p_iv->addTerm(new fl::Triangle("FINE", 0.10, 0.20, 0.30));
		//p_iv->addTerm(new fl::Triangle("FINE", 0.10, 0.15, 0.20));
		p_iv->addTerm(new fl::Ramp("HIGH", 0.30, 1.00));
		//p_iv->addTerm(new fl::Ramp("HIGH", 0.20, 1.00));
		//p_iv->addTerm(new fl::Ramp("HIGH", 0.15, 1.00));
		p_fuzzy_eng_->addInputVariable(p_iv);

		p_iv = new fl::InputVariable();
		p_iv->setEnabled(true);
		p_iv->setName(err_fuzzy_var_name);
		p_iv->setRange(-1, 1);
		p_iv->addTerm(new fl::Ramp("NEG", 0.20, -0.40));
		//p_iv->addTerm(new fl::Ramp("NEG", 0.15, -0.10));
		p_iv->addTerm(new fl::Triangle("OK", 0.10, 0.20, 0.30));
		//p_iv->addTerm(new fl::Triangle("OK", 0.00, 0.15, 0.30));
		p_iv->addTerm(new fl::Ramp("POS", 0.30, 1.00));
		//p_iv->addTerm(new fl::Ramp("POS", 0.15, 0.40));
		p_fuzzy_eng_->addInputVariable(p_iv);

		p_iv = new fl::InputVariable();
		p_iv->setEnabled(true);
		p_iv->setName(deltaerr_fuzzy_var_name);
		p_iv->setRange(-2, 2);
		p_iv->addTerm(new fl::Ramp("NEG", 0.00, -2.00));
		//p_iv->addTerm(new fl::Triangle("ZERO", -0.50, 0.00, 0.50));
		p_iv->addTerm(new fl::Triangle("ZERO", -0.10, 0.00, 0.10));
		p_iv->addTerm(new fl::Ramp("POS", 0.00, 2.00));
		p_fuzzy_eng_->addInputVariable(p_iv);

		fl::OutputVariable* p_ov = 0;
		p_ov = new fl::OutputVariable();
		p_ov->setEnabled(true);
		p_ov->setName(deltac_fuzzy_var_name);
		p_ov->setRange(-1, 1);
		//p_ov->setLockValueInRange(true);
		p_ov->fuzzyOutput()->setAccumulation(new fl::AlgebraicSum());
		p_ov->setDefuzzifier(new fl::Centroid());
		p_ov->setDefaultValue(fl::nan);
		p_ov->setPreviousValue(false);
		p_ov->addTerm(new fl::Triangle("BDW", -1.00, -0.55, -0.10));
		p_ov->addTerm(new fl::Triangle("DWN", -0.20, -0.125, -0.05));
		p_ov->addTerm(new fl::Triangle("STY", -0.10, 0.0, 0.10));
		p_ov->addTerm(new fl::Triangle("UP", 0.05, 0.125, 0.20));
		p_ov->addTerm(new fl::Triangle("BUP", 0.10, 0.55, 1.00));
		p_fuzzy_eng_->addOutputVariable(p_ov);

		p_ov = new fl::OutputVariable();
		p_ov->setEnabled(true);
		p_ov->setName(deltam_fuzzy_var_name);
		p_ov->setRange(-1, 1);
		//p_ov->setLockValueInRange(true);
		p_ov->fuzzyOutput()->setAccumulation(new fl::AlgebraicSum());
		p_ov->setDefuzzifier(new fl::Centroid());
		p_ov->setDefaultValue(fl::nan);
		p_ov->setPreviousValue(false);
		p_ov->addTerm(new fl::Triangle("BDW", -1.00, -0.55, -0.10));
		p_ov->addTerm(new fl::Triangle("DWN", -0.20, -0.125, -0.05));
		p_ov->addTerm(new fl::Triangle("STY", -0.10, 0.0, 0.10));
		p_ov->addTerm(new fl::Triangle("UP", 0.05, 0.125, 0.20));
		p_ov->addTerm(new fl::Triangle("BUP", 0.10, 0.55, 1.00));
		p_fuzzy_eng_->addOutputVariable(p_ov);

		fl::RuleBlock* p_rules = new fl::RuleBlock();
		p_rules->setEnabled(true);
		p_rules->setConjunction(new fl::Minimum());
		p_rules->setDisjunction(new fl::Maximum());
		p_rules->setImplication(new fl::AlgebraicProduct());
		p_rules->addRule(fl::Rule::parse("if " + cres_fuzzy_var_name + " is LOW and " + err_fuzzy_var_name + " is NEG and " + deltaerr_fuzzy_var_name + " is NEG then " + deltac_fuzzy_var_name + " is BUP", p_fuzzy_eng_.get()));
		p_rules->addRule(fl::Rule::parse("if " + cres_fuzzy_var_name + " is LOW and " + err_fuzzy_var_name + " is NEG and " + deltaerr_fuzzy_var_name + " is ZERO then " + deltac_fuzzy_var_name + " is BUP", p_fuzzy_eng_.get()));
		//p_rules->addRule(fl::Rule::parse("if " + cres_fuzzy_var_name + " is LOW and " + err_fuzzy_var_name + " is NEG and " + deltaerr_fuzzy_var_name + " is POS then " + deltac_fuzzy_var_name + " is BUP", p_fuzzy_eng_.get()));
		p_rules->addRule(fl::Rule::parse("if " + cres_fuzzy_var_name + " is LOW and " + err_fuzzy_var_name + " is NEG and " + deltaerr_fuzzy_var_name + " is POS then " + deltac_fuzzy_var_name + " is UP", p_fuzzy_eng_.get()));
		p_rules->addRule(fl::Rule::parse("if " + cres_fuzzy_var_name + " is LOW and " + err_fuzzy_var_name + " is OK and " + deltaerr_fuzzy_var_name + " is NEG then " + deltac_fuzzy_var_name + " is UP", p_fuzzy_eng_.get()));
		p_rules->addRule(fl::Rule::parse("if " + cres_fuzzy_var_name + " is LOW and " + err_fuzzy_var_name + " is OK and " + deltaerr_fuzzy_var_name + " is ZERO then " + deltac_fuzzy_var_name + " is UP", p_fuzzy_eng_.get()));
		p_rules->addRule(fl::Rule::parse("if " + cres_fuzzy_var_name + " is LOW and " + err_fuzzy_var_name + " is OK and " + deltaerr_fuzzy_var_name + " is POS then " + deltac_fuzzy_var_name + " is STY", p_fuzzy_eng_.get()));
		p_rules->addRule(fl::Rule::parse("if " + cres_fuzzy_var_name + " is LOW and " + err_fuzzy_var_name + " is POS and " + deltaerr_fuzzy_var_name + " is NEG then " + deltac_fuzzy_var_name + " is UP", p_fuzzy_eng_.get()));
		//p_rules->addRule(fl::Rule::parse("if " + cres_fuzzy_var_name + " is LOW and " + err_fuzzy_var_name + " is POS and " + deltaerr_fuzzy_var_name + " is ZERO then " + deltac_fuzzy_var_name + " is UP", p_fuzzy_eng_.get()));
		p_rules->addRule(fl::Rule::parse("if " + cres_fuzzy_var_name + " is LOW and " + err_fuzzy_var_name + " is POS and " + deltaerr_fuzzy_var_name + " is ZERO then " + deltac_fuzzy_var_name + " is STY", p_fuzzy_eng_.get()));
		p_rules->addRule(fl::Rule::parse("if " + cres_fuzzy_var_name + " is LOW and " + err_fuzzy_var_name + " is POS and " + deltaerr_fuzzy_var_name + " is POS then " + deltac_fuzzy_var_name + " is STY", p_fuzzy_eng_.get()));
		p_rules->addRule(fl::Rule::parse("if " + cres_fuzzy_var_name + " is FINE and " + err_fuzzy_var_name + " is NEG and " + deltaerr_fuzzy_var_name + " is NEG then " + deltac_fuzzy_var_name + " is UP", p_fuzzy_eng_.get()));
		p_rules->addRule(fl::Rule::parse("if " + cres_fuzzy_var_name + " is FINE and " + err_fuzzy_var_name + " is NEG and " + deltaerr_fuzzy_var_name + " is ZERO then " + deltac_fuzzy_var_name + " is UP", p_fuzzy_eng_.get()));
		p_rules->addRule(fl::Rule::parse("if " + cres_fuzzy_var_name + " is FINE and " + err_fuzzy_var_name + " is NEG and " + deltaerr_fuzzy_var_name + " is POS then " + deltac_fuzzy_var_name + " is STY", p_fuzzy_eng_.get()));
		p_rules->addRule(fl::Rule::parse("if " + cres_fuzzy_var_name + " is FINE and " + err_fuzzy_var_name + " is OK and " + deltaerr_fuzzy_var_name + " is NEG then " + deltac_fuzzy_var_name + " is STY", p_fuzzy_eng_.get()));
		//p_rules->addRule(fl::Rule::parse("if " + cres_fuzzy_var_name + " is FINE and " + err_fuzzy_var_name + " is OK and " + deltaerr_fuzzy_var_name + " is NEG then " + deltac_fuzzy_var_name + " is UP", p_fuzzy_eng_.get()));
		p_rules->addRule(fl::Rule::parse("if " + cres_fuzzy_var_name + " is FINE and " + err_fuzzy_var_name + " is OK and " + deltaerr_fuzzy_var_name + " is ZERO then " + deltac_fuzzy_var_name + " is STY", p_fuzzy_eng_.get()));
		p_rules->addRule(fl::Rule::parse("if " + cres_fuzzy_var_name + " is FINE and " + err_fuzzy_var_name + " is OK and " + deltaerr_fuzzy_var_name + " is POS then " + deltac_fuzzy_var_name + " is DWN", p_fuzzy_eng_.get()));
		p_rules->addRule(fl::Rule::parse("if " + cres_fuzzy_var_name + " is FINE and " + err_fuzzy_var_name + " is POS and " + deltaerr_fuzzy_var_name + " is NEG then " + deltac_fuzzy_var_name + " is STY", p_fuzzy_eng_.get()));
		//p_rules->addRule(fl::Rule::parse("if " + cres_fuzzy_var_name + " is FINE and " + err_fuzzy_var_name + " is POS and " + deltaerr_fuzzy_var_name + " is ZERO then " + deltac_fuzzy_var_name + " is DWN", p_fuzzy_eng_.get()));
		p_rules->addRule(fl::Rule::parse("if " + cres_fuzzy_var_name + " is FINE and " + err_fuzzy_var_name + " is POS and " + deltaerr_fuzzy_var_name + " is ZERO then " + deltac_fuzzy_var_name + " is STY", p_fuzzy_eng_.get()));
		p_rules->addRule(fl::Rule::parse("if " + cres_fuzzy_var_name + " is FINE and " + err_fuzzy_var_name + " is POS and " + deltaerr_fuzzy_var_name + " is POS then " + deltac_fuzzy_var_name + " is DWN", p_fuzzy_eng_.get()));
		p_rules->addRule(fl::Rule::parse("if " + cres_fuzzy_var_name + " is HIGH and " + err_fuzzy_var_name + " is NEG and " + deltaerr_fuzzy_var_name + " is NEG then " + deltac_fuzzy_var_name + " is UP", p_fuzzy_eng_.get()));
		//p_rules->addRule(fl::Rule::parse("if " + cres_fuzzy_var_name + " is HIGH and " + err_fuzzy_var_name + " is NEG and " + deltaerr_fuzzy_var_name + " is ZERO then " + deltac_fuzzy_var_name + " is STY", p_fuzzy_eng_.get()));
		p_rules->addRule(fl::Rule::parse("if " + cres_fuzzy_var_name + " is HIGH and " + err_fuzzy_var_name + " is NEG and " + deltaerr_fuzzy_var_name + " is ZERO then " + deltac_fuzzy_var_name + " is UP", p_fuzzy_eng_.get()));
		p_rules->addRule(fl::Rule::parse("if " + cres_fuzzy_var_name + " is HIGH and " + err_fuzzy_var_name + " is NEG and " + deltaerr_fuzzy_var_name + " is POS then " + deltac_fuzzy_var_name + " is STY", p_fuzzy_eng_.get()));
		p_rules->addRule(fl::Rule::parse("if " + cres_fuzzy_var_name + " is HIGH and " + err_fuzzy_var_name + " is OK and " + deltaerr_fuzzy_var_name + " is NEG then " + deltac_fuzzy_var_name + " is STY", p_fuzzy_eng_.get()));
		p_rules->addRule(fl::Rule::parse("if " + cres_fuzzy_var_name + " is HIGH and " + err_fuzzy_var_name + " is OK and " + deltaerr_fuzzy_var_name + " is ZERO then " + deltac_fuzzy_var_name + " is DWN", p_fuzzy_eng_.get()));
		p_rules->addRule(fl::Rule::parse("if " + cres_fuzzy_var_name + " is HIGH and " + err_fuzzy_var_name + " is OK and " + deltaerr_fuzzy_var_name + " is POS then " + deltac_fuzzy_var_name + " is DWN", p_fuzzy_eng_.get()));
		p_rules->addRule(fl::Rule::parse("if " + cres_fuzzy_var_name + " is HIGH and " + err_fuzzy_var_name + " is POS and " + deltaerr_fuzzy_var_name + " is NEG then " + deltac_fuzzy_var_name + " is DWN", p_fuzzy_eng_.get()));
		p_rules->addRule(fl::Rule::parse("if " + cres_fuzzy_var_name + " is HIGH and " + err_fuzzy_var_name + " is POS and " + deltaerr_fuzzy_var_name + " is ZERO then " + deltac_fuzzy_var_name + " is BDW", p_fuzzy_eng_.get()));
		p_rules->addRule(fl::Rule::parse("if " + cres_fuzzy_var_name + " is HIGH and " + err_fuzzy_var_name + " is POS and " + deltaerr_fuzzy_var_name + " is POS then " + deltac_fuzzy_var_name + " is BDW", p_fuzzy_eng_.get()));
		p_rules->addRule(fl::Rule::parse("if " + mres_fuzzy_var_name + " is LOW and " + err_fuzzy_var_name + " is NEG and " + deltaerr_fuzzy_var_name + " is NEG then " + deltam_fuzzy_var_name + " is BUP", p_fuzzy_eng_.get()));
		p_rules->addRule(fl::Rule::parse("if " + mres_fuzzy_var_name + " is LOW and " + err_fuzzy_var_name + " is NEG and " + deltaerr_fuzzy_var_name + " is ZERO then " + deltam_fuzzy_var_name + " is BUP", p_fuzzy_eng_.get()));
		//p_rules->addRule(fl::Rule::parse("if " + mres_fuzzy_var_name + " is LOW and " + err_fuzzy_var_name + " is NEG and " + deltaerr_fuzzy_var_name + " is POS then " + deltam_fuzzy_var_name + " is BUP", p_fuzzy_eng_.get()));
		p_rules->addRule(fl::Rule::parse("if " + mres_fuzzy_var_name + " is LOW and " + err_fuzzy_var_name + " is NEG and " + deltaerr_fuzzy_var_name + " is POS then " + deltam_fuzzy_var_name + " is UP", p_fuzzy_eng_.get()));
		p_rules->addRule(fl::Rule::parse("if " + mres_fuzzy_var_name + " is LOW and " + err_fuzzy_var_name + " is OK and " + deltaerr_fuzzy_var_name + " is NEG then " + deltam_fuzzy_var_name + " is UP", p_fuzzy_eng_.get()));
		p_rules->addRule(fl::Rule::parse("if " + mres_fuzzy_var_name + " is LOW and " + err_fuzzy_var_name + " is OK and " + deltaerr_fuzzy_var_name + " is ZERO then " + deltam_fuzzy_var_name + " is UP", p_fuzzy_eng_.get()));
		p_rules->addRule(fl::Rule::parse("if " + mres_fuzzy_var_name + " is LOW and " + err_fuzzy_var_name + " is OK and " + deltaerr_fuzzy_var_name + " is POS then " + deltam_fuzzy_var_name + " is STY", p_fuzzy_eng_.get()));
		p_rules->addRule(fl::Rule::parse("if " + mres_fuzzy_var_name + " is LOW and " + err_fuzzy_var_name + " is POS and " + deltaerr_fuzzy_var_name + " is NEG then " + deltam_fuzzy_var_name + " is UP", p_fuzzy_eng_.get()));
		//p_rules->addRule(fl::Rule::parse("if " + mres_fuzzy_var_name + " is LOW and " + err_fuzzy_var_name + " is POS and " + deltaerr_fuzzy_var_name + " is ZERO then " + deltam_fuzzy_var_name + " is UP", p_fuzzy_eng_.get()));
		p_rules->addRule(fl::Rule::parse("if " + mres_fuzzy_var_name + " is LOW and " + err_fuzzy_var_name + " is POS and " + deltaerr_fuzzy_var_name + " is ZERO then " + deltam_fuzzy_var_name + " is STY", p_fuzzy_eng_.get()));
		p_rules->addRule(fl::Rule::parse("if " + mres_fuzzy_var_name + " is LOW and " + err_fuzzy_var_name + " is POS and " + deltaerr_fuzzy_var_name + " is POS then " + deltam_fuzzy_var_name + " is STY", p_fuzzy_eng_.get()));
		p_rules->addRule(fl::Rule::parse("if " + mres_fuzzy_var_name + " is FINE and " + err_fuzzy_var_name + " is NEG and " + deltaerr_fuzzy_var_name + " is NEG then " + deltam_fuzzy_var_name + " is UP", p_fuzzy_eng_.get()));
		p_rules->addRule(fl::Rule::parse("if " + mres_fuzzy_var_name + " is FINE and " + err_fuzzy_var_name + " is NEG and " + deltaerr_fuzzy_var_name + " is ZERO then " + deltam_fuzzy_var_name + " is UP", p_fuzzy_eng_.get()));
		p_rules->addRule(fl::Rule::parse("if " + mres_fuzzy_var_name + " is FINE and " + err_fuzzy_var_name + " is NEG and " + deltaerr_fuzzy_var_name + " is POS then " + deltam_fuzzy_var_name + " is STY", p_fuzzy_eng_.get()));
		p_rules->addRule(fl::Rule::parse("if " + mres_fuzzy_var_name + " is FINE and " + err_fuzzy_var_name + " is OK and " + deltaerr_fuzzy_var_name + " is NEG then " + deltam_fuzzy_var_name + " is STY", p_fuzzy_eng_.get()));
		//p_rules->addRule(fl::Rule::parse("if " + mres_fuzzy_var_name + " is FINE and " + err_fuzzy_var_name + " is OK and " + deltaerr_fuzzy_var_name + " is NEG then " + deltam_fuzzy_var_name + " is UP", p_fuzzy_eng_.get()));
		p_rules->addRule(fl::Rule::parse("if " + mres_fuzzy_var_name + " is FINE and " + err_fuzzy_var_name + " is OK and " + deltaerr_fuzzy_var_name + " is ZERO then " + deltam_fuzzy_var_name + " is STY", p_fuzzy_eng_.get()));
		p_rules->addRule(fl::Rule::parse("if " + mres_fuzzy_var_name + " is FINE and " + err_fuzzy_var_name + " is OK and " + deltaerr_fuzzy_var_name + " is POS then " + deltam_fuzzy_var_name + " is DWN", p_fuzzy_eng_.get()));
		p_rules->addRule(fl::Rule::parse("if " + mres_fuzzy_var_name + " is FINE and " + err_fuzzy_var_name + " is POS and " + deltaerr_fuzzy_var_name + " is NEG then " + deltam_fuzzy_var_name + " is STY", p_fuzzy_eng_.get()));
		//p_rules->addRule(fl::Rule::parse("if " + mres_fuzzy_var_name + " is FINE and " + err_fuzzy_var_name + " is POS and " + deltaerr_fuzzy_var_name + " is ZERO then " + deltam_fuzzy_var_name + " is DWN", p_fuzzy_eng_.get()));
		p_rules->addRule(fl::Rule::parse("if " + mres_fuzzy_var_name + " is FINE and " + err_fuzzy_var_name + " is POS and " + deltaerr_fuzzy_var_name + " is ZERO then " + deltam_fuzzy_var_name + " is STY", p_fuzzy_eng_.get()));
		p_rules->addRule(fl::Rule::parse("if " + mres_fuzzy_var_name + " is FINE and " + err_fuzzy_var_name + " is POS and " + deltaerr_fuzzy_var_name + " is POS then " + deltam_fuzzy_var_name + " is DWN", p_fuzzy_eng_.get()));
		p_rules->addRule(fl::Rule::parse("if " + mres_fuzzy_var_name + " is HIGH and " + err_fuzzy_var_name + " is NEG and " + deltaerr_fuzzy_var_name + " is NEG then " + deltam_fuzzy_var_name + " is UP", p_fuzzy_eng_.get()));
		//p_rules->addRule(fl::Rule::parse("if " + mres_fuzzy_var_name + " is HIGH and " + err_fuzzy_var_name + " is NEG and " + deltaerr_fuzzy_var_name + " is ZERO then " + deltam_fuzzy_var_name + " is STY", p_fuzzy_eng_.get()));
		p_rules->addRule(fl::Rule::parse("if " + mres_fuzzy_var_name + " is HIGH and " + err_fuzzy_var_name + " is NEG and " + deltaerr_fuzzy_var_name + " is ZERO then " + deltam_fuzzy_var_name + " is UP", p_fuzzy_eng_.get()));
		p_rules->addRule(fl::Rule::parse("if " + mres_fuzzy_var_name + " is HIGH and " + err_fuzzy_var_name + " is NEG and " + deltaerr_fuzzy_var_name + " is POS then " + deltam_fuzzy_var_name + " is STY", p_fuzzy_eng_.get()));
		p_rules->addRule(fl::Rule::parse("if " + mres_fuzzy_var_name + " is HIGH and " + err_fuzzy_var_name + " is OK and " + deltaerr_fuzzy_var_name + " is NEG then " + deltam_fuzzy_var_name + " is STY", p_fuzzy_eng_.get()));
		p_rules->addRule(fl::Rule::parse("if " + mres_fuzzy_var_name + " is HIGH and " + err_fuzzy_var_name + " is OK and " + deltaerr_fuzzy_var_name + " is ZERO then " + deltam_fuzzy_var_name + " is DWN", p_fuzzy_eng_.get()));
		p_rules->addRule(fl::Rule::parse("if " + mres_fuzzy_var_name + " is HIGH and " + err_fuzzy_var_name + " is OK and " + deltaerr_fuzzy_var_name + " is POS then " + deltam_fuzzy_var_name + " is DWN", p_fuzzy_eng_.get()));
		p_rules->addRule(fl::Rule::parse("if " + mres_fuzzy_var_name + " is HIGH and " + err_fuzzy_var_name + " is POS and " + deltaerr_fuzzy_var_name + " is NEG then " + deltam_fuzzy_var_name + " is DWN", p_fuzzy_eng_.get()));
		p_rules->addRule(fl::Rule::parse("if " + mres_fuzzy_var_name + " is HIGH and " + err_fuzzy_var_name + " is POS and " + deltaerr_fuzzy_var_name + " is ZERO then " + deltam_fuzzy_var_name + " is BDW", p_fuzzy_eng_.get()));
		p_rules->addRule(fl::Rule::parse("if " + mres_fuzzy_var_name + " is HIGH and " + err_fuzzy_var_name + " is POS and " + deltaerr_fuzzy_var_name + " is POS then " + deltam_fuzzy_var_name + " is BDW", p_fuzzy_eng_.get()));

		p_fuzzy_eng_->addRuleBlock(p_rules);

		DCS_DEBUG_TRACE( p_fuzzy_eng_->toString() );
	}

	private: void do_reset()
	{
		typedef typename base_type::target_value_map::const_iterator target_iterator;
		typedef typename app_type::vm_pointer vm_pointer;

		const std::vector<vm_pointer> vms = this->app().vms();
		const std::size_t nvms = this->app().num_vms();
		const std::size_t num_vm_perf_cats = vm_perf_cats_.size();

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
		for (std::size_t i = 0; i < nvms; ++i)
		{
			for (std::size_t j = 0; j < num_vm_perf_cats; ++j)
			{
				const virtual_machine_performance_category cat = vm_perf_cats_[j];
				vm_pointer p_vm = vms[i];

				in_sensors_[cat][p_vm->id()] = p_vm->sensor(cat);
			}
		}

		// Reset error
		last_err_ = std::numeric_limits<real_type>::quiet_NaN();

		// Reset counters
		ctl_count_ = ctl_skip_count_
				   = ctl_fail_count_
				   = 0;

		// Reset fuzzy controller
		p_fuzzy_eng_->restart();

		//// Reset Cres estimator and smoother
		//this->data_estimator(cpu_util_virtual_machine_performance).reset();
		//this->data_smoother(cpu_util_virtual_machine_performance).reset();
		for (std::size_t i = 0; i < nvms; ++i)
		{
			for (std::size_t j = 0; j < num_vm_perf_cats; ++j)
			{
				const virtual_machine_performance_category cat = vm_perf_cats_[j];
				//this->data_estimator(cat, vms[i]->id(), ::boost::make_shared< testbed::mean_estimator<real_type> >());
				this->data_smoother(cat, vms[i]->id(), ::boost::make_shared< testbed::brown_single_exponential_smoother<real_type> >(beta_));
				//this->data_smoother(cat, vms[i]->id(), ::boost::make_shared< testbed::holt_winters_double_exponential_smoother<real_type> >(beta_));
			}
		}

		// Reset output data file
		if (p_dat_ofs_ && p_dat_ofs_->is_open())
		{
			p_dat_ofs_->close();
		}
		p_dat_ofs_.reset();
		if (!dat_fname_.empty())
		{
			p_dat_ofs_ = ::boost::make_shared< std::ofstream >(dat_fname_.c_str());
			if (!p_dat_ofs_->good())
			{
				std::ostringstream oss;
				oss << "Cannot open output data file '" << dat_fname_ << "'";

				DCS_EXCEPTION_THROW(std::runtime_error, oss.str());
			}

			*p_dat_ofs_ << "\"ts\"";

			for (std::size_t i = 0; i < nvms; ++i)
			{
				*p_dat_ofs_ << ",\"CPUCap_{" << vms[i]->id() << "}(k)\",\"CPUShare_{" << vms[i]->id() << "}(k)\""
							<< ",\"MemCap_{" << vms[i]->id() << "}(k)\",\"MemShare_{" << vms[i]->id() << "}(k)\"";
			}
			for (std::size_t i = 0; i < nvms; ++i)
			{
				*p_dat_ofs_ << ",\"CPUShare_{" << vms[i]->id() << "}(k-1)\""
							<< ",\"MemShare_{" << vms[i]->id() << "}(k-1)\"";
			}
			for (std::size_t i = 0; i < nvms; ++i)
			{
				*p_dat_ofs_ << ",\"CPUUtil_{" << vms[i]->id() << "}(k-1)\""
							<< ",\"MemUtil_{" << vms[i]->id() << "}(k-1)\"";
			}
			for (target_iterator tgt_it = this->target_values().begin(),
								 tgt_end_it = this->target_values().end();
				 tgt_it != tgt_end_it;
				 ++tgt_it)
			{
				const application_performance_category cat = tgt_it->first;

				*p_dat_ofs_ << ",\"ReferenceOutput_{" << cat << "}(k-1)\""
							<< ",\"MeasuredOutput_{" << cat << "}(k-1)\""
							<< ",\"RelativeOutputError_{" << cat << "}(k-1)\""
							<< ",\"DeltaRelativeOutputError_{" << cat << "}(k-1)\"";
			}
			for (std::size_t i = 0; i < nvms; ++i)
			{
				*p_dat_ofs_ << ",\"Cres_{" << vms[i]->id() << "}(k-1)\""
							<< ",\"Mres_{" << vms[i]->id() << "}(k-1)\"";
			}
			for (std::size_t i = 0; i < nvms; ++i)
			{
				*p_dat_ofs_ << ",\"DeltaC_{" << vms[i]->id() << "}(k)\""
							<< ",\"DeltaM_{" << vms[i]->id() << "}(k)\"";
			}
			//NOTE: C(k) and M(k) may differ from CPUShare(k) and MemShare(k) for several reasons:
			// - There is a latency in setting the new share (e.g., this is usually the case of memory, whereby the new share is not immediately set but the memory is (de)allocated incrementally)
			// - There is another component between this controller and physical resources that may change the wanted share (e.g., if a physical resource is shared among different VMs, there can be a component that try to allocate the contented physical resource fairly).
			for (std::size_t i = 0; i < nvms; ++i)
			{
				*p_dat_ofs_ << ",\"C_{" << vms[i]->id() << "}(k)\""
							<< ",\"M_{" << vms[i]->id() << "}(k)\"";
			}
			*p_dat_ofs_ << ",\"# Controls\",\"# Skip Controls\",\"# Fail Controls\"";
            *p_dat_ofs_ << ",\"Elapsed Time\"";
			*p_dat_ofs_ << std::endl;
		}
	}

	private: void do_sample()
	{
		typedef typename in_sensor_map::const_iterator in_sensor_iterator;
		typedef typename out_sensor_map::const_iterator out_sensor_iterator;
		typedef std::vector<typename sensor_type::observation_type> obs_container;
		typedef typename obs_container::const_iterator obs_iterator;

		DCS_DEBUG_TRACE("(" << this << ") BEGIN Do SAMPLE - Count: " << ctl_count_ << "/" << ctl_skip_count_ << "/" << ctl_fail_count_);

		// Collect input values
		for (in_sensor_iterator in_sens_it = in_sensors_.begin(),
								in_sens_end_it = in_sensors_.end();
			 in_sens_it != in_sens_end_it;
			 ++in_sens_it)
		{
			const virtual_machine_performance_category cat = in_sens_it->first;

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

        boost::timer::cpu_timer cpu_timer;

		const std::size_t num_vm_perf_cats = vm_perf_cats_.size();

		++ctl_count_;

		bool skip_ctl = false;

		std::map<virtual_machine_performance_category,std::vector<real_type> > old_xshares;
		std::map<virtual_machine_performance_category,std::vector<real_type> > new_xshares;
        std::map<virtual_machine_performance_category,std::vector<real_type> > xutils;
		std::map<virtual_machine_performance_category,std::vector<real_type> > xress;
		std::map< virtual_machine_performance_category, std::vector<real_type> > deltaxs;
		real_type err = std::numeric_limits<real_type>::quiet_NaN();
		real_type delta_err = std::numeric_limits<real_type>::quiet_NaN();

		std::vector<vm_pointer> vms = this->app().vms();
		const std::size_t nvms = vms.size();

		for (std::size_t i = 0; i < nvms; ++i)
		{
			for (std::size_t j = 0; j < num_vm_perf_cats; ++j)
			{
				const virtual_machine_performance_category cat = vm_perf_cats_[j];
				const vm_pointer p_vm = vms[i];

//				if (this->data_estimator(cat, vms[i]->id()).count() > 0)
//				{
//					//const real_type uh = this->data_estimator(cat, p_vm->id()).estimate();
//					const real_type uh = this->data_smoother(cat, p_vm->id()).forecast(0);
//					const real_type c = p_vm->cpu_share();
//
//					xress[cat].push_back(c-uh);
//				}
//				else
//				{
//					// No observation collected during the last control interval
//					DCS_DEBUG_TRACE("No input observation collected during the last control interval -> Skip control");
//					skip_ctl = true;
//					break;
//				}
				const real_type uh = this->data_smoother(cat, p_vm->id()).forecast(0);
				real_type c = 0;
				switch (cat)
				{
					case cpu_util_virtual_machine_performance:
						c = p_vm->cpu_share();
						break;
					case memory_util_virtual_machine_performance:
						c = p_vm->memory_share();
						break;
				}
				const real_type xres = c-uh;

				xress[cat].push_back(xres);
				old_xshares[cat].push_back(c);
				xutils[cat].push_back(uh);
DCS_DEBUG_TRACE("VM " << p_vm->id() << " - Performance Category: " << cat << " - Uhat(k): " << uh << " - C(k): " << c << " -> Cres(k+1): " << xres << " (Relative Cres(k+1): " << xres/c << ")");//XXX
			}
		}

		if (!skip_ctl)
		{
			for (target_iterator tgt_it = this->target_values().begin(),
							 	 tgt_end_it = this->target_values().end();
				 tgt_it != tgt_end_it;
				 ++tgt_it)
			{
				const application_performance_category cat = tgt_it->first;

				// Compute a summary statistics of collected observation
				if (this->data_estimator(cat).count() > 0)
				{
					const real_type yh = this->data_estimator(cat).estimate();
					const real_type yr = this->target_value(cat);

					switch (cat)
					{
						case response_time_application_performance:
							err = (yr-yh)/yr;
							break;
						case throughput_application_performance:
							err = (yh-yr)/yr;
							break;
					}
					if (std::isnan(last_err_))
					{
						//delta_err = 0;
						delta_err = std::numeric_limits<real_type>::quiet_NaN();
					}
					else
					{
						delta_err = err-last_err_;
					}
					last_err_ = err;
DCS_DEBUG_TRACE("APP Performance Category: " << cat << " - Yhat(k): " << yh << " - R: " << yr << " -> E(k+1): " << err << " - DeltaE(k+1): " << delta_err);//XXX

					if (std::isnan(delta_err))
					{
						DCS_DEBUG_TRACE("No output delta error available -> Skip control");
						skip_ctl = true;
						break;
					}
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

		// Skip control until we see enough observations.
		// This should give enough time to let the estimated performance metric
		// (e.g., 95th percentile of response time) stabilize
		if (ctl_count_ <= control_warmup_size)
		{
			skip_ctl = true;
		}

        if (!skip_ctl)
        {
			// Perform fuzzy control
			bool ok = false;
			try
			{
				for (std::size_t i = 0; i < nvms; ++i)
				{
					for (std::size_t j = 0; j < num_vm_perf_cats; ++j)
					{
						const virtual_machine_performance_category vm_cat = vm_perf_cats_[j];
						const real_type xres = xress.at(vm_cat)[i];
						const real_type old_xshare = old_xshares.at(vm_cat)[i];

						switch (vm_cat)
						{
							case cpu_util_virtual_machine_performance:
								p_fuzzy_eng_->setInputValue(cres_fuzzy_var_name, xres/old_xshare);
								//p_fuzzy_eng_->getOutputVariable(deltac_fuzzy_var_name)->setMinimum(-xres/old_xshare);
								//p_fuzzy_eng_->getOutputVariable(deltac_fuzzy_var_name)->setMaximum(1-old_xshare);
								break;
							case memory_util_virtual_machine_performance:
								p_fuzzy_eng_->setInputValue(mres_fuzzy_var_name, xres/old_xshare);
								//p_fuzzy_eng_->getOutputVariable(deltam_fuzzy_var_name)->setMinimum(-xres/old_xshare);
								//p_fuzzy_eng_->getOutputVariable(deltam_fuzzy_var_name)->setMaximum(1-old_xshare);
								break;
						}
					}

					p_fuzzy_eng_->setInputValue(err_fuzzy_var_name, err);
					p_fuzzy_eng_->setInputValue(deltaerr_fuzzy_var_name, delta_err);

					p_fuzzy_eng_->process();

DCS_DEBUG_TRACE("FUZZY OUTPUT '" << deltam_fuzzy_var_name << "' - VALUE: " << p_fuzzy_eng_->getOutputVariable(deltam_fuzzy_var_name)->getValue() << " - FUZZY OUTPUT VALUE: " << p_fuzzy_eng_->getOutputVariable(deltam_fuzzy_var_name)->fuzzyOutputValue());//XXX

					for (std::size_t j = 0; j < num_vm_perf_cats; ++j)
					{
						const virtual_machine_performance_category cat = vm_perf_cats_[j];
						const real_type old_xshare = old_xshares.at(cat)[i];
						const real_type xutil = xutils.at(cat)[i];
						//const real_type deltax_lb = -xress.at(cat)[i]/old_xshare;
                        const real_type deltax_lb = std::min(1.0, xutil*resource_share_lb_scale_factor)-old_xshare;
                        const real_type deltax_ub = std::max(0.0, 1-old_xshare);

						real_type fuzzy_deltax = 0;
						real_type deltax = 0;
						switch (cat)
						{
							case cpu_util_virtual_machine_performance:
								fuzzy_deltax = p_fuzzy_eng_->getOutputValue(deltac_fuzzy_var_name);
								break;
							case memory_util_virtual_machine_performance:
								fuzzy_deltax = p_fuzzy_eng_->getOutputValue(deltam_fuzzy_var_name);
								break;
						}
						deltax = dcs::math::clamp(fuzzy_deltax, deltax_lb, deltax_ub);

						deltaxs[cat].push_back(deltax);
DCS_DEBUG_TRACE("VM " << vms[i]->id() << ", Performance Category: " << cat << " -> DeltaX(k+1): " << deltaxs.at(cat).at(i) << " (computed: " << fuzzy_deltax << ", lb: " << deltax_lb << ", ub: " << deltax_ub << ")");//XXX
					}
				}

				ok = true;
			}
			catch (fl::Exception const& fe)
			{
				DCS_DEBUG_TRACE( "Caught exception: " << fe.what() );

				std::ostringstream oss;
				oss << "Unable to compute optimal control: " << fe.what();
				::dcs::log_warn(DCS_LOGGING_AT, oss.str());
			}
			catch (std::exception const& e)
			{
				DCS_DEBUG_TRACE( "Caught exception: " << e.what() );

				std::ostringstream oss;
				oss << "Unable to compute optimal control: " << e.what();
				::dcs::log_warn(DCS_LOGGING_AT, oss.str());
			}

			// Apply fuzzy control results
			if (ok)
			{
				for (std::size_t i = 0; i < nvms; ++i)
				{
					vm_pointer p_vm = vms[i];

					for (std::size_t j = 0; j < num_vm_perf_cats; ++j)
					{
						const virtual_machine_performance_category cat = vm_perf_cats_[j];

						//real_type old_share = 0;
						//switch (cat)
						//{
						//	case cpu_util_virtual_machine_performance:
						//		old_share = p_vm->cpu_share();
						//		break;
						//	case memory_util_virtual_machine_performance:
						//		old_share = p_vm->memory_share();
						//		break;
						//}
						////old_xshares[cat].push_back(old_share);
						const real_type old_share = old_xshares.at(cat)[i];

						const real_type new_share = std::max(std::min(old_share+deltaxs[cat][i], 1.0), 0.0);

						DCS_DEBUG_TRACE("VM '" << p_vm->id() << "' - Performance Category: " << cat << " - old-share: " << old_share << " - new-share: " << new_share);

						if (std::isfinite(new_share) && !::dcs::math::float_traits<real_type>::essentially_equal(old_share, new_share))
						{
							switch (cat)
							{
								case cpu_util_virtual_machine_performance:
									p_vm->cpu_share(new_share);
									break;
								case memory_util_virtual_machine_performance:
									p_vm->memory_share(new_share);
									break;
							}
DCS_DEBUG_TRACE("VM " << vms[i]->id() << ", Performance Category: " << cat << " -> C(k+1): " << new_share);//XXX

							new_xshares[cat].push_back(new_share);
						}
						else
						{
DCS_DEBUG_TRACE("VM " << vms[i]->id() << ", Performance Category: " << cat << " -> C(k+1) not set!");//XXX
							new_xshares[cat].push_back(old_share);
						}
					}
				}
DCS_DEBUG_TRACE("Control applied");//XXX
			}
			else
			{
				++ctl_fail_count_;

				std::ostringstream oss;
				oss << "Control not applied: failed to solve the control problem";
				::dcs::log_warn(DCS_LOGGING_AT, oss.str());
			}
		}
		else
		{
			++ctl_skip_count_;
		}

        cpu_timer.stop();

		// Export to file
		if (p_dat_ofs_)
		{
			// Initialize data structures if needed

			if (new_xshares.size() == 0)
			{
				for (::std::size_t i = 0; i < nvms; ++i)
				{
					const vm_pointer p_vm = vms[i];

					// check: p_vm != null
					DCS_DEBUG_ASSERT( p_vm );

					new_xshares[cpu_util_virtual_machine_performance].push_back(p_vm->cpu_share());
					new_xshares[memory_util_virtual_machine_performance].push_back(p_vm->memory_share());
				}
			}
			if (old_xshares.size() == 0)
			{
				for (::std::size_t i = 0; i < nvms; ++i)
				{
					const vm_pointer p_vm = vms[i];

					// check: p_vm != null
					DCS_DEBUG_ASSERT( p_vm );

					old_xshares[cpu_util_virtual_machine_performance].push_back(p_vm->cpu_share());
					old_xshares[memory_util_virtual_machine_performance].push_back(p_vm->memory_share());
				}
			}
			if (deltaxs.size() == 0)
			{
				for (std::size_t j = 0; j < num_vm_perf_cats; ++j)
				{
					const virtual_machine_performance_category vm_cat = vm_perf_cats_[j];

					deltaxs[vm_cat].assign(nvms, std::numeric_limits<real_type>::quiet_NaN());
				}
			}
			if (xress.size() == 0)
			{
				for (std::size_t j = 0; j < num_vm_perf_cats; ++j)
				{
					const virtual_machine_performance_category vm_cat = vm_perf_cats_[j];

					xress[vm_cat].assign(nvms, std::numeric_limits<real_type>::quiet_NaN());
				}
			}
			if (xutils.size() == 0)
			{
				for (std::size_t j = 0; j < num_vm_perf_cats; ++j)
				{
					const virtual_machine_performance_category vm_cat = vm_perf_cats_[j];

					xutils[vm_cat].assign(nvms, std::numeric_limits<real_type>::quiet_NaN());
				}
			}

			// Write to output stream

			*p_dat_ofs_ << std::time(0) << ",";
			for (std::size_t i = 0; i < nvms; ++i)
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
                *p_dat_ofs_ << old_xshares.at(cpu_util_virtual_machine_performance)[i]
                            << "," << old_xshares.at(memory_util_virtual_machine_performance)[i];
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
				//*p_dat_ofs_ << this->data_smoother(cpu_util_virtual_machine_performance, p_vm->id()).forecast(0)
				//			<< "," << this->data_smoother(memory_util_virtual_machine_performance, p_vm->id()).forecast(0);
				for (std::size_t j = 0; j < num_vm_perf_cats; ++j)
				{
					const virtual_machine_performance_category vm_cat = vm_perf_cats_[j];

                    if (j != 0)
                    {
                        *p_dat_ofs_ << ",";
                    }
					//*p_dat_ofs_ << this->data_smoother(vm_cat, p_vm->id()).forecast(0);
					*p_dat_ofs_ << xutils.at(vm_cat)[i];
				}
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
				*p_dat_ofs_ << yr << "," << yh << "," << err << "," << delta_err;
			}
			for (std::size_t i = 0; i < nvms; ++i)
			{
				for (std::size_t j = 0; j < num_vm_perf_cats; ++j)
				{
					const virtual_machine_performance_category vm_cat = vm_perf_cats_[j];
					const real_type xres = xress.at(vm_cat).at(i);

					*p_dat_ofs_ << "," << xres;
				}
			}
			for (std::size_t i = 0; i < nvms; ++i)
			{
				for (std::size_t j = 0; j < num_vm_perf_cats; ++j)
				{
					const virtual_machine_performance_category vm_cat = vm_perf_cats_[j];
					const real_type deltax = deltaxs.at(vm_cat).at(i);

					*p_dat_ofs_ << "," << deltax;
				}
			}
			for (std::size_t i = 0; i < nvms; ++i)
			{
				for (std::size_t j = 0; j < num_vm_perf_cats; ++j)
				{
					const virtual_machine_performance_category vm_cat = vm_perf_cats_[j];
					const real_type xshare = new_xshares.at(vm_cat).at(i);

					*p_dat_ofs_ << "," << xshare;
				}
			}
			*p_dat_ofs_ << "," << ctl_count_ << "," << ctl_skip_count_ << "," << ctl_fail_count_;
            *p_dat_ofs_ << "," << (cpu_timer.elapsed().user+cpu_timer.elapsed().system);
			*p_dat_ofs_ << std::endl;
		}

		DCS_DEBUG_TRACE("(" << this << ") END Do CONTROL - Count: " << ctl_count_ << "/" << ctl_skip_count_ << "/" << ctl_fail_count_);
	}


	private: real_type beta_; ///< The EWMA smoothing factor for resource utilizations
	private: ::boost::shared_ptr<fl::Engine> p_fuzzy_eng_; ///< The fuzzy control engine
	private: real_type last_err_; ///< The last performance relative error collected from the controlled system
	private: std::size_t ctl_count_; ///< Number of times control function has been invoked
	private: std::size_t ctl_skip_count_; ///< Number of times control has been skipped
	private: std::size_t ctl_fail_count_; ///< Number of times control has failed
	private: in_sensor_map in_sensors_;
	private: out_sensor_map out_sensors_;
	private: std::string dat_fname_;
	private: ::boost::shared_ptr< std::ofstream > p_dat_ofs_;
	private: std::vector<virtual_machine_performance_category> vm_perf_cats_;
	//private: std::vector<application_performance_category> app_perf_cats_;
}; // anglano2014_fc2q_mimo_v2_application_manager

template <typename T>
const std::size_t anglano2014_fc2q_mimo_v2_application_manager<T>::control_warmup_size = 5;

template <typename T>
const float anglano2014_fc2q_mimo_v2_application_manager<T>::resource_share_lb_scale_factor = 1.1;

template <typename T>
const std::string anglano2014_fc2q_mimo_v2_application_manager<T>::err_fuzzy_var_name = "E";

template <typename T>
const std::string anglano2014_fc2q_mimo_v2_application_manager<T>::deltaerr_fuzzy_var_name = "DeltaE";

template <typename T>
const std::string anglano2014_fc2q_mimo_v2_application_manager<T>::cres_fuzzy_var_name = "Cres";

template <typename T>
const std::string anglano2014_fc2q_mimo_v2_application_manager<T>::deltac_fuzzy_var_name = "DeltaC";

template <typename T>
const std::string anglano2014_fc2q_mimo_v2_application_manager<T>::mres_fuzzy_var_name = "Mres";

template <typename T>
const std::string anglano2014_fc2q_mimo_v2_application_manager<T>::deltam_fuzzy_var_name = "DeltaM";

}} // Namespace dcs::testbed

#endif // DCS_TESTBED_ANGLANO2014_FC2Q_MIMO_V2_APPLICATION_MANAGER_HPP
