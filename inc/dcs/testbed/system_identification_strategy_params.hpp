/**
 * \file dcs/testbed/system_identification_strategy_params.hpp
 *
 * \brief Class for parameterizing system identification strategies.
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

#ifndef DCS_TESTBED_SYSTEM_IDENTIFICATION_STRATEGY_PARAMS_HPP
#define DCS_TESTBED_SYSTEM_IDENTIFICATION_STRATEGY_PARAMS_HPP


#include <cstddef>


namespace dcs { namespace testbed {

enum system_identification_strategy_category
{
	rls_bittanti1990_system_identification_strategy,
	rls_ff_system_identification_strategy,
	rls_kulhavy1984_system_identification_strategy,
	rls_park1991_system_identification_strategy
};


template <typename TraitsT>
class base_system_identification_strategy_params
{
	public: typedef TraitsT traits_type;
	public: typedef ::std::size_t size_type;


	public: base_system_identification_strategy_params()
	: n_a_(0),
	  n_b_(0),
	  n_c_(0),
	  d_(0),
	  n_y_(0),
	  n_u_(0)
	{
	}

	public: virtual ~base_system_identification_strategy_params()
	{
	}

	public: system_identification_strategy_category category() const
	{
		return do_category();
	}

	public: void output_order(size_type x)
	{
		n_a_ = x;
	}

	public: size_type output_order() const
	{
		return n_a_;
	}

	public: void input_order(size_type x)
	{
		n_b_ = x;
	}

	public: size_type input_order() const
	{
		return n_b_;
	}

	public: void noise_order(size_type x)
	{
		n_c_ = x;
	}

	public: size_type noise_order() const
	{
		return n_c_;
	}

	public: void input_delay(size_type x)
	{
		d_ = x;
	}

	public: size_type input_delay() const
	{
		return d_;
	}

	public: void num_outputs(size_type x)
	{
		n_y_ = x;
	}

	public: size_type num_outputs() const
	{
		return n_y_;
	}

	public: void num_inputs(size_type x)
	{
		n_u_ = x;
	}

	public: size_type num_inputs() const
	{
		return n_u_;
	}

	private: virtual system_identification_strategy_category do_category() const = 0;


	/// Output order.
	private: size_type n_a_;
	/// Input order.
	private: size_type n_b_;
	/// Noise order.
	private: size_type n_c_;
	/// Input delay.
	private: size_type d_;
	/// Number of outputs.
	private: size_type n_y_;
	/// Number of inputs.
	private: size_type n_u_;
}; // base_system_identification_strategy_params


template <typename TraitsT>
class rls_system_identification_strategy_params: public base_system_identification_strategy_params<TraitsT>
{
	private: typedef base_system_identification_strategy_params<TraitsT> base_type;
	public: typedef TraitsT traits_type;
	public: typedef ::std::size_t size_type;
	public: typedef typename traits_type::real_type real_type;
	public: typedef typename traits_type::uint_type uint_type;


	public: rls_system_identification_strategy_params()
	: base_type(),
	  miso_(false),
	  max_cov_heuristic_(false),
	  max_cov_heuristic_val_(0),
	  cond_cov_heuristic_(false),
	  cond_cov_heuristic_trust_digits_(0)
	{
	}


	public: void mimo_as_miso(bool value)
	{
		miso_ = value;
	}


	public: bool mimo_as_miso() const
	{
		return miso_;
	}


	public: void max_covariance_heuristic(bool value)
	{
		max_cov_heuristic_ = value;
	}


	public: bool max_covariance_heuristic() const
	{
		return max_cov_heuristic_;
	}


	public: void max_covariance_heuristic_max_value(real_type value)
	{
		max_cov_heuristic_val_ = value;
	}


	public: real_type max_covariance_heuristic_max_value() const
	{
		return max_cov_heuristic_val_;
	}


	public: void condition_number_covariance_heuristic(bool value)
	{
		cond_cov_heuristic_ = value;
	}


	public: bool condition_number_covariance_heuristic() const
	{
		return cond_cov_heuristic_;
	}


	public: void condition_number_covariance_heuristic_trusted_digits(uint_type value)
	{
		cond_cov_heuristic_trust_digits_ = value;
	}


	public: uint_type condition_number_covariance_heuristic_trusted_digits() const
	{
		return cond_cov_heuristic_trust_digits_;
	}


	private: bool miso_;
	private: bool max_cov_heuristic_;
	private: real_type max_cov_heuristic_val_;
	private: bool cond_cov_heuristic_;
	private: uint_type cond_cov_heuristic_trust_digits_;
}; // rls_system_identification_strategy_params


template <typename TraitsT>
class rls_bittanti1990_system_identification_strategy_params: public rls_system_identification_strategy_params<TraitsT>
{
	private: typedef rls_system_identification_strategy_params<TraitsT> base_type;
	public: typedef TraitsT traits_type;
	public: typedef typename traits_type::real_type real_type;


	public: rls_bittanti1990_system_identification_strategy_params(real_type ff, real_type delta)
	: base_type(),
	  ff_(ff),
	  delta_(delta)
	{
	}


	private: system_identification_strategy_category do_category() const
	{
		return rls_bittanti1990_system_identification_strategy;
	}


	public: void forgetting_factor(real_type value)
	{
		ff_ = value;
	}


	public: real_type forgetting_factor() const
	{
		return ff_;
	}


	public: void correction_factor(real_type value)
	{
		delta_ = value;
	}


	public: real_type correction_factor() const
	{
		return delta_;
	}


	private: real_type ff_;
	private: real_type delta_;
}; // rls_bittanti1990_system_identification_strategy_params


template <typename TraitsT>
class rls_ff_system_identification_strategy_params: public rls_system_identification_strategy_params<TraitsT>
{
	private: typedef rls_system_identification_strategy_params<TraitsT> base_type;
	public: typedef TraitsT traits_type;
	public: typedef typename traits_type::real_type real_type;


	public: rls_ff_system_identification_strategy_params(real_type ff)
	: base_type(),
	  ff_(ff)
	{
	}


	private: system_identification_strategy_category do_category() const
	{
		return rls_ff_system_identification_strategy;
	}


	public: void forgetting_factor(real_type value)
	{
		ff_ = value;
	}


	public: real_type forgetting_factor() const
	{
		return ff_;
	}


	private: real_type ff_;
}; // rls_ff_system_identification_strategy_params


template <typename TraitsT>
class rls_kulhavy1984_system_identification_strategy_params: public rls_system_identification_strategy_params<TraitsT>
{
	private: typedef rls_system_identification_strategy_params<TraitsT> base_type;
	public: typedef TraitsT traits_type;
	public: typedef typename traits_type::real_type real_type;


	public: rls_kulhavy1984_system_identification_strategy_params(real_type ff)
	: base_type(),
	  ff_(ff)
	{
	}


	private: system_identification_strategy_category do_category() const
	{
		return rls_kulhavy1984_system_identification_strategy;
	}


	public: void forgetting_factor(real_type value)
	{
		ff_ = value;
	}


	public: real_type forgetting_factor() const
	{
		return ff_;
	}


	private: real_type ff_;
}; // rls_kulhavy1984_system_identification_strategy_params


template <typename TraitsT>
class rls_park1991_system_identification_strategy_params: public rls_system_identification_strategy_params<TraitsT>
{
	private: typedef rls_system_identification_strategy_params<TraitsT> base_type;
	public: typedef TraitsT traits_type;
	public: typedef typename traits_type::real_type real_type;


	public: rls_park1991_system_identification_strategy_params(real_type ff, real_type rho)
	: base_type(),
	  ff_(ff),
	  rho_(rho)
	{
	}


	private: system_identification_strategy_category do_category() const
	{
		return rls_park1991_system_identification_strategy;
	}


	public: void forgetting_factor(real_type value)
	{
		ff_ = value;
	}


	public: real_type forgetting_factor() const
	{
		return ff_;
	}


	public: void sensitivity_gain(real_type value)
	{
		rho_ = value;
	}


	public: real_type sensitivity_gain() const
	{
		return rho_;
	}


	private: real_type ff_;
	private: real_type rho_;
}; // rls_park1991_system_identification_strategy_params


}} // Namespace dcs::testbed


#endif // DCS_TESTBED_SYSTEM_IDENTIFICATION_STRATEGY_PARAMS_HPP
