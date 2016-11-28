/**
 * \file dcs/testbed/traits.hpp
 *
 * \brief Type traits class.
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

#ifndef DCS_TESTBED_TRAITS_HPP
#define DCS_TESTBED_TRAITS_HPP


#include <boost/random/mersenne_twister.hpp>


namespace dcs { namespace testbed {

template <typename RealT=double,
		  typename UIntT=unsigned long,
		  typename RNGT=boost::random::mt19937>
struct traits
{
	typedef RealT real_type;
	typedef UIntT uint_type;
	typedef RNGT rng_type;
}; // traits

}} // Namespace dcs::testbed
#endif // DCS_TESTBED_TRAITS_HPP
