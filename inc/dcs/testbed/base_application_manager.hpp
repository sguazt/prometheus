/**
 * \file dcs/testbed/base_application_manager.hpp
 *
 * \brief Base class for application managers.
 *
 * \author Marco Guazzone (marco.guazzone@gmail.com)
 *
 * <hr/>
 *
 * Copyright (C) 2012-2013  Marco Guazzone (marco.guazzone@gmail.com)
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

#ifndef DCS_TESTBED_BASE_APPLICATION_MANAGER_HPP
#define DCS_TESTBED_BASE_APPLICATION_MANAGER_HPP


#include <boost/shared_ptr.hpp>
#include <dcs/assert.hpp>
#include <dcs/exception.hpp>
#include <dcs/testbed/application_performance_category.hpp>
#include <dcs/testbed/base_application.hpp>
#include <dcs/testbed/data_estimators.hpp>
//#include <dcs/testbed/data_smoothers.hpp>
#include <stdexcept>
#include <map>
#include <vector>


namespace dcs { namespace testbed {

template <typename TraitsT>
class base_application_manager
{
	public: typedef TraitsT traits_type;
	public: typedef typename traits_type::real_type real_type;
	protected: typedef base_application<traits_type> app_type;
	protected: typedef base_estimator<real_type> data_estimator_type;
//	protected: typedef base_smoother<real_type> data_smoother_type;
	protected: typedef ::std::map<application_performance_category,real_type> target_value_map;
	public: typedef ::boost::shared_ptr<app_type> app_pointer;
	public: typedef ::boost::shared_ptr<data_estimator_type> data_estimator_pointer;
//	public: typedef ::boost::shared_ptr<data_smoother_type> data_smoother_pointer;
	protected: typedef ::std::map<application_performance_category,data_estimator_pointer> data_estimator_map;


	public: base_application_manager()
	: ts_(1),
	  tc_(1)
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

        estimators_[cat] = p_estimator;
	}

	public: data_estimator_type& data_estimator(application_performance_category cat)
	{
		return *(estimators_[cat]);
	}

	public: data_estimator_type const& data_estimator(application_performance_category cat) const
	{
		// pre: exists(estimators_[cat])
		DCS_ASSERT(estimators_.count(cat) > 0,
				   DCS_EXCEPTION_THROW(::std::invalid_argument,
									   "Invalid category for data estimator"));

		return *(estimators_.at(cat));
	}

//	public: void data_smoother(data_smoother_pointer const& p_smoother)
//	{
//		// pre: p_estimator != null
//		DCS_ASSERT(p_smoother,
//				   DCS_EXCEPTION_THROW(::std::invalid_argument, "Invalid pointer to data smoother"));
//
//		p_smoother_ = p_smoother;
//	}

//	public: data_smoother_pointer data_smoother()
//	{
//		return p_smoother_;
//	}

//	public: data_smoother_pointer data_smoother() const
//	{
//		return p_smoother_;
//	}

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

	public: void reset()
	{
		DCS_ASSERT(p_app_,
				   DCS_EXCEPTION_THROW(::std::runtime_error,
									   "Application is not set"));

		this->do_reset();
	}

	public: void sample()
	{
		this->do_sample();
	}

	public: void control()
	{
		this->do_control();
	}

	protected: app_type& app_ptr()
	{
		return p_app_;
	}

	protected: app_type& app_ptr() const
	{
		return p_app_;
	}

	protected: data_estimator_map& data_estimators()
	{
		return estimators_;
	}

	protected: data_estimator_map const& data_estimators() const
	{
		return estimators_;
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
	private: data_estimator_map estimators_; ///< Mapping between application performance categories and data estimator pointers
//	private: data_smoother_pointer p_smoother_; ///< Pointer to the data smoother
};

}} // Namespace dcs::testbed

#endif // DCS_TESTBED_BASE_APPLICATION_MANAGER_HPP
