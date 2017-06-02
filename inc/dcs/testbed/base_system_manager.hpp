/**
 * \file dcs/testbed/base_system_manager.hpp
 *
 * \brief Base class for system managers.
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

#ifndef DCS_TESTBED_BASE_SYSTEM_MANAGER_HPP
#define DCS_TESTBED_BASE_SYSTEM_MANAGER_HPP


#include <boost/shared_ptr.hpp>
#include <dcs/testbed/base_virtual_machine.hpp>
#include <vector>


namespace dcs { namespace testbed {

/**
 * \brief Base class for system managers.
 *
 * \tparam TraitsT Traits type.
 *
 * \author Marco Guazzone (marco.guazzone@gmail.com)
 */
template <typename TraitsT>
class base_system_manager
{
	public: typedef TraitsT traits_type;
	protected: typedef base_virtual_machine<traits_type> vm_type;
	protected: typedef ::boost::shared_ptr<vm_type> vm_pointer;


	protected: base_system_manager()
	{
		// Empty
	}

	public: virtual ~base_system_manager()
	{
		// Empty
	}

	/// Manages the given VMs
	public: template <typename IterT>
			void manage(IterT first, IterT last)
	{
		::std::vector<vm_pointer> vms(first, last);

		this->do_manage(vms);
	}

	private: virtual void do_manage(::std::vector<vm_pointer> const& vms) = 0;
}; // base_system_manager

}} // Namespace dcs::testbed

#endif // DCS_TESTBED_BASE_SYSTEM_MANAGER_HPP
