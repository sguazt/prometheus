/**
 * \file dcs/testbed/constant_signal_generator.hpp
 *
 * \brief Generates constant signals.
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

#ifndef DCS_TESTBED_CONSTANT_SIGNAL_GENERATOR_HPP
#define DCS_TESTBED_CONSTANT_SIGNAL_GENERATOR_HPP


#include <dcs/testbed/base_signal_generator.hpp>


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


	private: vector_type u_;
};

}} // Namespace dcs::testbed


#endif // DCS_TESTBED_CONSTANT_SIGNAL_GENERATOR_HPP
