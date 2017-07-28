/**
 * \file dcs/testbed/virtual_machine_performance_category.hpp
 *
 * \brief Categories for performance metrics.
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

#ifndef DCS_TESTBED_VIRTUAL_MACHINE_PERFORMANCE_CATEGORY_HPP
#define DCS_TESTBED_VIRTUAL_MACHINE_PERFORMANCE_CATEGORY_HPP


#include <vector>


namespace dcs { namespace testbed {

enum virtual_machine_performance_category
{
	//cpu_share_virtual_machine_performance,
	cpu_util_virtual_machine_performance,
	//memory_share_virtual_machine_performance,
	memory_util_virtual_machine_performance
};


/// Gets all the available virtual machine performance categories
std::vector<virtual_machine_performance_category> virtual_machine_performance_categories()
{
	std::vector<virtual_machine_performance_category> v;

	v.push_back(cpu_util_virtual_machine_performance);
	v.push_back(memory_util_virtual_machine_performance);

	return v;
}

}} // Namespace dcs::testbed


#endif // DCS_TESTBED_VIRTUAL_MACHINE_PERFORMANCE_CATEGORY_HPP
