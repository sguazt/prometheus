/**
 * \file dcs/testbed/workload_category.hpp
 *
 * \brief Category of workloads.
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

#ifndef DCS_TESTBED_WORKLOAD_CATEGORY_HPP
#define DCS_TESTBED_WORKLOAD_CATEGORY_HPP


#include <dcs/exception.hpp>
#include <dcs/string/algorithm/to_lower.hpp>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>


namespace dcs { namespace testbed {

enum workload_category
{
	olio_workload, ///< Workload for the Apache Olio project
	rubis_workload ///< Workload for the RUBiS project
};


template <typename CharT, typename CharTraitsT>
inline
::std::basic_istream<CharT,CharTraitsT>& operator>>(::std::basic_istream<CharT,CharTraitsT>& is, workload_category& wkl_cat)
{
	::std::string s;
	is >> s;
	::dcs::string::to_lower(s);

	if (!s.compare("olio") || !s.compare("cloudstone"))
	{
		wkl_cat = olio_workload;
	}
	else if (!s.compare("rubis"))
	{
		wkl_cat = rubis_workload;
	}
	else
	{
		DCS_EXCEPTION_THROW(::std::runtime_error,
							"Unknown workload category");
	}

	return is;
}

template <typename CharT, typename CharTraitsT>
inline
::std::basic_ostream<CharT,CharTraitsT>& operator>>(::std::basic_ostream<CharT,CharTraitsT>& os, workload_category wkl_cat)
{
	switch (wkl_cat)
	{
		case olio_workload:
			os << "olio";
			break;
		case rubis_workload:
			os << "rubis";
			break;
	}

	return os;
}


inline
::std::string to_string(workload_category wkl_cat)
{
	::std::ostringstream oss;
	oss << wkl_cat;

	return oss.str();
}

}} // Namespace dcs::testbed

#endif // DCS_TESTBED_WORKLOAD_CATEGORY_HPP
