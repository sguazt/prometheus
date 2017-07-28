/**
 * \file dcs/testbed/uniform_signal_generator.hpp
 *
 * \brief Generates signals according to a uniform probability distribution.
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

#ifndef DCS_TESTBED_UNIFORM_SIGNAL_GENERATOR_HPP
#define DCS_TESTBED_UNIFORM_SIGNAL_GENERATOR_HPP


#include <boost/random/uniform_real_distribution.hpp>
#include <cmath>
#include <cstddef>
#include <dcs/assert.hpp>
#include <dcs/testbed/base_signal_generator.hpp>
#include <limits>
#include <stdexcept>


namespace dcs { namespace testbed {

template <typename ValueT, typename RandomGeneratorT>
class uniform_signal_generator: public base_signal_generator<ValueT>
{
	private: typedef base_signal_generator<ValueT> base_type;
	public: typedef ValueT value_type;
	public: typedef typename base_type::vector_type vector_type;
	private: typedef ::boost::random::uniform_real_distribution<value_type> uniform_distribution_type;
	private: typedef ::std::vector<uniform_distribution_type> uniform_distribution_container;
	private: typedef RandomGeneratorT random_generator_type;


	public: uniform_signal_generator(vector_type const& u_min, vector_type const& u_max, random_generator_type& rng)
	: rng_(rng),
	  ub_( ::std::numeric_limits<value_type>::infinity()),
	  lb_(-::std::numeric_limits<value_type>::infinity())
	{
		// pre: size(u_min) == size(u_max)
		DCS_ASSERT(u_min.size() == u_max.size(),
				   DCS_EXCEPTION_THROW(::std::invalid_argument,
									   "Size of min and max vectors does not match"));

		::std::size_t n(u_min.size());
		for (::std::size_t i = 0; i < n; ++i)
		{
			distrs_.push_back(uniform_distribution_type(u_min[i], u_max[i]));
		}
	}

 
	private: vector_type do_generate()
	{
		::std::size_t n(distrs_.size());
		vector_type u(n);
		for (::std::size_t i = 0; i < n; ++i)
		{
			u[i] = ::std::min(::std::max(distrs_[i](rng_), lb_[i]), ub_[i]);
		}

		return u;
	}


	private: void do_reset()
	{
		// do nothing: we should reset the random number generator, but this should be done elsewhere.
	}

	private: void do_upper_bound(value_type val)
	{
		ub_ = vector_type(distrs_.size(), val);
	}

	private: void do_lower_bound(value_type val)
	{
		lb_ = vector_type(distrs_.size(), val);
	}


	private: random_generator_type& rng_;
	private: uniform_distribution_container distrs_;
	private: vector_type ub_;
	private: vector_type lb_;
}; // uniform_signal_generator

}} // Namespace dcs::testbed


#endif // DCS_TESTBED_UNIFORM_SIGNAL_GENERATOR_HPP
