/**
 * \file dcs/testbed/application.hpp
 *
 * \brief Generic application.
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

#ifndef DCS_TESTBED_APPLICATION_HPP
#define DCS_TESTBED_APPLICATION_HPP


#include <cstddef>
#include <dcs/assert.hpp>
#include <dcs/exception.hpp>
#include <dcs/testbed/application_performance_category.hpp>
#include <dcs/testbed/base_application.hpp>
#include <dcs/testbed/base_virtual_machine.hpp>
#include <map>
#include <stdexcept>
#include <vector>


namespace dcs { namespace testbed {

/**
 * \brief Generic application.
 *
 * \tparam TraitsT Traits type.
 *
 * \author Marco Guazzone (marco.guazzone@gmail.com)
 */
template <typename TraitsT>
class application: public base_application<TraitsT>
{
	private: typedef base_application<TraitsT> base_type;
	public: typedef typename base_type::traits_type traits_type;
	public: typedef typename base_type::vm_pointer vm_pointer;
	public: typedef typename base_type::sensor_pointer sensor_pointer;
	public: typedef typename base_type::slo_checker_type slo_checker_type;
	public: typedef typename base_type::real_type real_type;
//	private: typedef typename base_type::vm_type::identifier_type vm _identifier_type;
//	private: typedef ::std::map<vm_identifier_type,vm_pointer> vm_container;
	private: typedef ::std::vector<vm_pointer> vm_container;
	private: typedef ::std::map<application_performance_category,sensor_pointer> sensor_map;
	private: typedef ::std::map<application_performance_category,slo_checker_type> slo_checker_map;


	public: application()
	{
		// Empty
	}

	public: template <typename IterT>
			application(IterT vm_first, IterT vm_last)
	: vms_(vm_first,vm_last)
	{
//		while (vm_first != vm_last)
//		{
//			vms_[*(vm_first)->id()] = *vm_first;
//			++vm_first;
//		}
	}

	private: ::std::size_t do_num_vms() const
	{
		return vms_.size();
	}

	private: void do_vms(const std::vector<vm_pointer>& vms)
	{
		vms_.assign(vms.begin(), vms.end());
	}

	private: ::std::vector<vm_pointer> do_vms() const
	{
//		::std::vector<vm_pointer> vms(vms_.size());
//
//		::std::size_t i(0);
//		typename vm_container::const_iterator end_it(vms_.end());
//		for (typename vm_container::const_iterator it = vms_.begin();
//			 it != end_it;
//			 ++it)
//		{
//			vms[i] = *it;
//			++i;
//		}
//
//		return vms;
		return vms_;
	}

//	private: vm_pointer do_vm(vm_identifier_type id) const
//	{
//		DCS_ASSERT(vms_.count(id) > 0,
//				   DCS_EXCEPTION_THROW(::std::invalid_argument,
//									   "Invalid VM identifier"));
//
//		return vms_.at(id);
//	}

	private: void do_register_sensor(application_performance_category cat, sensor_pointer const& p_sens)
	{
		DCS_ASSERT(p_sens,
				   DCS_EXCEPTION_THROW(::std::invalid_argument,
									   "Invalid sensor"));

		sensors_[cat] = p_sens;
	}

	private: void do_deregister_sensor(application_performance_category cat)
	{
		DCS_ASSERT(sensors_.count(cat) > 0,
				   DCS_EXCEPTION_THROW(::std::invalid_argument,
									   "Invalid category: sensor not found"));

		sensors_.erase(cat);
	}

	private: sensor_pointer do_sensor(application_performance_category cat)
	{
		DCS_ASSERT(sensors_.count(cat) > 0,
				   DCS_EXCEPTION_THROW(::std::invalid_argument,
									   "Invalid category: sensor not found"));

		return sensors_.at(cat);
	}

	private: sensor_pointer do_sensor(application_performance_category cat) const
	{
		DCS_ASSERT(sensors_.count(cat) > 0,
				   DCS_EXCEPTION_THROW(::std::invalid_argument,
									   "Invalid category: sensor not found"));

		return sensors_.at(cat);
	}

	private: void do_slo(application_performance_category cat, slo_checker_type const& checker)
	{
		slo_map_[cat] = checker;
	}

	private: bool do_slo(application_performance_category cat, real_type val) const
	{
		//DCS_ASSERT(slo_map_.count(cat) > 0,
		//		   DCS_EXCEPTION_THROW(::std::invalid_argument,
		//							   "Invalid category: SLO checker not found"));

		return slo_map_.count(cat) > 0 ? slo_map_.at(cat)(val) : true;
	}


	private: vm_container vms_;
	private: sensor_map sensors_;
	private: slo_checker_map slo_map_;
}; // application

}} // Namespace dcs::testbed

#endif // DCS_TESTBED_APPLICATION_HPP
