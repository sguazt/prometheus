/**
 * \file dcs/testbed/workload_generator_category.hpp
 *
 * \brief Category of workload generators.
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
