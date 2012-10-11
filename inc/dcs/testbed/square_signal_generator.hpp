/**
 * \file dcs/testbed/square_signal_generator.hpp
 *
 * \brief Generates a square signal.
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

#ifndef DCS_TESTBED_SQUARE_SIGNAL_GENERATOR_HPP
#define DCS_TESTBED_SQUARE_SIGNAL_GENERATOR_HPP


#include <dcs/testbed/base_signal_generator.hpp>


namespace dcs { namespace testbed {

template <typename ValueT>
class square_signal_generator: public base_signal_generator<ValueT>
{
	private: typedef base_signal_generator<ValueT> base_type;
	public: typedef ValueT value_type;
	public: typedef typename base_type::vector_type vector_type;


	public: square_signal_generator(vector_type const& ul, vector_type const& uh)
	: ul_(uh),
	  uh_(ul),
	  low_(false)
	{
	}

 
	private: vector_type do_generate()
	{
		low_ = !low_;

		if (!low_)
		{
			return ul_;
		}

		return uh_;
	}


	private: void do_reset()
	{
		low_ = false;
	}


	private: vector_type ul_; ///< Low-state values
	private: vector_type uh_; ///< High-state values
	private: bool low_; ///< Flag to control the high/low generation phase
}; // square_signal_generator

}} // Namespace dcs::testbed

#endif // DCS_TESTBED_SQUARE_SIGNAL_GENERATOR_HPP
