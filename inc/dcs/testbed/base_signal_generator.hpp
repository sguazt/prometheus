/**
 * \file dcs/testbed/base_signal_generator.hpp
 *
 * \brief Base class for signal generators.
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

#ifndef DCS_TESTBED_BASE_SIGNAL_GENERATOR_HPP
#define DCS_TESTBED_BASE_SIGNAL_GENERATOR_HPP


#include <vector>


namespace dcs { namespace testbed {

template <typename ValueT>
class base_signal_generator
{
	public: typedef ValueT value_type;
	public: typedef ::std::vector<value_type> vector_type;


	public: vector_type operator()()
	{
		return do_generate();
	}

	public: void reset()
	{
		do_reset();
	}

	public: void upper_bound(value_type val)
	{
		do_upper_bound(val);
	}

	public: void lower_bound(value_type val)
	{
		do_lower_bound(val);
	}

	public: virtual ~base_signal_generator()
	{
		// empty
	}

	private: virtual vector_type do_generate() = 0;

	private: virtual void do_reset() = 0;

	private: virtual void do_upper_bound(value_type val) = 0;

	private: virtual void do_lower_bound(value_type val) = 0;
}; // base_signal_generator

}} // Namespace dcs::testbed


#endif // DCS_TESTBED_SIGNAL_GENERATOR_HPP
