/**
 * \file dcs/testbed/detail/runnable.hpp
 *
 * \brief Call the \c run method of the stored object.
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
