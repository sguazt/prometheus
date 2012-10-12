/**
 * \file dcs/testbed/guassian_signal_generator.hpp
 *
 * \brief Generates signals according to a Normal distribution.
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

#ifndef DCS_TESTBED_GAUSSIAN_SIGNAL_GENERATOR_HPP
#define DCS_TESTBED_GAUSSIAN_SIGNAL_GENERATOR_HPP


#include <boost/random/normal_distribution.hpp>
#include <cstddef>
#include <dcs/assert.hpp>
#include <dcs/exception.hpp>
#include <dcs/testbed/base_signal_generator.hpp>
#include <stdexcept>
#include <vector>


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
	: rng_(rng)
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
			u[i] = distrs_[i](rng_);
		}

		return u;
	}

	private: void do_reset()
	{
		// do nothing: the generator is reset by resetting the random number generator, which should be made elsewhere.
	}


	private: random_generator_type& rng_;
	private: normal_distribution_container distrs_;
};

}} // Namespace dcs::testbed


#endif // DCS_TESTBED_GAUSSIAN_SIGNAL_GENERATOR_HPP
