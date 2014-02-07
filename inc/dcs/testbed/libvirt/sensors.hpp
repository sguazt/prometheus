/**
 * \file dcs/testbed/libvirt/sensors.hpp
 *
 * \brief A set of sensor classes for libvirt-based Virtual Machines.
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

#ifndef DCS_TESTBED_LIBVIRT_SENSORS_HPP
#define DCS_TESTBED_LIBVIRT_SENSORS_HPP


#include <boost/cstdint.hpp>
#include <cerrno>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <dcs/assert.hpp>
#include <dcs/exception.hpp>
#include <dcs/testbed/base_sensor.hpp>
#include <dcs/testbed/libvirt/detail/utility.hpp>
#include <iomanip>
#include <iostream>
#include <libvirt/libvirt.h>
#include <libvirt/virterror.h>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unistd.h>


namespace dcs { namespace testbed { namespace libvirt {

template <typename TraitsT>
class cpu_utilization_sensor: public base_sensor<TraitsT>
{
	private: typedef base_sensor<TraitsT> base_type;
	public: typedef typename base_type::traits_type traits_type;
	public: typedef typename base_type::observation_type observation_type;
	private: typedef typename traits_type::real_type real_type;


	public: cpu_utilization_sensor(::virConnectPtr p_conn, ::virDomainPtr p_dom)
	: p_conn_(p_conn),
	  p_dom_(p_dom),
	  cpu_util_(0),
	  first_(true),
	  norm_(true)
	{
	}

	public: void normalized(bool value)
	{
		norm_ = value;
	}

	public: bool normalized() const
	{
		return norm_;
	}

	private: void do_sense()
	{
		int ret;

		if (!first_)
		{
			prev_time_ = cur_time_;
			prev_node_info_ = cur_node_info_;
		}

		ret = ::clock_gettime(CLOCK_REALTIME, &cur_time_);
		if (-1 == ret)
		{
			::std::ostringstream oss;
			oss << "Failed to get clock time info: " << ::std::strerror(errno);

			DCS_EXCEPTION_THROW(::std::runtime_error, oss.str());
		}

		// Get the CPU time used (in ns)
		ret = ::virDomainGetInfo(p_dom_, &cur_node_info_);
		if (-1 == ret)
		{
			::std::ostringstream oss;
			oss << "Failed to get domain info: " << detail::last_error(p_conn_);

			DCS_EXCEPTION_THROW(::std::runtime_error, oss.str());
		}

		if (first_)
		{
			first_ = false;
			
			prev_time_ = cur_time_;
			prev_node_info_ = cur_node_info_;
		}
		else
		{
			::boost::uint64_t ns_elapsed = (cur_time_.tv_sec-prev_time_.tv_sec)*1.0e9+(cur_time_.tv_nsec-prev_time_.tv_nsec);
			::boost::uint64_t ns_used = cur_node_info_.cpuTime-prev_node_info_.cpuTime;

			cpu_util_ = static_cast<double>(ns_used/static_cast<long double>(ns_elapsed));

			if (norm_)
			{
				int nvcpus = detail::num_vcpus(p_conn_, p_dom_, VIR_DOMAIN_VCPU_MAXIMUM);

				cpu_util_ /= static_cast<double>(nvcpus);
			}
DCS_DEBUG_TRACE("nsec-used: " << ns_used << " - nsec-elaps: " << ns_elapsed << " --> UTIL: " << static_cast<double>(ns_used/static_cast<long double>(ns_elapsed)));//XXX
		}
	}

	private: void do_reset()
	{
		cpu_util_ = 0;
		first_ = true;
//		prev_time_ = cur_time_ = 0;
	}

	private: bool do_has_observations() const
	{
		return !first_;
	}

	private: ::std::vector<observation_type> do_observations() const
	{
		::std::vector<observation_type> obs;

		if (this->has_observations())
		{
			observation_type ob(0, "", cpu_util_);
			obs.push_back(ob);
		}

		return obs;
	}


	private: ::virConnectPtr p_conn_;
	private: ::virDomainPtr p_dom_;
	private: real_type cpu_util_;
	private: bool first_;
	private: bool norm_;
	private: ::timespec prev_time_;
	private: ::timespec cur_time_;
	private: ::virDomainInfo prev_node_info_;
	private: ::virDomainInfo cur_node_info_;
}; // cpu_utilization_sensor

}}} // Namespace dcs::testbed::libvirt

#endif // DCS_TESTBED_LIBVIRT_SENSORS_HPP
