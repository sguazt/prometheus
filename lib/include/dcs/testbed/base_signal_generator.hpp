/**
 * \file dcs/testbed/base_signal_generator.hpp
 *
 * \brief Base class for signal generators.
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

#ifndef DCS_TESTBED_BASE_SIGNAL_GENERATOR_HPP
#define DCS_TESTBED_BASE_SIGNAL_GENERATOR_HPP


#include <vector>


namespace dcs { namespace testbed {

/**
 * \brief Base class for signal generators.
 *
 * \tparam TraitsT Traits type.
 *
 * \author Marco Guazzone (marco.guazzone@gmail.com)
 */
template <typename ValueT>
class base_signal_generator
{
	public: typedef ValueT value_type;
	public: typedef ::std::vector<value_type> vector_type;


	protected: base_signal_generator()
	{
		// empty
	}

	public: virtual ~base_signal_generator()
	{
		// empty
	}

	/// Generates and returns a new signal value
	public: vector_type operator()()
	{
		return do_generate();
	}

	/// Resets this generator
	public: void reset()
	{
		do_reset();
	}

	/// Sets the given value as an upper bound for the signal to be generated
	public: void upper_bound(value_type val)
	{
		do_upper_bound(val);
	}

	/// Sets the given value as a lower bound for the signal to be generated
	public: void lower_bound(value_type val)
	{
		do_lower_bound(val);
	}

	private: virtual vector_type do_generate() = 0;

	private: virtual void do_reset() = 0;

	private: virtual void do_upper_bound(value_type val) = 0;

	private: virtual void do_lower_bound(value_type val) = 0;
}; // base_signal_generator

}} // Namespace dcs::testbed


#endif // DCS_TESTBED_SIGNAL_GENERATOR_HPP
