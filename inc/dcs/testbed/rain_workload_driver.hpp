/**
 * \file dcs/testbed/rain_workload_driver.hpp
 *
 * \brief Workload driver based on the RAIN workload toolkit.
 *
 * \author Marco Guazzone (marco.guazzone@gmail.com)
 *
 * <hr/>
 *
 * Copyright (C) 2012       Marco Guazzone
 *                          [Distributed Computing System (DCS) Group,
 *                           Computer Science Institute,
 *                           Department of Science and Technological Innovation,
 *                           University of Piemonte Orientale,
 *                           Alessandria (Italy)]
 *
 * This file is part of dcsxx-testbed.
 *
 * dcsxx-testbed is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * dcsxx-testbed is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with dcsxx-testbed. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef DCS_TESTBED_RAIN_WORKLOAD_DRIVER_HPP
#define DCS_TESTBED_RAIN_WORKLOAD_DRIVER_HPP


#include <boost/smart_ptr.hpp>
#include <cstdlib>
#include <dcs/exception.hpp>
#include <dcs/system/posix_process.hpp>
#include <dcs/testbed/base_workload_driver.hpp>
#include <string>
#include <vector>


namespace dcs { namespace testbed {

namespace detail { namespace /*<unnamed>*/ {

inline
::std::string make_java_command(::std::string const& java_home)
{
	return java_home + "/bin/java";
}

inline
::std::string make_java_command()
{
	char* java_path = ::std::getenv("JAVA_HOME");
	if (java_path)
	{
		return make_java_command(::std::string(java_path));
	}

	java_path = ::std::getenv("JRE_HOME");
	if (java_path)
	{
		return make_java_command(::std::string(java_path));
	}

	return ::std::string();
}

/**
 * \brief Build arguments to pass to RAIN workload toolkit.
 *
 * The basic structure of the RAIN command is:
 * <pre>
 *  java [<java-arg1> ... <java-argN>] \
 *       -cp "rain.jar:<path to workload JAR>" \
 *       radlab.rain.Benchmark <path to Rain JSON configuration file>
 * </pre>
 */
template <typename FwdIterT>
inline
::std::vector< ::std::string > make_rain_args(::std::string const& workload,
											  ::std::string const& rain_home,
											  FwdIterT arg_first,
											  FwdIterT arg_last)
{
	::std::vector< ::std::string > args(arg_first, arg_last);

	args.push_back("-cp \"" + rain_home + "/rain.jar:" + rain_home + "/workloads/" + workload + ".jar\"");
	args.push_back("radlab.rain.Benchmark");
	args.push_back(rain_home + "/config/rain.config." + workload + ".json");

	return args;
}

inline
::std::vector< ::std::string > make_rain_args(::std::string const& workload,
											  ::std::string const& rain_home)
{
	::std::vector< ::std::string > java_args;
	java_args.push_back("-Xmx1g");
	java_args.push_back("-Xms256m");

	return make_rain_args(workload, rain_home, java_args.begin(), java_args.end());
}

inline
::std::vector< ::std::string > make_rain_args(::std::string const& workload)
{
	return make_rain_args(workload, ".");
}

}} // Namespace detail::<unnamed>


class rain_workload_driver: public base_workload_driver
{
	private: typedef ::dcs::system::posix_process sys_process_type;
	private: typedef ::boost::shared_ptr<sys_process_type> sys_process_pointer;


	public: enum workload_category
	{
		olio_workload
	};


	public: rain_workload_driver(workload_category wkl_cat)
	: cmd_(detail::make_java_command()),
	  args_(detail::make_rain_args(to_string(wkl_cat)))
	{
	}

	public: rain_workload_driver(workload_category wkl_cat,
								 ::std::string const& rain_home)
	: cmd_(detail::make_java_command()),
	  args_(detail::make_rain_args(to_string(wkl_cat), rain_home))
	{
	}

	public: rain_workload_driver(workload_category wkl_cat,
								 ::std::string const& rain_home,
								 ::std::string const& java_home)
	: cmd_(detail::make_java_command(java_home)),
	  args_(detail::make_rain_args(to_string(wkl_cat), rain_home))
	{
	}

	public: template <typename FwdIterT>
			rain_workload_driver(workload_category wkl_cat,
								 ::std::string const& rain_home,
								 ::std::string const& java_home,
								 FwdIterT arg_first,
								 FwdIterT arg_last)
	: cmd_(detail::make_java_command(java_home)),
	  args_(detail::make_rain_args(to_string(wkl_cat), rain_home, arg_first, arg_last))
	{
	}

	private: static ::std::string to_string(workload_category wkl_cat)
	{
		switch (wkl_cat)
		{
			case olio_workload:
				return "olio";
		}

		DCS_EXCEPTION_THROW(::std::invalid_argument,
							"Unknown workload category");
	}

	private: void do_start()
	{
		if (p_proc_ && p_proc_->alive())
		{
			p_proc_->terminate();
		}
		p_proc_ = ::boost::make_shared<sys_process_type>(cmd_);

		p_proc_->asynch(true);
		p_proc_->run(args_.begin(), args_.end());

		if (p_proc_->status() != ::dcs::system::running_process_status)
		{
		   DCS_EXCEPTION_THROW(::std::runtime_error,
							   "Unable to start RAIN workload driver");
		}
	}

	private: void do_stop()
	{
		p_proc_->terminate();
	}


	private: ::std::string cmd_;
	private: ::std::vector< ::std::string > args_;
	private: sys_process_pointer p_proc_;
}; // rain_workload_driver

}} // Namespace dcs::testbed

#endif // DCS_TESTBED_RAIN_WORKLOAD_DRIVER_HPP
