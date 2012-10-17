/**
 * \file dcs/testbed/uniform_signal_generator.hpp
 *
 * \brief Generates signals according to a uniform probability distribution.
 *
 * \author Marco Guazzone (marco.guazzone@gmail.com)
 *
 * <hr/>
 *
 * Copyright (C) 2012       Marco Guazzone
 *                          [Distributed Computing System (DCS) Group,
 *                           Computer Science Institute,
 *                           Department of Science and Technological Innovation,
 *                           University of Piemonte Orientale,
 *                           Alessandria (Italy)]
 *
 * This file is part of dcsxx-testbed.
 *
 * dcsxx-testbed is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * dcsxx-testbed is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with dcsxx-testbed. If not, see <http://www.gnu.org/licenses/>.
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
