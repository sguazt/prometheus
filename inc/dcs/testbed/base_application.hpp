/**
 * \file dcs/testbed/base_application.hpp
 *
 * \brief Base class for applications.
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

#ifndef DCS_TESTBED_BASE_APPLICATION_HPP
#define DCS_TESTBED_BASE_APPLICATION_HPP


#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>
#include <cstddef>
#include <dcs/testbed/application_performance_category.hpp>
#include <dcs/testbed/base_sensor.hpp>
#include <dcs/testbed/base_virtual_machine.hpp>
#include <string>


namespace dcs { namespace testbed {

/**
 * \brief Base class for applications.
 *
 * \tparam TraitsT Traits type.
 *
 * \author Marco Guazzone (marco.guazzone@gmail.com)
 */
template <typename TraitsT>
class base_application
{
	public: typedef TraitsT traits_type;
	public: typedef base_virtual_machine<traits_type> vm_type;
	public: typedef ::boost::shared_ptr<vm_type> vm_pointer;
//	public: typedef typename vm_type::identifier_type vm_identifier_type;
	public: typedef base_sensor<traits_type> sensor_type;
	public: typedef ::boost::shared_ptr<sensor_type> sensor_pointer;
	public: typedef typename traits_type::real_type real_type;
	public: typedef ::boost::function<bool (real_type x)> slo_checker_type;
	public: typedef unsigned long identifier_type;


	private: static identifier_type next_id_;


	/// Generate a unique identifier for this application
	public: static identifier_type make_id()
	{
		return next_id_++;
	}


	protected: base_application()
	: id_(make_id())
	{
		// Empty
	}

	public: virtual ~base_application()
	{
		// Empty
	}

	/// Returns the unique identifier of this application
	public: identifier_type id() const
	{
		return id_;
	}

	/// Sets the name of this apoplication
	public: void name(::std::string const& val)
	{
		name_ = val;
	}

	/// Gets the name of this apoplication
	public: ::std::string name() const
	{
		return name_;
	}

	/// Gets the number of VMs associated with this application
	public: ::std::size_t num_vms() const
	{
		return do_num_vms();
	}

	/// Gets the VMs associated with this application
	public: ::std::vector<vm_pointer> vms() const
	{
		return do_vms();
	}

//	public: vm_pointer vm(vm_identifier_type id) const
//	{
//		return do_vm(id);
//	}

	/// Registers a sensor for collecting measure related to the given application performance category
	public: void register_sensor(application_performance_category cat, sensor_pointer const& p_sens)
	{
		do_register_sensor(cat, p_sens);
	}

	/// Deregisters a sensor for collecting measure related to the given application performance category
	public: void deregister_sensor(application_performance_category cat)
	{
		do_deregister_sensor(cat);
	}

	/// Returns the sensor for collecting measure related to the given application performance category
	public: sensor_pointer sensor(application_performance_category cat)
	{
		return do_sensor(cat);
	}

	/// Returns the sensor for collecting measure related to the given application performance category
	public: sensor_pointer sensor(application_performance_category cat) const
	{
		return do_sensor(cat);
	}

	/// Sets the given function as a checker for SLOs of the given category
	public: template <typename FuncT>
			void slo(application_performance_category cat, FuncT checker)
	{
		do_slo(cat, slo_checker_type(checker));
	}

	/// Checks if the SLOs of the given category is satisfied against the given value
	public: bool slo(application_performance_category cat, real_type val)
	{
		return do_slo(cat, val);
	}

	private: virtual ::std::size_t do_num_vms() const = 0;

	private: virtual ::std::vector<vm_pointer> do_vms() const = 0;

	private: virtual void do_register_sensor(application_performance_category cat, sensor_pointer const& p_sens) = 0;

	private: virtual void do_deregister_sensor(application_performance_category cat) = 0;

	private: virtual sensor_pointer do_sensor(application_performance_category cat) = 0;

	private: virtual sensor_pointer do_sensor(application_performance_category cat) const = 0;

	private: virtual void do_slo(application_performance_category cat, slo_checker_type const& checker) = 0;

	private: virtual bool do_slo(application_performance_category cat, real_type val) const = 0;


	private: identifier_type id_; ///< The unique identifier
	private: ::std::string name_; ///< The mnemonic name of this application
}; // base_application

template <typename T>
typename base_application<T>::identifier_type base_application<T>::next_id_ = 0;

}} // Namespace dcs::testbed

#endif // DCS_TESTBED_BASE_APPLICATION_HPP
