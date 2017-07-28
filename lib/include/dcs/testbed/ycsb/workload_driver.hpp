/**
 * \file dcs/testbed/ycsb/workload_driver.hpp
 *
 * \brief Workload driver based on the YCSB workload toolkit.
 *
 * \author Marco Guazzone (marco.guazzone@gmail.com)
 *
 * <hr/>
 *
 * Copyright 2014 Marco Guazzone (marco.guazzone@gmail.com)
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

#ifndef DCS_TESTBED_YCSB_WORKLOAD_DRIVER_HPP
#define DCS_TESTBED_YCSB_WORKLOAD_DRIVER_HPP


#include <boost/smart_ptr.hpp>
#include <boost/thread.hpp>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <dcs/debug.hpp>
#include <dcs/exception.hpp>
#include <dcs/logging.hpp>
#include <dcs/system/posix_process.hpp>
#include <dcs/system/process_status_category.hpp>
#include <dcs/testbed/application_performance_category.hpp>
#include <dcs/testbed/base_sensor.hpp>
#include <dcs/testbed/base_workload_driver.hpp>
#include <dcs/testbed/ycsb/sensors.hpp>
#include <dcs/testbed/workload_category.hpp>
#include <dcs/testbed/workload_generator_category.hpp>
#include <exception>
#include <fstream>
#include <istream>
#include <map>
#include <sstream>
#include <string>
#include <vector>


namespace dcs { namespace testbed { namespace ycsb {

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
 * \brief Build arguments to pass to YCSB workload toolkit.
 *
 * The basic structure of the YCSB command is:
 * <pre>
 *  ycsb <command> <database> [options]
 * </pre>
 */
template <typename WklIterT, typename ArgIterT>
::std::vector< ::std::string > make_ycsb_args(workload_category wkl_cat,
											  ::std::string const& ycsb_home,
											  ::std::string const& ycsb_wkl_class,
											  ::std::string const& classpath,
											  bool load_phase,
											  WklIterT first_wkl,
											  WklIterT last_wkl,
											  ArgIterT first_arg,
											  ArgIterT last_arg)
{
	::std::vector< ::std::string > args(first_arg, last_arg);

	::std::string java_cp;
	if (!classpath.empty())
	{
		if (!java_cp.empty())
		{
			java_cp += ":";
		}
		java_cp += classpath;
	}
	if (!ycsb_home.empty())
	{
		if (!java_cp.empty())
		{
			java_cp += ":";
		}
		java_cp += ycsb_home;
	}
	if (!java_cp.empty())
	{
		args.push_back("-cp");
		args.push_back(java_cp);
	}
	args.push_back("com.yahoo.ycsb.Client");
	if (load_phase)
	{
		// Load phase
		args.push_back("-load");
	}
	else
	{
		// Transaction phase
		args.push_back("-t");
	}
	args.push_back("-db");
	if (!ycsb_wkl_class.empty())
	{
		args.push_back(ycsb_wkl_class);
	}
	else
	{
		std::string wkl_class;
		switch (wkl_cat)
		{
			case cassandra_workload:
				wkl_class = "com.yahoo.ycsb.db.CassandraClient7";
				break;
			case redis_workload:
				wkl_class = "com.yahoo.ycsb.db.RedisClient";
				break;
			default:
			{
				::std::ostringstream oss;
				oss << "Workload '" << wkl_cat << "' not handled";
				DCS_EXCEPTION_THROW(::std::invalid_argument, oss.str());
			}
		}
		args.push_back(wkl_class);
	}

	args.push_back("-s");
	while (first_wkl != last_wkl)
	{
		args.push_back("-P");
		args.push_back(*first_wkl);
		++first_wkl;
	}

	return args;
}

template <typename WklIterT>
::std::vector< ::std::string > make_ycsb_args(workload_category wkl_cat,
											  ::std::string const& ycsb_home,
											  ::std::string const& ycsb_wkl_class,
											  ::std::string const& classpath,
											  bool load_phase,
											  WklIterT first_wkl,
											  WklIterT last_wkl)
{
	::std::vector< ::std::string > java_args;
	//java_args.push_back("-s");

	return make_ycsb_args(wkl_cat, ycsb_home, ycsb_wkl_class, classpath, load_phase, first_wkl, last_wkl, java_args.begin(), java_args.end());
}

template <typename WklIterT>
::std::vector< ::std::string > make_ycsb_args(workload_category wkl_cat,
											  bool load_phase,
											  WklIterT first_wkl,
											  WklIterT last_wkl)
{
	return make_ycsb_args(wkl_cat, ".", "", "", load_phase, first_wkl, last_wkl);
}

inline
::std::string make_ycsb_status_file_path(workload_category wkl_cat, ::std::string const& path = ".", ::std::string const& suffix = "")
{
	::std::string workload;
	switch (wkl_cat)
	{
		case cassandra_workload:
			workload = "cassandra";
			break;
		case redis_workload:
			workload = "redis";
			break;
		default:
		{
			::std::ostringstream oss;
			oss << "Workload '" << wkl_cat << "' not handled";
			DCS_EXCEPTION_THROW(::std::invalid_argument, oss.str());
		}
	}

//	char fname[L_tmpnam];
//	DCS_ASSERT( ::std::tmpnam(fname),
//				DCS_EXCEPTION_THROW( ::std::runtime_error,
//									 "Unable to create a name for the YCSB status file" ));

	return path + "/ycsb-status-" + workload + "-" + suffix + ".log";
}

} // Namespace <unnamed>

/// Monitors the YCSB execution
template <typename DriverT>
struct monitor_runnable;

// Redirects the YCSB standard output to a log file.
template <typename DriverT>
struct logger_runnable;

// Redirects the YCSB standard error to a log file.
template <typename DriverT>
struct status_dumper_runnable;

} // Namespace detail


template <typename TraitsT>
class workload_driver: public base_workload_driver<TraitsT>
{
	private: typedef base_workload_driver<TraitsT> base_type;
	private: typedef workload_driver<TraitsT> self_type;
	public: typedef typename base_type::traits_type traits_type;
	private: typedef ::dcs::system::posix_process sys_process_type;
	private: typedef ::boost::shared_ptr<sys_process_type> sys_process_pointer;
	public: typedef typename base_type::real_type real_type;
	public: typedef typename base_type::app_pointer app_pointer;
	public: typedef base_sensor<traits_type> sensor_type;
	public: typedef ::boost::shared_ptr<sensor_type> sensor_pointer;
	private: typedef ::boost::mutex mutex_type;


	template <typename T>
	friend struct detail::monitor_runnable;
	template <typename T>
	friend struct detail::logger_runnable;
	template <typename T>
	friend struct detail::status_dumper_runnable;


	public: template <typename WklIterT>
			workload_driver(workload_category wkl_cat,
							WklIterT first_wkl,
							WklIterT last_wkl)
	: cmd_(detail::make_java_command()),
	  args_(detail::make_ycsb_args(wkl_cat, "", "", false, first_wkl, last_wkl)),
	  status_path_(detail::make_ycsb_status_file_path(wkl_cat)),
	  ready_(false),
	  monitor_thread_active_(false),
	  logger_thread_active_(false),
	  status_dumper_thread_active_(false)
	{
	}

	public: template <typename WklIterT>
			workload_driver(workload_category wkl_cat,
							WklIterT first_wkl,
							WklIterT last_wkl,
							::std::string const& ycsb_home,
							::std::string const& ycsb_wkl_class,
							::std::string const& ycsb_classpath)
	: cmd_(detail::make_java_command()),
	  args_(detail::make_ycsb_args(wkl_cat, ycsb_home, ycsb_wkl_class, ycsb_classpath, false, first_wkl, last_wkl)),
	  status_path_(detail::make_ycsb_status_file_path(wkl_cat)),
	  ready_(false),
	  monitor_thread_active_(false),
	  logger_thread_active_(false),
	  status_dumper_thread_active_(false)
	{
	}

	public: template <typename WklIterT>
			workload_driver(workload_category wkl_cat,
							WklIterT first_wkl,
							WklIterT last_wkl,
							::std::string const& ycsb_home,
							::std::string const& ycsb_wkl_class,
							::std::string const& ycsb_classpath,
							::std::string const& java_home)
	: cmd_(detail::make_java_command(java_home)),
	  args_(detail::make_ycsb_args(wkl_cat, ycsb_home, ycsb_wkl_class, ycsb_classpath, false, first_wkl, last_wkl)),
	  status_path_(detail::make_ycsb_status_file_path(wkl_cat)),
	  ready_(false),
	  monitor_thread_active_(false),
	  logger_thread_active_(false),
	  status_dumper_thread_active_(false)
	{
	}

	public: template <typename WklIterT, typename ArgsIterT>
			workload_driver(workload_category wkl_cat,
							WklIterT first_wkl,
							WklIterT last_wkl,
							::std::string const& ycsb_home,
							::std::string const& ycsb_wkl_class,
							::std::string const& ycsb_classpath,
							::std::string const& java_home,
							ArgsIterT first_arg,
							ArgsIterT last_arg)
	: cmd_(detail::make_java_command(java_home)),
	  args_(detail::make_ycsb_args(wkl_cat, ycsb_home, ycsb_wkl_class, ycsb_classpath, false, first_wkl, last_wkl, first_arg, last_arg)),
	  status_path_(detail::make_ycsb_status_file_path(wkl_cat)),
	  ready_(false),
	  monitor_thread_active_(false),
	  logger_thread_active_(false),
	  status_dumper_thread_active_(false)
	{
	}

	public: ~workload_driver()
	{
		// Terminate process
		try
		{
			proc_.terminate();
		}
		catch (...)
		{
			// empty
		}
		// Remove threads
		if (this->monitor_thread_active())
		{
			monitor_thread_.interrupt();
			monitor_thread_.join();
		}
		if (this->logger_thread_active())
		{
			logger_thread_.interrupt();
			logger_thread_.join();
		}
		if (this->status_dumper_thread_active())
		{
			status_dumper_thread_.interrupt();
			status_dumper_thread_.join();
		}
	}

	public: ::std::string status_file_path() const
	{
		return status_path_;
	}

	public: sensor_pointer sensor(application_performance_category cat) const
	{
		switch (cat)
		{
			case throughput_application_performance:
				return ::boost::make_shared< throughput_sensor<traits_type> >(status_path_);
			default:
				break;
		}

		::std::ostringstream oss;
		oss << "Application performance metric '" << cat << "' not handled";
		DCS_EXCEPTION_THROW(::std::invalid_argument, oss.str());
	}

	public: std::map<application_performance_category,sensor_pointer> sensors() const
	{
		std::map<application_performance_category,sensor_pointer> map;

		map[throughput_application_performance] = this->sensor(throughput_application_performance);

		return map;
	}


	private: void ready(bool val)
	{
		::boost::lock_guard<mutex_type> lock(ready_mutex_);

		ready_ = val;
	}

	private: sys_process_type& process()
	{
		return proc_;
	}

	private: sys_process_type const& process() const
	{
		return proc_;
	}

	private: ::boost::thread& monitor_thread()
	{
		return monitor_thread_;
	}

	private: ::boost::thread const& monitor_thread() const
	{
		return monitor_thread_;
	}

	private: void monitor_thread_active(bool val)
	{
		::boost::lock_guard<mutex_type> lock(monitor_thread_mutex_);

		monitor_thread_active_ = val;
	}

	private: bool monitor_thread_active() const
	{
		::boost::lock_guard<mutex_type> lock(monitor_thread_mutex_);

		return monitor_thread_active_;
	}

	private: ::boost::thread& logger_thread()
	{
		return logger_thread_;
	}

	private: ::boost::thread const& logger_thread() const
	{
		return logger_thread_;
	}

	private: void logger_thread_active(bool val)
	{
		::boost::lock_guard<mutex_type> lock(logger_thread_mutex_);

		logger_thread_active_ = val;
	}

	private: bool logger_thread_active() const
	{
		::boost::lock_guard<mutex_type> lock(logger_thread_mutex_);

		return logger_thread_active_;
	}

	private: ::boost::thread& status_dumper_thread()
	{
		return status_dumper_thread_;
	}

	private: ::boost::thread const& status_dumper_thread() const
	{
		return status_dumper_thread_;
	}

	private: void status_dumper_thread_active(bool val)
	{
		::boost::lock_guard<mutex_type> lock(status_dumper_thread_mutex_);

		status_dumper_thread_active_ = val;
	}

	private: bool status_dumper_thread_active() const
	{
		::boost::lock_guard<mutex_type> lock(status_dumper_thread_mutex_);

		return status_dumper_thread_active_;
	}

/*
	private: void do_app(app_pointer const& p_app)
	{
		p_app_ = p_app;
	}

	private: app_pointer do_app()
	{
		return p_app_;
	}

	private: app_pointer do_app() const
	{
		return p_app_;
	}
*/

	private: workload_generator_category do_category() const
	{
		return ycsb_workload_generator;
	}

	private: void do_reset()
	{
		// Stop previously running process and thread (if any)
		if (proc_.alive())
		{
			proc_.terminate();
		}
		if (this->monitor_thread_active())
		{
			monitor_thread_.interrupt();
			monitor_thread_.join();
			this->monitor_thread_active(false);
		}
		if (this->logger_thread_active())
		{
			logger_thread_.interrupt();
			logger_thread_.join();
			this->logger_thread_active(false);
		}
		if (this->status_dumper_thread_active())
		{
			status_dumper_thread_.interrupt();
			status_dumper_thread_.join();
			this->status_dumper_thread_active(false);
		}

		ready_ = false;
	}

	private: void do_start()
	{
		this->reset();

		// Run a new process
		proc_.command(cmd_);
		proc_.asynch(true);
		proc_.run(args_.begin(), args_.end(), false, true, true);
		if (proc_.status() != ::dcs::system::running_process_status)
		{
			::std::ostringstream oss;
			oss << "Unable to start YCSB workload driver: " << ::strerror(errno);

			DCS_EXCEPTION_THROW(::std::runtime_error, oss.str());
		}

		// Run a thread to monitor YCSB execution
		try
		{
			detail::monitor_runnable<self_type> runner(this);
			monitor_thread_ = ::boost::thread(runner);
		}
		catch (::std::exception const& e)
		{
			::std::ostringstream oss;
			oss << "Unable to start monitor thread for the YCSB workload driver: " << e.what();

			DCS_EXCEPTION_THROW(::std::runtime_error, oss.str());
		}
	}

	private: void do_stop()
	{
		proc_.terminate();

		if (this->monitor_thread_active())
		{
			try
			{
				monitor_thread_.interrupt();
				monitor_thread_.join();
			}
			catch (::std::exception const& e)
			{
				::std::ostringstream oss;
				oss << "Unable to join monitor thread for the YCSB workload driver: " << e.what();

				DCS_EXCEPTION_THROW(::std::runtime_error, oss.str());
			}
			this->monitor_thread_active(false);
		}

		if (this->logger_thread_active())
		{
			try
			{
				logger_thread_.interrupt();
				logger_thread_.join();
			}
			catch (::std::exception const& e)
			{
				::std::ostringstream oss;
				oss << "Unable to join logger thread for the YCSB workload driver: " << e.what();

				DCS_EXCEPTION_THROW(::std::runtime_error, oss.str());
			}
			this->logger_thread_active(false);
		}

		if (this->status_dumper_thread_active())
		{
			try
			{
				status_dumper_thread_.interrupt();
				status_dumper_thread_.join();
			}
			catch (::std::exception const& e)
			{
				::std::ostringstream oss;
				oss << "Unable to join status dumper thread for the YCSB workload driver: " << e.what();

				DCS_EXCEPTION_THROW(::std::runtime_error, oss.str());
			}
			this->status_dumper_thread_active(false);
		}
	}

	private: bool do_done() const
	{
		return !proc_.alive();
	}

	private: bool do_ready() const
	{
		::boost::lock_guard<mutex_type> lock(ready_mutex_);

		return ready_;
	}


	private: ::std::string cmd_;
	private: ::std::vector< ::std::string > args_;
	private: ::std::string status_path_;
	private: bool ready_;
	private: bool monitor_thread_active_;
	private: bool logger_thread_active_;
	private: bool status_dumper_thread_active_;
//	private: app_pointer p_app_;
	private: sys_process_type proc_;
	private: ::boost::thread monitor_thread_;
	private: ::boost::thread logger_thread_;
	private: ::boost::thread status_dumper_thread_;
	private: mutable mutex_type ready_mutex_;
	private: mutable mutex_type monitor_thread_mutex_;
	private: mutable mutex_type logger_thread_mutex_;
	private: mutable mutex_type status_dumper_thread_mutex_;
}; // workload_driver


namespace detail {

template <typename DriverT>
struct monitor_runnable
{
	monitor_runnable(DriverT* p_drv)
	: p_drv_(p_drv)
	{
	}

	void operator()()
	{
		DCS_DEBUG_TRACE("MONITOR THREAD -- Entering");

		p_drv_->monitor_thread_active(true);

		::std::istream& is = p_drv_->process().output_stream();

		while (is.good())
		{
			::std::string line;

			::std::getline(is, line);

			// Look for "Ramp up finished!" string
			if (line.find("Command line:") != ::std::string::npos)
			{
				p_drv_->ready(true);
				break;
			}
		}

		p_drv_->monitor_thread_active(false);

		// Run a thread to log the YCSB standard output
		try
		{
			p_drv_->logger_thread_ = ::boost::thread(logger_runnable<DriverT>(p_drv_));
		}
		catch (::std::exception const& e)
		{
			::std::ostringstream oss;
			oss << "Unable to start measurements logger thread for the YCSB workload driver: " << e.what();

			DCS_EXCEPTION_THROW(::std::runtime_error, oss.str());
		}
		// Run a thread to catch the YCSB standard error
		try
		{
			p_drv_->status_dumper_thread_ = ::boost::thread(status_dumper_runnable<DriverT>(p_drv_));
		}
		catch (::std::exception const& e)
		{
			::std::ostringstream oss;
			oss << "Unable to start status dumper thread for the YCSB workload driver: " << e.what();

			DCS_EXCEPTION_THROW(::std::runtime_error, oss.str());
		}

		DCS_DEBUG_TRACE("MONITOR THREAD -- Leaving");
	}


	DriverT* p_drv_;
}; // monitor_runnable

template <typename DriverT>
struct logger_runnable
{
	logger_runnable(DriverT* p_drv)
	: p_drv_(p_drv)
	{
	}

	void operator()()
	{
		DCS_DEBUG_TRACE("LOGGER THREAD -- Entering");

		p_drv_->logger_thread_active(true);

		::std::istream& is = p_drv_->process().output_stream();

		while (is.good())
		{
			::std::string line;

			::std::getline(is, line);

			::dcs::log_info(DCS_LOGGING_AT, line);
		}

		p_drv_->logger_thread_active(false);

		DCS_DEBUG_TRACE("LOGGER THREAD -- Leaving");
	}


	DriverT* p_drv_;
}; // logger_runnable

template <typename DriverT>
struct status_dumper_runnable
{
	status_dumper_runnable(DriverT* p_drv)
	: p_drv_(p_drv)
	{
	}

	void operator()()
	{
		DCS_DEBUG_TRACE("STATUS DUMPER THREAD -- Entering");

		p_drv_->status_dumper_thread_active(true);

		::std::istream& is = p_drv_->process().error_stream();
		if (is.good())
		{
			::std::ofstream ofs(p_drv_->status_file_path().c_str());

			if (ofs.good())
			{
				while (is.good())
				{
					::std::string line;

					::std::getline(is, line);

					DCS_DEBUG_TRACE("READ FROM ERROR: " << line);//XXX

					ofs << line << ::std::endl;
				}

				ofs.close();
			}
		}

		p_drv_->status_dumper_thread_active(false);

		DCS_DEBUG_TRACE("STATUS DUMPER THREAD -- Leaving");
	}


	DriverT* p_drv_;
}; // status_dumper_runnable

} // Namespace detail

}}} // Namespace dcs::testbed::ycsb

#endif // DCS_TESTBED_YCSB_WORKLOAD_DRIVER_HPP
