/**
 * \file dcs/testbed/rao2013_dynaqos_application_manager.hpp
 *
 * \brief Application manager based on the work by (RAO et al., 2013)
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

#ifndef DCS_TESTBED_RAO2013_DYNAQOS_APPLICATION_MANAGER_HPP
#define DCS_TESTBED_RAO2013_DYNAQOS_APPLICATION_MANAGER_HPP


#include <boost/smart_ptr.hpp>
#include <cmath>
#include <cstddef>
#include <dcs/assert.hpp>
#include <dcs/debug.hpp>
#include <dcs/exception.hpp>
#include <dcs/logging.hpp>
#include <dcs/macro.hpp>
#include <dcs/math/traits/float.hpp>
#include <dcs/testbed/application_performance_category.hpp>
#include <dcs/testbed/base_application_manager.hpp>
#include <dcs/testbed/data_smoothers.hpp>
#include <dcs/testbed/virtual_machine_performance_category.hpp>
#include <fl/Headers.h>
#include <fstream>
#include <limits>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>


namespace dcs { namespace testbed {

/**
 * \brief Application manager based on the work by (Rao et al., 2013)
 * 
 * \tparam Traits The traits type
 *
 * This class implements the DynaQoS framework proposed in [1,2].
 *
 * References:
 * -# J. Rao and Y. Wei and J. Gong and C.-Z. Xu,
 *    "QoS Guarantees and Service Differentiation for Dynamic Cloud Applications,"
 *    IEEE Transactions on Network and Service Management 10(1):43-55, 2013.
 * -# J. Wei and C.-Z. Xu,
 *    "eQoS: Provisioning of Client-Perceived End-to-End QoS Guarantees in Web Servers,"
 *    IEEE Transactions on Computers 55(12):1543-1556, 2006.
 * .
 *
 * \author Marco Guazzone (marco.guazzone@gmail.com)
 */
template <typename TraitsT>
class rao2013_dynaqos_application_manager: public base_application_manager<TraitsT>
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


	private: static const ::std::string alpha_fuzzy_var_name;
	private: static const ::std::string e_fuzzy_var_name;
	private: static const ::std::string de_fuzzy_var_name;
	private: static const ::std::string du_fuzzy_var_name;


	public: rao2013_dynaqos_application_manager()
	: gamma_(0.8),
	  Ke_(0),
	  Kde_(0),
	  p_rc_fuzzy_eng_(new ::fl::Engine()),
	  p_sfc_fuzzy_eng_(new ::fl::Engine())
	{
		init();
	}

	public: void discount_factor(real_type value)
	{
		gamma_ = value;
	}

	public: real_type discount_factor() const
	{
		return gamma_;
	}

	public: void export_data_to(::std::string const& fname)
	{
		dat_fname_ = fname;
	}

	private: void init()
	{
		DCS_DEBUG_ASSERT( p_rc_fuzzy_eng_ );
		DCS_DEBUG_ASSERT( p_sfc_fuzzy_eng_ );

		const real_type one_third = 1.0/3.0;
		const real_type two_third = 2.0/3.0;
		const real_type one_sixth = 1.0/6.0;
		const real_type five_sixth = 5.0/6.0;

		::fl::InputVariable* p_iv = 0;
		::fl::OutputVariable* p_ov = 0;
		::fl::RuleBlock* p_rules = 0;

		// Setup the Resource Controller
		//   Membership functions taken from (Wei et al.,2006) [2]

		for (::std::size_t i = 0; i < 2; ++i)
		{
			p_iv = new ::fl::InputVariable();
			p_iv->setEnabled(true);
			switch (i)
			{
				case 0: // \Delta e(k)
					p_iv->setName(de_fuzzy_var_name);
					break;
				case 1: // e(k)
					p_iv->setName(e_fuzzy_var_name);
					break;
			}
			p_iv->setRange(-1, 1);
			p_iv->addTerm(new ::fl::Ramp("NL", -two_third, -1));
			p_iv->addTerm(new ::fl::Triangle("NM", -1, -two_third, -one_third));
			p_iv->addTerm(new ::fl::Triangle("NS", -two_third, -one_third, 0));
			p_iv->addTerm(new ::fl::Triangle("ZE", -one_third, 0, one_third));
			p_iv->addTerm(new ::fl::Triangle("PS", 0, one_third, two_third));
			p_iv->addTerm(new ::fl::Triangle("PM", one_third, two_third, 1));
			p_iv->addTerm(new ::fl::Ramp("PL", two_third, 1));
			p_rc_fuzzy_eng_->addInputVariable(p_iv);
		}

		p_ov = new ::fl::OutputVariable();
		p_ov->setEnabled(true);
		p_ov->setName(du_fuzzy_var_name);
		p_ov->setRange(-1, 1);
		p_ov->fuzzyOutput()->setAccumulation(new ::fl::Maximum());
		p_ov->setDefuzzifier(new ::fl::Centroid());
		p_ov->setDefaultValue(::fl::nan);
		p_ov->setLockPreviousValue(false);
		p_ov->addTerm(new ::fl::Ramp("NL", -two_third, -1));
		p_ov->addTerm(new ::fl::Triangle("NM", -1, -two_third, -one_third));
		p_ov->addTerm(new ::fl::Triangle("NS", -two_third, -one_third, 0));
		p_ov->addTerm(new ::fl::Triangle("ZE", -one_third, 0, one_third));
		p_ov->addTerm(new ::fl::Triangle("PS", 0, one_third, two_third));
		p_ov->addTerm(new ::fl::Triangle("PM", one_third, two_third, 1));
		p_ov->addTerm(new ::fl::Ramp("PL", two_third, 1));
		p_rc_fuzzy_eng_->addOutputVariable(p_ov);

		p_rules = new ::fl::RuleBlock();
		p_rules->setEnabled(true);
		p_rules->setConjunction(new ::fl::Minimum());
		p_rules->setDisjunction(new ::fl::Maximum());
		p_rules->setImplication(new ::fl::Minimum());
		p_rules->addRule(::fl::Rule::parse("if " + e_fuzzy_var_name + " is NL and " + de_fuzzy_var_name + " is NL then " + du_fuzzy_var_name + " is PL", p_rc_fuzzy_eng_.get()));
		p_rules->addRule(::fl::Rule::parse("if " + e_fuzzy_var_name + " is NL and " + de_fuzzy_var_name + " is NM then " + du_fuzzy_var_name + " is PL", p_rc_fuzzy_eng_.get()));
		p_rules->addRule(::fl::Rule::parse("if " + e_fuzzy_var_name + " is NL and " + de_fuzzy_var_name + " is NS then " + du_fuzzy_var_name + " is PL", p_rc_fuzzy_eng_.get()));
		p_rules->addRule(::fl::Rule::parse("if " + e_fuzzy_var_name + " is NL and " + de_fuzzy_var_name + " is ZE then " + du_fuzzy_var_name + " is PL", p_rc_fuzzy_eng_.get()));
		p_rules->addRule(::fl::Rule::parse("if " + e_fuzzy_var_name + " is NL and " + de_fuzzy_var_name + " is PS then " + du_fuzzy_var_name + " is PM", p_rc_fuzzy_eng_.get()));
		p_rules->addRule(::fl::Rule::parse("if " + e_fuzzy_var_name + " is NL and " + de_fuzzy_var_name + " is PM then " + du_fuzzy_var_name + " is PS", p_rc_fuzzy_eng_.get()));
		p_rules->addRule(::fl::Rule::parse("if " + e_fuzzy_var_name + " is NL and " + de_fuzzy_var_name + " is PL then " + du_fuzzy_var_name + " is ZE", p_rc_fuzzy_eng_.get()));
		p_rules->addRule(::fl::Rule::parse("if " + e_fuzzy_var_name + " is NM and " + de_fuzzy_var_name + " is NL then " + du_fuzzy_var_name + " is PL", p_rc_fuzzy_eng_.get()));
		p_rules->addRule(::fl::Rule::parse("if " + e_fuzzy_var_name + " is NM and " + de_fuzzy_var_name + " is NM then " + du_fuzzy_var_name + " is PL", p_rc_fuzzy_eng_.get()));
		p_rules->addRule(::fl::Rule::parse("if " + e_fuzzy_var_name + " is NM and " + de_fuzzy_var_name + " is NS then " + du_fuzzy_var_name + " is PL", p_rc_fuzzy_eng_.get()));
		p_rules->addRule(::fl::Rule::parse("if " + e_fuzzy_var_name + " is NM and " + de_fuzzy_var_name + " is ZE then " + du_fuzzy_var_name + " is PM", p_rc_fuzzy_eng_.get()));
		p_rules->addRule(::fl::Rule::parse("if " + e_fuzzy_var_name + " is NM and " + de_fuzzy_var_name + " is PS then " + du_fuzzy_var_name + " is PS", p_rc_fuzzy_eng_.get()));
		p_rules->addRule(::fl::Rule::parse("if " + e_fuzzy_var_name + " is NM and " + de_fuzzy_var_name + " is PM then " + du_fuzzy_var_name + " is ZE", p_rc_fuzzy_eng_.get()));
		p_rules->addRule(::fl::Rule::parse("if " + e_fuzzy_var_name + " is NM and " + de_fuzzy_var_name + " is PL then " + du_fuzzy_var_name + " is NS", p_rc_fuzzy_eng_.get()));
		p_rules->addRule(::fl::Rule::parse("if " + e_fuzzy_var_name + " is NS and " + de_fuzzy_var_name + " is NL then " + du_fuzzy_var_name + " is PL", p_rc_fuzzy_eng_.get()));
		p_rules->addRule(::fl::Rule::parse("if " + e_fuzzy_var_name + " is NS and " + de_fuzzy_var_name + " is NM then " + du_fuzzy_var_name + " is PL", p_rc_fuzzy_eng_.get()));
		p_rules->addRule(::fl::Rule::parse("if " + e_fuzzy_var_name + " is NS and " + de_fuzzy_var_name + " is NS then " + du_fuzzy_var_name + " is PM", p_rc_fuzzy_eng_.get()));
		p_rules->addRule(::fl::Rule::parse("if " + e_fuzzy_var_name + " is NS and " + de_fuzzy_var_name + " is ZE then " + du_fuzzy_var_name + " is PS", p_rc_fuzzy_eng_.get()));
		p_rules->addRule(::fl::Rule::parse("if " + e_fuzzy_var_name + " is NS and " + de_fuzzy_var_name + " is PS then " + du_fuzzy_var_name + " is ZE", p_rc_fuzzy_eng_.get()));
		p_rules->addRule(::fl::Rule::parse("if " + e_fuzzy_var_name + " is NS and " + de_fuzzy_var_name + " is PM then " + du_fuzzy_var_name + " is NS", p_rc_fuzzy_eng_.get()));
		p_rules->addRule(::fl::Rule::parse("if " + e_fuzzy_var_name + " is NS and " + de_fuzzy_var_name + " is PL then " + du_fuzzy_var_name + " is NM", p_rc_fuzzy_eng_.get()));
		p_rules->addRule(::fl::Rule::parse("if " + e_fuzzy_var_name + " is ZE and " + de_fuzzy_var_name + " is NL then " + du_fuzzy_var_name + " is PL", p_rc_fuzzy_eng_.get()));
		p_rules->addRule(::fl::Rule::parse("if " + e_fuzzy_var_name + " is ZE and " + de_fuzzy_var_name + " is NM then " + du_fuzzy_var_name + " is PM", p_rc_fuzzy_eng_.get()));
		p_rules->addRule(::fl::Rule::parse("if " + e_fuzzy_var_name + " is ZE and " + de_fuzzy_var_name + " is NS then " + du_fuzzy_var_name + " is PS", p_rc_fuzzy_eng_.get()));
		p_rules->addRule(::fl::Rule::parse("if " + e_fuzzy_var_name + " is ZE and " + de_fuzzy_var_name + " is ZE then " + du_fuzzy_var_name + " is ZE", p_rc_fuzzy_eng_.get()));
		p_rules->addRule(::fl::Rule::parse("if " + e_fuzzy_var_name + " is ZE and " + de_fuzzy_var_name + " is PS then " + du_fuzzy_var_name + " is NS", p_rc_fuzzy_eng_.get()));
		p_rules->addRule(::fl::Rule::parse("if " + e_fuzzy_var_name + " is ZE and " + de_fuzzy_var_name + " is PM then " + du_fuzzy_var_name + " is NM", p_rc_fuzzy_eng_.get()));
		p_rules->addRule(::fl::Rule::parse("if " + e_fuzzy_var_name + " is ZE and " + de_fuzzy_var_name + " is PL then " + du_fuzzy_var_name + " is NL", p_rc_fuzzy_eng_.get()));
		p_rules->addRule(::fl::Rule::parse("if " + e_fuzzy_var_name + " is PS and " + de_fuzzy_var_name + " is NL then " + du_fuzzy_var_name + " is PM", p_rc_fuzzy_eng_.get()));
		p_rules->addRule(::fl::Rule::parse("if " + e_fuzzy_var_name + " is PS and " + de_fuzzy_var_name + " is NM then " + du_fuzzy_var_name + " is PS", p_rc_fuzzy_eng_.get()));
		p_rules->addRule(::fl::Rule::parse("if " + e_fuzzy_var_name + " is PS and " + de_fuzzy_var_name + " is NS then " + du_fuzzy_var_name + " is ZE", p_rc_fuzzy_eng_.get()));
		p_rules->addRule(::fl::Rule::parse("if " + e_fuzzy_var_name + " is PS and " + de_fuzzy_var_name + " is ZE then " + du_fuzzy_var_name + " is NS", p_rc_fuzzy_eng_.get()));
		p_rules->addRule(::fl::Rule::parse("if " + e_fuzzy_var_name + " is PS and " + de_fuzzy_var_name + " is PS then " + du_fuzzy_var_name + " is NM", p_rc_fuzzy_eng_.get()));
		p_rules->addRule(::fl::Rule::parse("if " + e_fuzzy_var_name + " is PS and " + de_fuzzy_var_name + " is PM then " + du_fuzzy_var_name + " is NL", p_rc_fuzzy_eng_.get()));
		p_rules->addRule(::fl::Rule::parse("if " + e_fuzzy_var_name + " is PS and " + de_fuzzy_var_name + " is PL then " + du_fuzzy_var_name + " is NL", p_rc_fuzzy_eng_.get()));
		p_rules->addRule(::fl::Rule::parse("if " + e_fuzzy_var_name + " is PM and " + de_fuzzy_var_name + " is NL then " + du_fuzzy_var_name + " is PS", p_rc_fuzzy_eng_.get()));
		p_rules->addRule(::fl::Rule::parse("if " + e_fuzzy_var_name + " is PM and " + de_fuzzy_var_name + " is NM then " + du_fuzzy_var_name + " is ZE", p_rc_fuzzy_eng_.get()));
		p_rules->addRule(::fl::Rule::parse("if " + e_fuzzy_var_name + " is PM and " + de_fuzzy_var_name + " is NS then " + du_fuzzy_var_name + " is NS", p_rc_fuzzy_eng_.get()));
		p_rules->addRule(::fl::Rule::parse("if " + e_fuzzy_var_name + " is PM and " + de_fuzzy_var_name + " is ZE then " + du_fuzzy_var_name + " is NM", p_rc_fuzzy_eng_.get()));
		p_rules->addRule(::fl::Rule::parse("if " + e_fuzzy_var_name + " is PM and " + de_fuzzy_var_name + " is PS then " + du_fuzzy_var_name + " is NL", p_rc_fuzzy_eng_.get()));
		p_rules->addRule(::fl::Rule::parse("if " + e_fuzzy_var_name + " is PM and " + de_fuzzy_var_name + " is PM then " + du_fuzzy_var_name + " is NL", p_rc_fuzzy_eng_.get()));
		p_rules->addRule(::fl::Rule::parse("if " + e_fuzzy_var_name + " is PM and " + de_fuzzy_var_name + " is PL then " + du_fuzzy_var_name + " is NL", p_rc_fuzzy_eng_.get()));
		p_rules->addRule(::fl::Rule::parse("if " + e_fuzzy_var_name + " is PL and " + de_fuzzy_var_name + " is NL then " + du_fuzzy_var_name + " is ZE", p_rc_fuzzy_eng_.get()));
		p_rules->addRule(::fl::Rule::parse("if " + e_fuzzy_var_name + " is PL and " + de_fuzzy_var_name + " is NM then " + du_fuzzy_var_name + " is NS", p_rc_fuzzy_eng_.get()));
		p_rules->addRule(::fl::Rule::parse("if " + e_fuzzy_var_name + " is PL and " + de_fuzzy_var_name + " is NS then " + du_fuzzy_var_name + " is NM", p_rc_fuzzy_eng_.get()));
		p_rules->addRule(::fl::Rule::parse("if " + e_fuzzy_var_name + " is PL and " + de_fuzzy_var_name + " is ZE then " + du_fuzzy_var_name + " is NL", p_rc_fuzzy_eng_.get()));
		p_rules->addRule(::fl::Rule::parse("if " + e_fuzzy_var_name + " is PL and " + de_fuzzy_var_name + " is PS then " + du_fuzzy_var_name + " is NL", p_rc_fuzzy_eng_.get()));
		p_rules->addRule(::fl::Rule::parse("if " + e_fuzzy_var_name + " is PL and " + de_fuzzy_var_name + " is PM then " + du_fuzzy_var_name + " is NL", p_rc_fuzzy_eng_.get()));
		p_rules->addRule(::fl::Rule::parse("if " + e_fuzzy_var_name + " is PL and " + de_fuzzy_var_name + " is PL then " + du_fuzzy_var_name + " is NL", p_rc_fuzzy_eng_.get()));
		p_rc_fuzzy_eng_->addRuleBlock(p_rules);

		// Setup the Scaling Factor Controller
		//   Membership functions taken from (Wei et al.,2006) [2]

		for (::std::size_t i = 0; i < 2; ++i)
		{
			p_iv = new ::fl::InputVariable();
			p_iv->setEnabled(true);
			switch (i)
			{
				case 0: // \Delta e(k)
					p_iv->setName(de_fuzzy_var_name);
					break;
				case 1: // e(k)
					p_iv->setName(e_fuzzy_var_name);
					break;
			}
			p_iv->setRange(-1, 1);
			p_iv->addTerm(new ::fl::Ramp("NL", -two_third, -1));
			p_iv->addTerm(new ::fl::Triangle("NM", -1, -two_third, -one_third));
			p_iv->addTerm(new ::fl::Triangle("NS", -two_third, -one_third, 0));
			p_iv->addTerm(new ::fl::Triangle("ZE", -one_third, 0, one_third));
			p_iv->addTerm(new ::fl::Triangle("PS", 0, one_third, two_third));
			p_iv->addTerm(new ::fl::Triangle("PM", one_third, two_third, 1));
			p_iv->addTerm(new ::fl::Ramp("PL", two_third, 1));
			p_sfc_fuzzy_eng_->addInputVariable(p_iv);
		}

		p_ov = new ::fl::OutputVariable();
		p_ov->setEnabled(true);
		p_ov->setName(alpha_fuzzy_var_name);
		p_ov->setRange(0, 1);
		p_ov->fuzzyOutput()->setAccumulation(new ::fl::Maximum());
		p_ov->setDefuzzifier(new ::fl::Centroid());
		p_ov->setDefaultValue(::fl::nan);
		p_ov->setLockPreviousValue(false);
		p_ov->addTerm(new ::fl::Ramp("ZE", one_sixth, 0));
		p_ov->addTerm(new ::fl::Triangle("VS", 0, one_sixth, one_third));
		p_ov->addTerm(new ::fl::Triangle("SM", one_sixth, one_third, 0.5));
		p_ov->addTerm(new ::fl::Triangle("SL", one_third, 0.5, two_third));
		p_ov->addTerm(new ::fl::Triangle("ML", 0.5, two_third, five_sixth));
		p_ov->addTerm(new ::fl::Triangle("LG", two_third, five_sixth, 1));
		p_ov->addTerm(new ::fl::Ramp("VL", five_sixth, 1));
		p_sfc_fuzzy_eng_->addOutputVariable(p_ov);

		p_rules = new ::fl::RuleBlock();
		p_rules->setEnabled(true);
		p_rules->setConjunction(new ::fl::Minimum());
		p_rules->setDisjunction(new ::fl::Maximum());
		p_rules->setImplication(new ::fl::Minimum());
		p_rules->addRule(::fl::Rule::parse("if " + e_fuzzy_var_name + " is NL and " + de_fuzzy_var_name + " is NL then " + alpha_fuzzy_var_name + " is VL", p_sfc_fuzzy_eng_.get()));
		p_rules->addRule(::fl::Rule::parse("if " + e_fuzzy_var_name + " is NL and " + de_fuzzy_var_name + " is NM then " + alpha_fuzzy_var_name + " is VL", p_sfc_fuzzy_eng_.get()));
		p_rules->addRule(::fl::Rule::parse("if " + e_fuzzy_var_name + " is NL and " + de_fuzzy_var_name + " is NS then " + alpha_fuzzy_var_name + " is VL", p_sfc_fuzzy_eng_.get()));
		p_rules->addRule(::fl::Rule::parse("if " + e_fuzzy_var_name + " is NL and " + de_fuzzy_var_name + " is ZE then " + alpha_fuzzy_var_name + " is SM", p_sfc_fuzzy_eng_.get()));
		p_rules->addRule(::fl::Rule::parse("if " + e_fuzzy_var_name + " is NL and " + de_fuzzy_var_name + " is PS then " + alpha_fuzzy_var_name + " is VS", p_sfc_fuzzy_eng_.get()));
		p_rules->addRule(::fl::Rule::parse("if " + e_fuzzy_var_name + " is NL and " + de_fuzzy_var_name + " is PM then " + alpha_fuzzy_var_name + " is VS", p_sfc_fuzzy_eng_.get()));
		p_rules->addRule(::fl::Rule::parse("if " + e_fuzzy_var_name + " is NL and " + de_fuzzy_var_name + " is PL then " + alpha_fuzzy_var_name + " is ZE", p_sfc_fuzzy_eng_.get()));
		p_rules->addRule(::fl::Rule::parse("if " + e_fuzzy_var_name + " is NM and " + de_fuzzy_var_name + " is NL then " + alpha_fuzzy_var_name + " is VL", p_sfc_fuzzy_eng_.get()));
		p_rules->addRule(::fl::Rule::parse("if " + e_fuzzy_var_name + " is NM and " + de_fuzzy_var_name + " is NM then " + alpha_fuzzy_var_name + " is VL", p_sfc_fuzzy_eng_.get()));
		p_rules->addRule(::fl::Rule::parse("if " + e_fuzzy_var_name + " is NM and " + de_fuzzy_var_name + " is NS then " + alpha_fuzzy_var_name + " is LG", p_sfc_fuzzy_eng_.get()));
		p_rules->addRule(::fl::Rule::parse("if " + e_fuzzy_var_name + " is NM and " + de_fuzzy_var_name + " is ZE then " + alpha_fuzzy_var_name + " is SL", p_sfc_fuzzy_eng_.get()));
		p_rules->addRule(::fl::Rule::parse("if " + e_fuzzy_var_name + " is NM and " + de_fuzzy_var_name + " is PS then " + alpha_fuzzy_var_name + " is SM", p_sfc_fuzzy_eng_.get()));
		p_rules->addRule(::fl::Rule::parse("if " + e_fuzzy_var_name + " is NM and " + de_fuzzy_var_name + " is PM then " + alpha_fuzzy_var_name + " is SM", p_sfc_fuzzy_eng_.get()));
		p_rules->addRule(::fl::Rule::parse("if " + e_fuzzy_var_name + " is NM and " + de_fuzzy_var_name + " is PL then " + alpha_fuzzy_var_name + " is SM", p_sfc_fuzzy_eng_.get()));
		p_rules->addRule(::fl::Rule::parse("if " + e_fuzzy_var_name + " is NS and " + de_fuzzy_var_name + " is NL then " + alpha_fuzzy_var_name + " is VL", p_sfc_fuzzy_eng_.get()));
		p_rules->addRule(::fl::Rule::parse("if " + e_fuzzy_var_name + " is NS and " + de_fuzzy_var_name + " is NM then " + alpha_fuzzy_var_name + " is VL", p_sfc_fuzzy_eng_.get()));
		p_rules->addRule(::fl::Rule::parse("if " + e_fuzzy_var_name + " is NS and " + de_fuzzy_var_name + " is NS then " + alpha_fuzzy_var_name + " is LG", p_sfc_fuzzy_eng_.get()));
		p_rules->addRule(::fl::Rule::parse("if " + e_fuzzy_var_name + " is NS and " + de_fuzzy_var_name + " is ZE then " + alpha_fuzzy_var_name + " is ML", p_sfc_fuzzy_eng_.get()));
		p_rules->addRule(::fl::Rule::parse("if " + e_fuzzy_var_name + " is NS and " + de_fuzzy_var_name + " is PS then " + alpha_fuzzy_var_name + " is VS", p_sfc_fuzzy_eng_.get()));
		p_rules->addRule(::fl::Rule::parse("if " + e_fuzzy_var_name + " is NS and " + de_fuzzy_var_name + " is PM then " + alpha_fuzzy_var_name + " is SM", p_sfc_fuzzy_eng_.get()));
		p_rules->addRule(::fl::Rule::parse("if " + e_fuzzy_var_name + " is NS and " + de_fuzzy_var_name + " is PL then " + alpha_fuzzy_var_name + " is SL", p_sfc_fuzzy_eng_.get()));
		p_rules->addRule(::fl::Rule::parse("if " + e_fuzzy_var_name + " is ZE and " + de_fuzzy_var_name + " is NL then " + alpha_fuzzy_var_name + " is LG", p_sfc_fuzzy_eng_.get()));
		p_rules->addRule(::fl::Rule::parse("if " + e_fuzzy_var_name + " is ZE and " + de_fuzzy_var_name + " is NM then " + alpha_fuzzy_var_name + " is ML", p_sfc_fuzzy_eng_.get()));
		p_rules->addRule(::fl::Rule::parse("if " + e_fuzzy_var_name + " is ZE and " + de_fuzzy_var_name + " is NS then " + alpha_fuzzy_var_name + " is SL", p_sfc_fuzzy_eng_.get()));
		p_rules->addRule(::fl::Rule::parse("if " + e_fuzzy_var_name + " is ZE and " + de_fuzzy_var_name + " is ZE then " + alpha_fuzzy_var_name + " is ZE", p_sfc_fuzzy_eng_.get()));
		p_rules->addRule(::fl::Rule::parse("if " + e_fuzzy_var_name + " is ZE and " + de_fuzzy_var_name + " is PS then " + alpha_fuzzy_var_name + " is SL", p_sfc_fuzzy_eng_.get()));
		p_rules->addRule(::fl::Rule::parse("if " + e_fuzzy_var_name + " is ZE and " + de_fuzzy_var_name + " is PM then " + alpha_fuzzy_var_name + " is ML", p_sfc_fuzzy_eng_.get()));
		p_rules->addRule(::fl::Rule::parse("if " + e_fuzzy_var_name + " is ZE and " + de_fuzzy_var_name + " is PL then " + alpha_fuzzy_var_name + " is LG", p_sfc_fuzzy_eng_.get()));
		p_rules->addRule(::fl::Rule::parse("if " + e_fuzzy_var_name + " is PS and " + de_fuzzy_var_name + " is NL then " + alpha_fuzzy_var_name + " is SL", p_sfc_fuzzy_eng_.get()));
		p_rules->addRule(::fl::Rule::parse("if " + e_fuzzy_var_name + " is PS and " + de_fuzzy_var_name + " is NM then " + alpha_fuzzy_var_name + " is SM", p_sfc_fuzzy_eng_.get()));
		p_rules->addRule(::fl::Rule::parse("if " + e_fuzzy_var_name + " is PS and " + de_fuzzy_var_name + " is NS then " + alpha_fuzzy_var_name + " is VS", p_sfc_fuzzy_eng_.get()));
		p_rules->addRule(::fl::Rule::parse("if " + e_fuzzy_var_name + " is PS and " + de_fuzzy_var_name + " is ZE then " + alpha_fuzzy_var_name + " is ML", p_sfc_fuzzy_eng_.get()));
		p_rules->addRule(::fl::Rule::parse("if " + e_fuzzy_var_name + " is PS and " + de_fuzzy_var_name + " is PS then " + alpha_fuzzy_var_name + " is LG", p_sfc_fuzzy_eng_.get()));
		p_rules->addRule(::fl::Rule::parse("if " + e_fuzzy_var_name + " is PS and " + de_fuzzy_var_name + " is PM then " + alpha_fuzzy_var_name + " is LG", p_sfc_fuzzy_eng_.get()));
		p_rules->addRule(::fl::Rule::parse("if " + e_fuzzy_var_name + " is PS and " + de_fuzzy_var_name + " is PL then " + alpha_fuzzy_var_name + " is VL", p_sfc_fuzzy_eng_.get()));
		p_rules->addRule(::fl::Rule::parse("if " + e_fuzzy_var_name + " is PM and " + de_fuzzy_var_name + " is NL then " + alpha_fuzzy_var_name + " is SM", p_sfc_fuzzy_eng_.get()));
		p_rules->addRule(::fl::Rule::parse("if " + e_fuzzy_var_name + " is PM and " + de_fuzzy_var_name + " is NM then " + alpha_fuzzy_var_name + " is SM", p_sfc_fuzzy_eng_.get()));
		p_rules->addRule(::fl::Rule::parse("if " + e_fuzzy_var_name + " is PM and " + de_fuzzy_var_name + " is NS then " + alpha_fuzzy_var_name + " is SM", p_sfc_fuzzy_eng_.get()));
		p_rules->addRule(::fl::Rule::parse("if " + e_fuzzy_var_name + " is PM and " + de_fuzzy_var_name + " is ZE then " + alpha_fuzzy_var_name + " is SL", p_sfc_fuzzy_eng_.get()));
		p_rules->addRule(::fl::Rule::parse("if " + e_fuzzy_var_name + " is PM and " + de_fuzzy_var_name + " is PS then " + alpha_fuzzy_var_name + " is LG", p_sfc_fuzzy_eng_.get()));
		p_rules->addRule(::fl::Rule::parse("if " + e_fuzzy_var_name + " is PM and " + de_fuzzy_var_name + " is PM then " + alpha_fuzzy_var_name + " is VL", p_sfc_fuzzy_eng_.get()));
		p_rules->addRule(::fl::Rule::parse("if " + e_fuzzy_var_name + " is PM and " + de_fuzzy_var_name + " is PL then " + alpha_fuzzy_var_name + " is VL", p_sfc_fuzzy_eng_.get()));
		p_rules->addRule(::fl::Rule::parse("if " + e_fuzzy_var_name + " is PL and " + de_fuzzy_var_name + " is NL then " + alpha_fuzzy_var_name + " is ZE", p_sfc_fuzzy_eng_.get()));
		p_rules->addRule(::fl::Rule::parse("if " + e_fuzzy_var_name + " is PL and " + de_fuzzy_var_name + " is NM then " + alpha_fuzzy_var_name + " is VS", p_sfc_fuzzy_eng_.get()));
		p_rules->addRule(::fl::Rule::parse("if " + e_fuzzy_var_name + " is PL and " + de_fuzzy_var_name + " is NS then " + alpha_fuzzy_var_name + " is VS", p_sfc_fuzzy_eng_.get()));
		p_rules->addRule(::fl::Rule::parse("if " + e_fuzzy_var_name + " is PL and " + de_fuzzy_var_name + " is ZE then " + alpha_fuzzy_var_name + " is SM", p_sfc_fuzzy_eng_.get()));
		p_rules->addRule(::fl::Rule::parse("if " + e_fuzzy_var_name + " is PL and " + de_fuzzy_var_name + " is PS then " + alpha_fuzzy_var_name + " is VL", p_sfc_fuzzy_eng_.get()));
		p_rules->addRule(::fl::Rule::parse("if " + e_fuzzy_var_name + " is PL and " + de_fuzzy_var_name + " is PM then " + alpha_fuzzy_var_name + " is VL", p_sfc_fuzzy_eng_.get()));
		p_rules->addRule(::fl::Rule::parse("if " + e_fuzzy_var_name + " is PL and " + de_fuzzy_var_name + " is PL then " + alpha_fuzzy_var_name + " is VL", p_sfc_fuzzy_eng_.get()));
		p_sfc_fuzzy_eng_->addRuleBlock(p_rules);
	}

	private: void do_reset()
	{
		typedef typename base_type::target_value_map::const_iterator target_iterator;
		typedef typename app_type::vm_pointer vm_pointer;

		const ::std::vector<vm_pointer> vms = this->app().vms();
		const ::std::size_t nvms = this->app().num_vms();

		// Reset output sensors
		app_sensors_.clear();
		const target_iterator tgt_end_it = this->target_values().end();
		for (target_iterator tgt_it = this->target_values().begin();
			 tgt_it != tgt_end_it;
			 ++tgt_it)
		{
			const application_performance_category cat(tgt_it->first);

			app_sensors_[cat] = this->app().sensor(cat);
			es_[cat] = 0;
		}

		// Reset input sensors
		vm_sensors_.clear();
		for (::std::size_t i = 0; i < nvms; ++i)
		{
			const virtual_machine_performance_category cat = cpu_util_virtual_machine_performance;
			vm_pointer p_vm = vms[i];

			vm_sensors_[cat][p_vm->id()] = p_vm->sensor(cat);
		}

		// Reset counters
		ctl_count_ = ctl_skip_count_
				   = ctl_fail_count_
				   = 0;

		// Reset fuzzy controllers
		p_rc_fuzzy_eng_->restart();
		p_sfc_fuzzy_eng_->restart();

		// Reset input scaling factors
		Ke_ = Kde_ = 0;

		// Reset VCPU util estimator and smoother
		for (::std::size_t i = 0; i < nvms; ++i)
		{
			//this->data_estimator(cpu_util_virtual_machine_performance, vms[i]->id(), ::boost::make_shared< testbed::mean_estimator<real_type> >());
			this->data_smoother(cpu_util_virtual_machine_performance, vms[i]->id(), ::boost::make_shared< testbed::brown_single_exponential_smoother<real_type> >(0.9));
			//this->data_smoother(cpu_util_virtual_machine_performance, vms[i]->id(), ::boost::make_shared< testbed::holt_winters_double_exponential_smoother<real_type> >(beta_));
		}

		// Reset output data file and write header
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
				*p_dat_ofs_ << ",\"Cap_{" << vms[i]->id() << "}\",\"Share_{" << vms[i]->id() << "}\",\"Util_{" << vms[i]->id() << "}\"";
			}
			for (target_iterator tgt_it = this->target_values().begin();
				 tgt_it != tgt_end_it;
				 ++tgt_it)
			{
				const application_performance_category cat = tgt_it->first;

				*p_dat_ofs_ << ",\"y_{" << cat << "}\",\"r_{" << cat << "}\"";
			}
			*p_dat_ofs_ << ",\"alpha\",\"Delta u\",\"K_e\",\"K_{Delta e}\",\"# Controls\",\"# Skip Controls\",\"# Fail Controls\"";
			*p_dat_ofs_ << ::std::endl;
		}
	}

	private: void do_sample()
	{
		typedef typename vm_sensor_map::const_iterator vm_sensor_iterator;
		typedef typename app_sensor_map::const_iterator app_sensor_iterator;
		typedef ::std::vector<typename sensor_type::observation_type> obs_container;
		typedef typename obs_container::const_iterator obs_iterator;

		DCS_DEBUG_TRACE("(" << this << ") BEGIN Do SAMPLE - Count: " << ctl_count_ << "/" << ctl_skip_count_ << "/" << ctl_fail_count_);

		// Collect VM values
		const vm_sensor_iterator vm_sens_end_it = vm_sensors_.end();
		for (vm_sensor_iterator vm_sens_it = vm_sensors_.begin();
			 vm_sens_it != vm_sens_end_it;
			 ++vm_sens_it)
		{
			const virtual_machine_performance_category cat = vm_sens_it->first;

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
						//this->data_estimator(cat, vm_id).collect(it->value());
						this->data_smoother(cat, vm_id).smooth(it->value());
					}
				}
			}
		}

		// Collect App values
		const app_sensor_iterator app_sens_end_it = app_sensors_.end();
		for (app_sensor_iterator app_sens_it = app_sensors_.begin();
			 app_sens_it != app_sens_end_it;
			 ++app_sens_it)
		{
			const application_performance_category cat(app_sens_it->first);

			sensor_pointer p_sens(app_sens_it->second);

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

		::std::vector<vm_pointer> vms = this->app().vms();
		const ::std::size_t nvms = vms.size();

		//std::map<application_performance_category,real_type> es_;
		std::map<application_performance_category,real_type> des;

		const target_iterator tgt_end_it = this->target_values().end();
		for (target_iterator tgt_it = this->target_values().begin();
			 tgt_it != tgt_end_it;
			 ++tgt_it)
		{
			const application_performance_category cat(tgt_it->first);

			// Compute a summary statistics of collected observation
			if (this->data_estimator(cat).count() > 0)
			{
				const real_type y = this->data_estimator(cat).estimate();
				const real_type r = this->target_value(cat);

				real_type e = 0;
				switch (cat)
				{
					case response_time_application_performance:
						if (dcs::math::float_traits<real_type>::approximately_less_equal(y, 2.0*r))
						{
							e = (r-y)/r;
						}
						else
						{
							e = -1;
						}
						break;
					case throughput_application_performance:
						if (dcs::math::float_traits<real_type>::approximately_greater_equal(y, 0.5*r))
						{
							e = (y-r)/r;
						}
						else
						{
							e = -1;
						}
						break;
				}
				if (es_.count(cat) > 0)
				{
					des[cat] = e-es_.at(cat);
				}
				else
				{
					des[cat] = e;
				}
				es_[cat] = e;
DCS_DEBUG_TRACE("APP Performance Category: " << cat << " - Y(k): " << y << " - R: " << r << " -> E(k+1): " << es_.at(cat) << " - DeltaE(k+1): " << des.at(cat));//XXX
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

        if (!skip_ctl)
        {
			//FIXME: actually we only handle SISO systems
			DCS_ASSERT(es_.size() == 1,
					   DCS_EXCEPTION_THROW(::std::runtime_error,
					   "Only SISO system are currently managed"));

			// Compute the input to the RC fuzzy controller
			const real_type Ke = (ctl_count_ > 1) ? ::std::abs(Ke_) : 1.0;
			const real_type Kde = (ctl_count_ > 1) ? ::std::abs(Kde_) : 1.0;
			const real_type e = Ke*es_.begin()->second;
			const real_type de = Kde*des.begin()->second;

			// Update input scaling factors
			Ke_ = (1-gamma_)*Ke_ + gamma_*es_.begin()->second;
			Kde_ = (1-gamma_)*Kde_ - gamma_*des.begin()->second;

			// Perform fuzzy control
			bool ok = false;
			real_type du = 0;
			real_type alpha = 0;

			try
			{
				// Compute \Delta u(k)
				p_rc_fuzzy_eng_->setInputValue(e_fuzzy_var_name, e);
				p_rc_fuzzy_eng_->setInputValue(de_fuzzy_var_name, de);
				p_rc_fuzzy_eng_->process();
				du = p_rc_fuzzy_eng_->getOutputValue(du_fuzzy_var_name);

				// Compute the \alpha(k) scaling factor
				p_sfc_fuzzy_eng_->setInputValue(e_fuzzy_var_name, e);
				p_sfc_fuzzy_eng_->setInputValue(de_fuzzy_var_name, de);
				p_sfc_fuzzy_eng_->process();
				alpha = p_sfc_fuzzy_eng_->getOutputValue(alpha_fuzzy_var_name);

				ok = true;
			}
			catch (::fl::Exception const& fe)
			{
				DCS_DEBUG_TRACE( "Caught exception: " << fe.what() );

				::std::ostringstream oss;
				oss << "Unable to compute optimal control: " << fe.what();
				::dcs::log_warn(DCS_LOGGING_AT, oss.str());
			}
			catch (::std::exception const& se)
			{
				DCS_DEBUG_TRACE( "Caught exception: " << se.what() );

				::std::ostringstream oss;
				oss << "Unable to compute optimal control: " << se.what();
				::dcs::log_warn(DCS_LOGGING_AT, oss.str());
			}

			// Apply fuzzy control results
			if (ok)
			{
				for (::std::size_t i = 0; i < nvms; ++i)
				{
					vm_pointer p_vm = vms[i];

					const real_type c = p_vm->cpu_share();

					// Compute the output amplifier
					const real_type Kdu = c*0.5*::std::abs(e);

					const real_type u = ::std::max(::std::min(c+alpha*Kdu*du, 1.0), 0.0);

					DCS_DEBUG_TRACE("VM '" << p_vm->id() << "' - old-share: " << c << " - new-share: " << u);

					if (::std::isfinite(u) && !::dcs::math::float_traits<real_type>::essentially_equal(c, u))
					{
						p_vm->cpu_share(u);
DCS_DEBUG_TRACE("VM " << vms[i]->id() << ", Alpha: " << alpha << ", DeltaU: " << du << ", K_{DeltaU}: " << Kdu << " -> U(k+1): " << u);//XXX
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
				*p_dat_ofs_ << p_vm->cpu_cap() << "," << p_vm->cpu_share() << "," << this->data_smoother(cpu_util_virtual_machine_performance, p_vm->id()).forecast(0);
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
				const real_type y = this->data_estimator(cat).estimate();
				const real_type r = tgt_it->second;
				*p_dat_ofs_ << y << "," << r;
			}
			if (skip_ctl)
			{
				const real_type nan = ::std::numeric_limits<real_type>::quiet_NaN();

				*p_dat_ofs_ << "," << nan << "," << nan;
			}
			else
			{
				const real_type alpha = p_sfc_fuzzy_eng_->getOutputValue(alpha_fuzzy_var_name);
				const real_type du = p_rc_fuzzy_eng_->getOutputValue(du_fuzzy_var_name);

				*p_dat_ofs_ << "," << alpha << "," << du;
			}
			*p_dat_ofs_ << "," << Ke_ << "," << Kde_;
			*p_dat_ofs_ << "," << ctl_count_ << "," << ctl_skip_count_ << "," << ctl_fail_count_;
			*p_dat_ofs_ << ::std::endl;
		}

		DCS_DEBUG_TRACE("(" << this << ") END Do CONTROL - Count: " << ctl_count_ << "/" << ctl_skip_count_ << "/" << ctl_fail_count_);
	}


	private: real_type gamma_; ///< The EWMA smoothing factor for Cres
	private: real_type Ke_;
	private: real_type Kde_;
	private: ::boost::shared_ptr< ::fl::Engine > p_rc_fuzzy_eng_; ///< The fuzzy resource control engine
	private: ::boost::shared_ptr< ::fl::Engine > p_sfc_fuzzy_eng_; ///< The fuzzy scaling factor control engine
	private: ::std::map<application_performance_category,real_type> es_; ///< The e(k) variable, grouped by application performance category
	private: ::std::size_t ctl_count_; ///< Number of times control function has been invoked
	private: ::std::size_t ctl_skip_count_; ///< Number of times control has been skipped
	private: ::std::size_t ctl_fail_count_; ///< Number of times control has failed
	private: vm_sensor_map vm_sensors_;
	private: app_sensor_map app_sensors_;
	private: ::std::string dat_fname_;
	private: ::boost::shared_ptr< ::std::ofstream > p_dat_ofs_;
}; // rao2013_dynaqos_application_manager

template <typename T>
const ::std::string rao2013_dynaqos_application_manager<T>::alpha_fuzzy_var_name = "alpha";

template <typename T>
const ::std::string rao2013_dynaqos_application_manager<T>::de_fuzzy_var_name = "DeltaE";

template <typename T>
const ::std::string rao2013_dynaqos_application_manager<T>::du_fuzzy_var_name = "DeltaU";

template <typename T>
const ::std::string rao2013_dynaqos_application_manager<T>::e_fuzzy_var_name = "E";

}} // Namespace dcs::testbed

#endif // DCS_TESTBED_RAO2013_DYNAQOS_APPLICATION_MANAGER_HPP
