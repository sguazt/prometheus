/**
 * \file dcs/testbed/workload_category.hpp
 *
 * \brief Category of workloads.
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
	cassandra_workload, ///< Workload for the Apache Cassandra project
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

	if (!s.compare("cassandra"))
	{
		wkl_cat = cassandra_workload;
	}
	else if (!s.compare("olio") || !s.compare("cloudstone"))
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
		case cassandra_workload:
			os << "cassandra";
			break;
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
