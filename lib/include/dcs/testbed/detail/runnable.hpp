/**
 * \file dcs/testbed/detail/runnable.hpp
 *
 * \brief Call the \c run method of the stored object.
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

#ifndef DCS_TESTBED_DETAIL_RUNNABLE_HPP
#define DCS_TESTBED_DETAIL_RUNNABLE_HPP


#include <boost/smart_ptr.hpp>


namespace dcs { namespace testbed { namespace detail {

template <typename T>
struct runnable
{
	runnable(::boost::weak_ptr<T> const& ptr)
	: wp_(ptr)
	{
	}

	void operator()()
	{
		::boost::shared_ptr<T> sp(wp_.lock());

		sp->run();
	}

	::boost::weak_ptr<T> wp_;
}; // runnable

}}} // Namespace dcs::testbed::detail

#endif // DCS_TESTBED_DETAIL_RUNNABLE_HPP
