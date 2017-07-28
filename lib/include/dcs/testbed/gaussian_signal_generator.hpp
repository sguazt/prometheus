/**
 * \file dcs/testbed/guassian_signal_generator.hpp
 *
 * \brief Generates signals according to a Normal distribution.
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

#ifndef DCS_TESTBED_GAUSSIAN_SIGNAL_GENERATOR_HPP
#define DCS_TESTBED_GAUSSIAN_SIGNAL_GENERATOR_HPP


#include <boost/random/normal_distribution.hpp>
#include <cmath>
#include <cstddef>
#include <dcs/assert.hpp>
#include <dcs/exception.hpp>
#include <dcs/testbed/base_signal_generator.hpp>
#include <limits>
#include <stdexcept>


namespace dcs { namespace testbed {

template <typename ValueT, typename RandomGeneratorT>
class gaussian_signal_generator: public base_signal_generator<ValueT>
{
	private: typedef base_signal_generator<ValueT> base_type;
	public: typedef ValueT value_type;
	public: typedef typename base_type::vector_type vector_type;
	private: typedef ::boost::random::normal_distribution<value_type> normal_distribution_type;
	private: typedef ::std::vector<normal_distribution_type> normal_distribution_container;
	private: typedef RandomGeneratorT random_generator_type;


	public: gaussian_signal_generator(vector_type const& mu0, vector_type const& sigma0, random_generator_type& rng)
	: rng_(rng),
	  ub_(mu0.size(), ::std::numeric_limits<value_type>::infinity()),
	  lb_(mu0.size(),-::std::numeric_limits<value_type>::infinity())
	{
		// pre: size(mu0) == size(sigma0)
		DCS_ASSERT(mu0.size() == sigma0.size(),
				   DCS_EXCEPTION_THROW(::std::invalid_argument, "Size of input vectors does not match"));

		::std::size_t n(mu0.size());
		for (::std::size_t i = 0; i < n; ++i)
		{
			distrs_.push_back(normal_distribution_type(mu0[i], sigma0[i]));
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
		// do nothing: the generator is reset by resetting the random number generator, which should be made elsewhere.
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
	private: normal_distribution_container distrs_;
	private: vector_type ub_;
	private: vector_type lb_;
};

}} // Namespace dcs::testbed


#endif // DCS_TESTBED_GAUSSIAN_SIGNAL_GENERATOR_HPP
