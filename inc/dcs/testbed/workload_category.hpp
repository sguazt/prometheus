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
	redis_workload, ///< Workload for the Redis project
	rubis_workload ///< Workload for the RUBiS project
};

}} // Namespace dcs::testbed

#endif // DCS_TESTBED_WORKLOAD_CATEGORY_HPP
