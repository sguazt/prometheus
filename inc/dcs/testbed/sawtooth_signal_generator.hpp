/**
 * \file dcs/testbed/sawtooth_signal_generator.hpp
 *
 * \brief Generates sawtooth signals.
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

#ifndef DCS_TESTBED_SAWTOOTH_SIGNAL_GENERATOR_HPP
#define DCS_TESTBED_SAWTOOTH_SIGNAL_GENERATOR_HPP


#include <dcs/testbed/base_signal_generator.hpp>
#include <cmath>
#include <cstddef>
#include <limits>


namespace dcs { namespace testbed {

template <typename ValueT>
class sawtooth_signal_generator: public base_signal_generator<ValueT>
{
	private: typedef base_signal_generator<ValueT> base_type;
	public: typedef ValueT value_type;
	public: typedef typename base_type::vector_type vector_type;


	public: sawtooth_signal_generator(vector_type const& ul, vector_type const& uh, vector_type const& incr)
	: ul_(ul),
	  uh_(uh),
	  u_(ul),
	  h_(incr),
	  ub_(ul.size(), ::std::numeric_limits<value_type>::infinity()),
	  lb_(ul.size(),-::std::numeric_limits<value_type>::infinity())
	{
		//TODO: check preconditions on vector sizes
	}

	private: vector_type do_generate()
	{
		::std::size_t n(u_.size());
		for (::std::size_t i = 0; i < n; ++i)
		{
			u_[i] += h_[i];
			if (u_[i] > uh_[i])
			{
				u_[i] = ul_[i];
			}
			u_[i] = ::std::min(::std::max(u_[i], lb_[i]), ub_[i]);
		}

		return u_;
	}

	private: void do_reset()
	{
		u_ = ul_;
	}

	private: void do_upper_bound(value_type val)
	{
		ub_ = vector_type(ul_.size(), val);
	}

	private: void do_lower_bound(value_type val)
	{
		ub_ = vector_type(ul_.size(), val);
	}


	private: vector_type ul_; ///< Lower values
	private: vector_type uh_; ///< Higher values
	private: vector_type u_;
	private: vector_type h_;
	private: vector_type ub_; ///< Upper bound
	private: vector_type lb_; ///< Lower bound
};

}} // Namespace dcs::testbed


#endif // DCS_TESTBED_SAWTOOTH_SIGNAL_GENERATOR_HPP
