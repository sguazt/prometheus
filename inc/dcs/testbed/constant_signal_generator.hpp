/**
 * \file dcs/testbed/constant_signal_generator.hpp
 *
 * \brief Generates constant signals.
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

#ifndef DCS_TESTBED_CONSTANT_SIGNAL_GENERATOR_HPP
#define DCS_TESTBED_CONSTANT_SIGNAL_GENERATOR_HPP


#include <algorithm>
#include <boost/bind.hpp>
#include <dcs/assert.hpp>
#include <dcs/exception.hpp>
#include <dcs/testbed/base_signal_generator.hpp>
#include <functional>
#include <stdexcept>


namespace dcs { namespace testbed {

template <typename ValueT>
class constant_signal_generator: public base_signal_generator<ValueT>
{
	private: typedef base_signal_generator<ValueT> base_type;
	public: typedef ValueT value_type;
	public: typedef typename base_type::vector_type vector_type;


	public: constant_signal_generator(vector_type const& u0)
	: u_(u0)
	{
	}

	private: vector_type do_generate()
	{
		return u_;
	}

	private: void do_reset()
	{
		// do nothing: the signal is constant.
	}

	private: void do_upper_bound(value_type val)
	{
		// pre: for each ui in u_ : ui <= val
		DCS_ASSERT(::std::count_if(u_.begin(), u_.end(), ::boost::bind(::std::greater<value_type>(), ::_1, val)) == 0,
				   DCS_EXCEPTION_THROW(::std::invalid_argument,
									   "Invalid upper bound: some signal value is bigger"));
	}

	private: void do_lower_bound(value_type val)
	{
		// pre: for each ui in u_ : ui >= val
		DCS_ASSERT(::std::count_if(u_.begin(), u_.end(), ::boost::bind(::std::less<value_type>(), ::_1, val)) == 0,
				   DCS_EXCEPTION_THROW(::std::invalid_argument,
									   "Invalid lower bound: some signal value is smaller"));
	}


	private: vector_type u_;
};

}} // Namespace dcs::testbed


#endif // DCS_TESTBED_CONSTANT_SIGNAL_GENERATOR_HPP
