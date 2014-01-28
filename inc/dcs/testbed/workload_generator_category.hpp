/**
 * \file dcs/testbed/workload_generator_category.hpp
 *
 * \brief Category of workload generators.
 *
 * \author Marco Guazzone (marco.guazzone@gmail.com)
 *
 * <hr/>
 *
 * Copyright (C) 2012       Marco Guazzone (marco.guazzone@gmail.com)
 *                          [Distributed Computing System (DCS) Group,
 *                           Computer Science Institute,
 *                           Department of Science and Technological Innovation,
 *                           University of Piemonte Orientale,
 *                           Alessandria (Italy)]
 *
 * This file is part of dcsxx-testbed (below referred to as "this program").
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef DCS_TESTBED_WORKLOAD_GENERATOR_CATEGORY_HPP
#define DCS_TESTBED_WORKLOAD_GENERATOR_CATEGORY_HPP


#include <dcs/exception.hpp>
#include <dcs/string/algorithm/to_lower.hpp>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>


namespace dcs { namespace testbed {

enum workload_generator_category
{
	rain_workload_generator, ///< Workload generator based on the RAIN project
	ycsb_workload_generator ///< Workload generator based on the YCSB project
};


template <typename CharT, typename CharTraitsT>
inline
::std::basic_istream<CharT,CharTraitsT>& operator>>(::std::basic_istream<CharT,CharTraitsT>& is, workload_generator_category& wkl_gen_cat)
{
	::std::string s;
	is >> s;
	::dcs::string::to_lower(s);

	if (!s.compare("rain"))
	{
		wkl_gen_cat = rain_workload_generator;
	}
	else if (!s.compare("ycsb"))
	{
		wkl_gen_cat = ycsb_workload_generator;
	}
	else
	{
		DCS_EXCEPTION_THROW(::std::runtime_error,
							"Unknown workload generator category");
	}

	return is;
}

template <typename CharT, typename CharTraitsT>
inline
::std::basic_ostream<CharT,CharTraitsT>& operator>>(::std::basic_ostream<CharT,CharTraitsT>& os, workload_generator_category wkl_gen_cat)
{
	switch (wkl_gen_cat)
	{
		case rain_workload_generator:
			os << "rain";
			break;
		case ycsb_workload_generator:
			os << "ycsb";
			break;
	}

	return os;
}

inline
::std::string to_string(workload_generator_category wkl_cat)
{
	::std::ostringstream oss;
	oss << wkl_cat;

	return oss.str();
}

}} // Namespace dcs::testbed

#endif // DCS_TESTBED_WORKLOAD_GENERATOR_CATEGORY_HPP
