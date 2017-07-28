/**
 * \file dcs/testbed/square_signal_generator.hpp
 *
 * \brief Generates a square signal.
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

#ifndef DCS_TESTBED_SQUARE_SIGNAL_GENERATOR_HPP
#define DCS_TESTBED_SQUARE_SIGNAL_GENERATOR_HPP


#include <cmath>
#include <dcs/assert.hpp>
#include <dcs/exception.hpp>
#include <dcs/testbed/base_signal_generator.hpp>
#include <limits>


namespace dcs { namespace testbed {

namespace detail { namespace /*<unnamed>*/ {

/// Vectorized version of std::max
template <typename InFwdIt1T, typename InFwdIt2T, typename OutFwdItT>
OutFwdItT max(InFwdIt1T in1_first, InFwdIt1T in1_last, InFwdIt2T in2_first, OutFwdItT out_first)
{
	while (in1_first != in1_last)
	{
		*out_first = ::std::max(*in1_first, *in2_first);
		++in1_first;
		++in2_first;
		++out_first;
	}

	return out_first;
}

/// Vectorized version of std::min
template <typename InFwdIt1T, typename InFwdIt2T, typename OutFwdItT>
OutFwdItT min(InFwdIt1T in1_first, InFwdIt1T in1_last, InFwdIt2T in2_first, OutFwdItT out_first)
{
	while (in1_first != in1_last)
	{
		*out_first = ::std::min(*in1_first, *in2_first);
		++in1_first;
		++in2_first;
		++out_first;
	}

	return out_first;
}

}} // Namespace detail::<unnamed>


template <typename ValueT>
class square_signal_generator: public base_signal_generator<ValueT>
{
	private: typedef base_signal_generator<ValueT> base_type;
	public: typedef ValueT value_type;
	public: typedef typename base_type::vector_type vector_type;


	public: square_signal_generator(vector_type const& ul, vector_type const& uh)
	: ul_(uh),
	  uh_(ul),
	  low_(false),
	  ub_( ::std::numeric_limits<value_type>::infinity()),
	  lb_(-::std::numeric_limits<value_type>::infinity())
	{
	}

 
	private: vector_type do_generate()
	{
		low_ = !low_;

		if (!low_)
		{
			vector_type u;
			detail::max(ul_.begin(), ul_.end(), lb_.begin(), u.begin());
			return u;
		}

		vector_type u;
		detail::min(uh_.begin(), uh_.end(), ub_.begin(), u.begin());
		return u;
	}


	private: void do_reset()
	{
		low_ = false;
	}

	private: void do_upper_bound(value_type val)
	{
		ub_ = vector_type(ul_.size(), val);
	}

	private: void do_lower_bound(value_type val)
	{
		lb_ = vector_type(ul_.size(), val);
	}


	private: vector_type ul_; ///< Low-state values
	private: vector_type uh_; ///< High-state values
	private: bool low_; ///< Flag to control the high/low generation phase
	private: vector_type ub_; ///< Upper bounds
	private: vector_type lb_; ///< Lower bounds
}; // square_signal_generator

}} // Namespace dcs::testbed

#endif // DCS_TESTBED_SQUARE_SIGNAL_GENERATOR_HPP
