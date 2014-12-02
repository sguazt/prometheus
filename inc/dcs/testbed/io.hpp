/**
 * \file dcs/testbed/io.hpp
 *
 * \brief I/O utilities
 *
 * \author Marco Guazzone (marco.guazzone@gmail.com)
 *
 * <hr/>
 *
 * Copyright 2014 Marco Guazzone (marco.guazzone@gmail.com)
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

#ifndef DCS_TESTBED_IO_HPP
#define DCS_TESTBED_IO_HPP


#include <boost/algorithm/string.hpp>
#include <dcs/exception.hpp>
#include <dcs/testbed/application_performance_category.hpp>
#include <dcs/testbed/virtual_machine_performance_category.hpp>
#include <dcs/testbed/workload_category.hpp>
#include <dcs/testbed/workload_generator_category.hpp>
#include <stdexcept>
#include <string>
#include <iostream>


namespace dcs { namespace testbed {

template <typename CharT, typename CharTraitsT>
std::basic_istream<CharT,CharTraitsT>& operator>>(std::basic_istream<CharT,CharTraitsT>& is, application_performance_category& cat)
{
	std::string s;
	is >> s;
	boost::to_lower(s);

	if (!s.compare("rt") || !s.compare("response-time"))
	{
		cat = response_time_application_performance;
	}
	else if (!s.compare("tput") || !s.compare("throughput"))
	{
		cat = throughput_application_performance;
	}
	else
	{
		DCS_EXCEPTION_THROW(std::runtime_error, "Unknown SLO metric");
	}

	return is;
}

template <typename CharT, typename CharTraitsT>
std::basic_ostream<CharT,CharTraitsT>& operator<<(std::basic_ostream<CharT,CharTraitsT>& os, application_performance_category cat)
{
	switch (cat)
	{
		case response_time_application_performance:
			os << "response-time";
			break;
		case throughput_application_performance:
			os << "throughput";
			break;
		default:
			DCS_EXCEPTION_THROW(std::runtime_error, "Unknown SLO metric");
	}

	return os;
}

template <typename CharT, typename CharTraitsT>
std::basic_istream<CharT,CharTraitsT>& operator>>(std::basic_istream<CharT,CharTraitsT>& is, virtual_machine_performance_category& cat)
{
	std::string s;
	is >> s;
	boost::to_lower(s);

	if (!s.compare("cpu-util") || !s.compare("cpu-utilization"))
	{
		cat = cpu_util_virtual_machine_performance;
	}
	else if (!s.compare("memory-util")
			 || !s.compare("memory-utilization")
			 || !s.compare("mem-util")
			 || !s.compare("mem-utilization"))
	{
		cat = memory_util_virtual_machine_performance;
	}
	else
	{
		DCS_EXCEPTION_THROW(std::runtime_error, "Unknown VM performance category");
	}

	return is;
}

template <typename CharT, typename CharTraitsT>
std::basic_ostream<CharT,CharTraitsT>& operator<<(std::basic_ostream<CharT,CharTraitsT>& os, virtual_machine_performance_category cat)
{
	switch (cat)
	{
		case cpu_util_virtual_machine_performance:
			os << "cpu-utilization";
			break;
		case memory_util_virtual_machine_performance:
			os << "memory-utilization";
			break;
		default:
			DCS_EXCEPTION_THROW(std::runtime_error, "Unknown VM performance category");
	}

	return os;
}

template <typename CharT, typename CharTraitsT>
std::basic_istream<CharT,CharTraitsT>& operator>>(std::basic_istream<CharT,CharTraitsT>& is, workload_category& wkl_cat)
{
	std::string s;
	is >> s;
	boost::to_lower(s);

	if (!s.compare("cassandra"))
	{
		wkl_cat = cassandra_workload;
	}
	else if (!s.compare("olio") || !s.compare("cloudstone"))
	{
		wkl_cat = olio_workload;
	}
	else if (!s.compare("redis"))
	{
		wkl_cat = redis_workload;
	}
	else if (!s.compare("rubis"))
	{
		wkl_cat = rubis_workload;
	}
	else
	{
		DCS_EXCEPTION_THROW(std::runtime_error, "Unknown workload category");
	}

	return is;
}

template <typename CharT, typename CharTraitsT>
::std::basic_ostream<CharT,CharTraitsT>& operator<<(std::basic_ostream<CharT,CharTraitsT>& os, workload_category wkl_cat)
{
	switch (wkl_cat)
	{
		case cassandra_workload:
			os << "cassandra";
			break;
		case olio_workload:
			os << "olio";
			break;
		case redis_workload:
			os << "redis";
			break;
		case rubis_workload:
			os << "rubis";
			break;
	}

	return os;
}

/*
inline
::std::string to_string(workload_category wkl_cat)
{
	::std::ostringstream oss;
	oss << wkl_cat;

	return oss.str();
}
*/

template <typename CharT, typename CharTraitsT>
std::basic_istream<CharT,CharTraitsT>& operator>>(std::basic_istream<CharT,CharTraitsT>& is, workload_generator_category& wkl_gen_cat)
{
	std::string s;
	is >> s;
	boost::to_lower(s);

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
		DCS_EXCEPTION_THROW(std::runtime_error,
							"Unknown workload generator category");
	}

	return is;
}

template <typename CharT, typename CharTraitsT>
std::basic_ostream<CharT,CharTraitsT>& operator>>(std::basic_ostream<CharT,CharTraitsT>& os, workload_generator_category wkl_gen_cat)
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

/*
inline
std::string to_string(workload_generator_category wkl_cat)
{
	std::ostringstream oss;
	oss << wkl_cat;

	return oss.str();
}
*/

}} // Namespace dcs::testbed


#endif // DCS_TESTBED_IO_HPP
