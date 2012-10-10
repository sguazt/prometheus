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
#include <dcs/system/process_status_category.hpp>
#include <dcs/testbed/base_workload_driver.hpp>
#include <istream>
extern "C" {
#include <pthread.h>
}
#include <string>
#include <vector>


namespace dcs { namespace testbed {

namespace detail {

namespace /*<unnamed>*/ {

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

	return ::std::string("java");
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

	args.push_back("-cp");
	args.push_back(rain_home + "/rain.jar:" + rain_home + "/workloads/" + workload + ".jar");
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

} // Namespace <unnamed>

void* monitor_rain_rampup(void* arg);

} // Namespace detail


class rain_workload_driver: public base_workload_driver
{
	private: typedef ::dcs::system::posix_process sys_process_type;
	private: typedef ::boost::shared_ptr<sys_process_type> sys_process_pointer;


	friend void* detail::monitor_rain_rampup(void*);


	public: enum workload_category
	{
		olio_workload
	};


	public: rain_workload_driver()
	: p_thread_(0),
	  p_mutex_(0)
	{
	}

	public: ~rain_workload_driver()
	{
		if (p_thread_)
		{
			::pthread_join(*p_thread_, 0);
		}
	}

	public: rain_workload_driver(workload_category wkl_cat)
	: cmd_(detail::make_java_command()),
	  args_(detail::make_rain_args(to_string(wkl_cat))),
	  ready_(false)
	{
	}

	public: rain_workload_driver(workload_category wkl_cat,
								 ::std::string const& rain_home)
	: cmd_(detail::make_java_command()),
	  args_(detail::make_rain_args(to_string(wkl_cat), rain_home)),
	  ready_(false)
	{
	}

	public: rain_workload_driver(workload_category wkl_cat,
								 ::std::string const& rain_home,
								 ::std::string const& java_home)
	: cmd_(detail::make_java_command(java_home)),
	  args_(detail::make_rain_args(to_string(wkl_cat), rain_home)),
	  ready_(false)
	{
	}

	public: template <typename FwdIterT>
			rain_workload_driver(workload_category wkl_cat,
								 ::std::string const& rain_home,
								 ::std::string const& java_home,
								 FwdIterT arg_first,
								 FwdIterT arg_last)
	: cmd_(detail::make_java_command(java_home)),
	  args_(detail::make_rain_args(to_string(wkl_cat), rain_home, arg_first, arg_last)),
	  ready_(false)
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

	private: void ready(bool val)
	{
		::pthread_mutex_lock(p_mutex_);
			ready_ = val;
		::pthread_mutex_unlock(p_mutex_);
	}

	private: sys_process_pointer process()
	{
		return p_proc_;
	}

	private: sys_process_pointer process() const
	{
		return p_proc_;
	}

	private: void do_start()
	{
		if (p_proc_ && p_proc_->alive())
		{
			p_proc_->terminate();
		}

		ready_ = false;
		p_proc_ = ::boost::make_shared<sys_process_type>(cmd_);

		p_proc_->asynch(true);
		p_proc_->run(args_.begin(), args_.end(), false, false, true);

		if (p_proc_->status() != ::dcs::system::running_process_status)
		{
		   DCS_EXCEPTION_THROW(::std::runtime_error,
							   "Unable to start RAIN workload driver");
		}

		//TODO: create a thread that monitor the output of the process
		::pthread_create(p_thread_, 0, &detail::monitor_rain_rampup, this);
	}

	private: void do_stop()
	{
		p_proc_->terminate();

		if (::pthread_join(*p_thread_, 0) != 0)
		{
			DCS_EXCEPTION_THROW(::std::runtime_error,
								"Unable to join thread");
		}
		p_thread_ = 0;
	}

	private: bool do_alive() const
	{
		return p_proc_->alive();
	}

	private: bool do_ready() const
	{
		return ready_;
	}


	private: ::std::string cmd_;
	private: ::std::vector< ::std::string > args_;
	private: sys_process_pointer p_proc_;
	private: bool ready_;
	private: ::pthread_t* p_thread_;
	private: ::pthread_mutex_t* p_mutex_;
}; // rain_workload_driver

namespace detail {

inline
void* monitor_rain_rampup(void* arg)
{
	rain_workload_driver* p_driver = static_cast<rain_workload_driver*>(arg);

	::std::istream& eis = p_driver->process()->error_stream();

	while (eis.good())
	{
		::std::string line;

		::std::getline(eis, line);

		// Look for "Ramp up finished!" string
		if (line.find("Ramp up finished") != ::std::string::npos)
		{
			p_driver->ready(true);
			break;
		}
	}

	return 0;
}

} // Namespace detail

}} // Namespace dcs::testbed

#endif // DCS_TESTBED_RAIN_WORKLOAD_DRIVER_HPP
