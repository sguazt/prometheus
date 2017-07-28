/**
 * \file dcs/testbed/registry.hpp
 *
 * \brief Global registry class.
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

#ifndef DCS_TESTBED_REGISTRY_HPP
#define DCS_TESTBED_REGISTRY_HPP


#define DCS_TESTBED_CONFIG_REGISTRY_USE_ABRAHAMS_SINGLETON
//#undef DCS_TESTBED_CONFIG_REGISTRY_USE_ABRAHAMS_SINGLETON


#if defined(DCS_TESTBED_CONFIG_REGISTRY_USE_ABRAHAMS_SINGLETON)

# include <boost/serialization/singleton.hpp>

#else // DCS_TESTBED_CONFIG_REGISTRY_USE_ABRAHAMS_SINGLETON

# include <boost/scoped_ptr.hpp>
# include <boost/thread/once.hpp>
# include <boost/utility.hpp>

#endif // DCS_TESTBED_CONFIG_REGISTRY_USE_ABRAHAMS_SINGLETON


#include <boost/shared_ptr.hpp>
#include <dcs/assert.hpp>
#include <dcs/debug.hpp>
#include <dcs/exception.hpp>
#include <dcs/testbed/base_virtual_machine_manager.hpp>
#include <map>
#include <stdexcept>


namespace dcs { namespace testbed {

#if !defined(DCS_TESTBED_CONFIG_REGISTRY_USE_ABRAHAMS_SINGLETON)

namespace detail { namespace /*<unnamed>*/ {

/**
 * \brief Thread safe lazy singleton template class.
 *
 * This class is a thread-safe lazy singleton template class, which can be used
 * during static initialization or anytime after.
 *
 * Original code found at http://www.boostcookbook.com/Recipe:/1235044
 *
 * \note
 *  - If T's constructor throws, instance() will return a null reference.
 *  - If your singleton class manages resources, you may provide a public
 *    destructor, and it will be called when the instance of your singleton
 *    class is out of scoped (see scoped_ptr docs).
 *  .
 *
 * \author Port4l, http://www.boostcookbook.com/User:/Port4l
 * \author Marco Guazzone (marco.guazzone@gmail.com)
 */
template <typename T>
class singleton: ::boost::noncopyable
{
	public: static T& instance()
	{
//		::boost::call_once(init, flag_);
		::boost::call_once(flag_, init);
		return *ptr_t_;
	}


	public: static T const& const_instance()
	{
//		::boost::call_once(init, flag_);
		::boost::call_once(flag_, init);
		return *ptr_t_;
	}


	protected: singleton() {}


	protected: virtual ~singleton() { }


	private: static void init() // never throws
	{
		ptr_t_.reset(new T());
	}


	private: static ::boost::once_flag flag_;
	private: static ::boost::scoped_ptr<T> ptr_t_;
};

template <typename T>
::boost::once_flag singleton<T>::flag_ = BOOST_ONCE_INIT;

template <typename T>
::boost::scoped_ptr<T> singleton<T>::ptr_t_(0);

}} // Namespace detail::<unnamed>

#endif // DCS_TESTBED_CONFIG_REGISTRY_USE_ABRAHAMS_SINGLETON


#if defined(DCS_TESTBED_CONFIG_REGISTRY_USE_ABRAHAMS_SINGLETON)

template <typename TraitsT>
class registry: public ::boost::serialization::singleton< registry<TraitsT> >
{
	private: typedef ::boost::serialization::singleton< registry<TraitsT> > base_type;
#else // DCS_TESTBED_CONFIG_REGISTRY_USE_ABRAHAMS_SINGLETON

template <typename TraitsT>
class registry: public detail::singleton< registry<TraitsT> >
{
	friend class detail::singleton< registry<TraitsT> >;
#endif // DCS_TESTBED_CONFIG_REGISTRY_USE_ABRAHAMS_SINGLETON
	public: typedef TraitsT traits_type;
	public: typedef base_virtual_machine_manager<traits_type> vmm_type;
	public: typedef ::boost::shared_ptr<vmm_type> vmm_pointer;
	public: typedef typename vmm_type::identifier_type vmm_identifier_type;
	private: typedef ::std::map<vmm_identifier_type,vmm_pointer> vmm_container;


	public: static registry& instance()
	{
		return base_type::get_mutable_instance();
	}

	public: static registry const& const_instance()
	{
		return base_type::get_const_instance();
	}

	public: void add_vmm(vmm_pointer const& ptr_vmm)
	{
		// pre: ptr_vmm not null
		DCS_ASSERT(ptr_vmm,
				   DCS_EXCEPTION_THROW(::std::invalid_argument,
									   "Invalid pointer to Virtual Machine Manager"));

		vmm_map_[ptr_vmm->id()] = ptr_vmm;
	}

	public: vmm_pointer vmm(vmm_identifier_type id) const
	{
		// pre: id is a valid VMM identifier
		DCS_ASSERT(vmm_map_.count(id) > 0,
				   DCS_EXCEPTION_THROW(::std::invalid_argument,
									   "Invalid Virtual Machine Manager identifier"));

		return vmm_map_.at(id);
	}

	public: bool exists_vmm(vmm_identifier_type id) const
	{
		return vmm_map_.count(id) > 0 ? true : false;
	}

//	/// Default constructor
//	protected: registry()
//	: vmm_map_()
//	{
//	}


	private: vmm_container vmm_map_;
}; // registry

}} // Namespace dcs::testbed


#endif // DCS_TESTBED_REGISTRY_HPP
