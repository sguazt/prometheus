/**
 * \file dcs/testbed/rain/workload_driver.hpp
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
#include <dcs/testbed/rain/sensors.hpp>
#include <dcs/testbed/workload_category.hpp>
#include <dcs/testbed/workload_generator_category.hpp>
#include <exception>
#include <fstream>
#include <istream>
#include <list>
#include <sstream>
#include <string>
#include <vector>


namespace dcs { namespace testbed { namespace rain {

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
::std::vector< ::std::string > make_rain_args(workload_category wkl_cat,
											  ::std::string const& rain_home,
											  FwdIterT arg_first,
											  FwdIterT arg_last)
{
	::std::vector< ::std::string > args(arg_first, arg_last);

	::std::string workload;
	switch (wkl_cat)
	{
		case olio_workload:
			workload = "olio";
			break;
		case rubis_workload:
			workload = "rubis";
			break;
		default:
		{
			::std::ostringstream oss;
			oss << "Workload '" << to_string(wkl_cat) << "' not handled";
			DCS_EXCEPTION_THROW(::std::invalid_argument, oss.str());
		}
	}

	args.push_back("-cp");
	args.push_back(rain_home + "/rain.jar:" + rain_home + "/workloads/" + workload + ".jar");
	args.push_back("radlab.rain.Benchmark");
	args.push_back(rain_home + "/config/rain.config." + workload + ".json");

	return args;
}

inline
::std::vector< ::std::string > make_rain_args(workload_category wkl_cat,
											  ::std::string const& rain_home)
{
	::std::vector< ::std::string > java_args;
	java_args.push_back("-Xmx1g");
	java_args.push_back("-Xms256m");

	return make_rain_args(wkl_cat, rain_home, java_args.begin(), java_args.end());
}

inline
::std::vector< ::std::string > make_rain_args(workload_category wkl_cat)
{
	return make_rain_args(wkl_cat, ".");
}

inline
::std::string make_rain_metrics_file_path(workload_category wkl_cat, ::std::string const& path = ".", ::std::string const& suffix = "")
{
	::std::string workload;
	switch (wkl_cat)
	{
		case olio_workload:
			workload = "cloudstone";
			break;
		case rubis_workload:
			workload = "rubis";
			break;
		default:
		{
			::std::ostringstream oss;
			oss << "Workload '" << to_string(wkl_cat) << "' not handled";
			DCS_EXCEPTION_THROW(::std::invalid_argument, oss.str());
		}
	}

	return path + "/metrics-snapshots-" + workload + "-001-" + suffix + ".log";
}

//template <typename MutexT, typename T>
//inline
//T atomic_get(MutexT& mutex, T const& attr)
//{
//	::boost::lock_guard<MutexT> lock(mutex);
//	return attr;
//}

//template <typename MutexT, typename T>
//inline
//void atomic_set(MutexT& mutex, T& attr, T val)
//{
//	::boost::lock_guard<MutexT> lock(mutex);
//	attr = val;
//}

} // Namespace <unnamed>

/// Monitors the RAIN ramp-up (transient) phase
template <typename DriverT>
struct rampup_monitor_runnable;

/**
 * \brief Monitors the RAIN steady-state phase and parses the RAIN metrics
 *  snapshot file.
 */
template <typename DriverT>
struct steady_state_monitor_runnable;

/// Redirects the RAIN steady-state standard output to a log file.
template <typename DriverT>
struct steady_state_logger_runnable;

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
	public: typedef typename base_type::observation_type observation_type;
	public: typedef base_sensor<traits_type> sensor_type;
	public: typedef ::boost::shared_ptr<sensor_type> sensor_pointer;
	private: typedef ::boost::mutex mutex_type;


	template <typename T>
	friend struct detail::rampup_monitor_runnable;
	template <typename T>
	friend struct detail::steady_state_monitor_runnable;
	template <typename T>
	friend struct detail::steady_state_logger_runnable;


	public: workload_driver(workload_category wkl_cat)
	: cmd_(detail::make_java_command()),
	  args_(detail::make_rain_args(wkl_cat)),
	  metrics_path_(detail::make_rain_metrics_file_path(wkl_cat)),
	  ready_(false),
	  rampup_thread_active_(false),
	  steady_thread_active_(false),
	  logger_thread_active_(false)
	{
	}

	public: workload_driver(workload_category wkl_cat,
							::std::string const& rain_home)
	: cmd_(detail::make_java_command()),
	  args_(detail::make_rain_args(wkl_cat, rain_home)),
	  metrics_path_(detail::make_rain_metrics_file_path(wkl_cat)),
	  ready_(false),
	  rampup_thread_active_(false),
	  steady_thread_active_(false),
	  logger_thread_active_(false)
	{
	}

	public: workload_driver(workload_category wkl_cat,
							::std::string const& rain_home,
							::std::string const& java_home)
	: cmd_(detail::make_java_command(java_home)),
	  args_(detail::make_rain_args(wkl_cat, rain_home)),
	  metrics_path_(detail::make_rain_metrics_file_path(wkl_cat)),
	  ready_(false),
	  rampup_thread_active_(false),
	  steady_thread_active_(false),
	  logger_thread_active_(false)
	{
	}

	public: template <typename FwdIterT>
			workload_driver(workload_category wkl_cat,
							::std::string const& rain_home,
							::std::string const& java_home,
							FwdIterT arg_first,
							FwdIterT arg_last)
	: cmd_(detail::make_java_command(java_home)),
	  args_(detail::make_rain_args(wkl_cat, rain_home, arg_first, arg_last)),
	  metrics_path_(detail::make_rain_metrics_file_path(wkl_cat)),
	  ready_(false),
	  rampup_thread_active_(false),
	  steady_thread_active_(false),
	  logger_thread_active_(false)
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
		if (this->rampup_thread_active())
		{
			rampup_thread_.interrupt();
			rampup_thread_.join();
		}
		if (this->steady_state_thread_active())
		{
			steady_thread_.interrupt();
			steady_thread_.join();
		}
		if (this->logger_thread_active())
		{
			logger_thread_.interrupt();
			logger_thread_.join();
		}
	}

	public: ::std::string metrics_file_path() const
	{
		return metrics_path_;
	}

	public: sensor_pointer sensor(application_performance_category cat) const
	{
		switch (cat)
		{
			case response_time_application_performance:
				return ::boost::make_shared< response_time_sensor<traits_type> >(metrics_path_);
		}

		DCS_EXCEPTION_THROW(::std::runtime_error, "Unknown sensor category");
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

	private: void add_observation(observation_type const& obs)
	{
		::boost::lock_guard<mutex_type> lock(obs_mutex_);

		obs_.push_back(obs);
	}

	private: void add_observation(::std::time_t ts,
								  ::std::string const& op,
								  real_type val)
	{
		this->add_observation(base_type::make_observation(ts, op, val));
	}

	private: ::boost::thread& rampup_thread()
	{
		return rampup_thread_;
	}

	private: ::boost::thread const& rampup_thread() const
	{
		return rampup_thread_;
	}

	private: void rampup_thread_active(bool val)
	{
		::boost::lock_guard<mutex_type> lock(rampup_thread_mutex_);

		rampup_thread_active_ = val;
	}

	private: bool rampup_thread_active() const
	{
		::boost::lock_guard<mutex_type> lock(rampup_thread_mutex_);

		return rampup_thread_active_;
	}

	private: ::boost::thread& steady_state_thread()
	{
		return steady_thread_;
	}

	private: ::boost::thread const& steady_state_thread() const
	{
		return steady_thread_;
	}

	private: void steady_state_thread_active(bool val)
	{
		::boost::lock_guard<mutex_type> lock(steady_thread_mutex_);

		steady_thread_active_ = val;
	}

	private: bool steady_state_thread_active() const
	{
		::boost::lock_guard<mutex_type> lock(steady_thread_mutex_);

		return steady_thread_active_;
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

	private: workload_generator_category do_category() const
	{
		return rain_workload_generator;
	}

	private: void do_reset()
	{
		// Stop previously running process and thread (if any)
		if (proc_.alive())
		{
			proc_.terminate();
		}
		if (this->rampup_thread_active())
		{
			rampup_thread_.interrupt();
			rampup_thread_.join();
			this->rampup_thread_active(false);
		}
		if (this->steady_state_thread_active())
		{
			steady_thread_.interrupt();
			steady_thread_.join();
			this->steady_state_thread_active(false);
		}
		if (this->logger_thread_active())
		{
			logger_thread_.interrupt();
			logger_thread_.join();
			this->logger_thread_active(false);
		}

		ready_ = false;
	}

	private: void do_start()
	{
		this->reset();

		// Run a new process
		proc_.command(cmd_);
		proc_.asynch(true);
		proc_.run(args_.begin(), args_.end(), false, true);
		if (proc_.status() != ::dcs::system::running_process_status)
		{
			::std::ostringstream oss;
			oss << "Unable to start RAIN workload driver: " << ::strerror(errno);

			DCS_EXCEPTION_THROW(::std::runtime_error, oss.str());
		}

		// Run a thread to monitor RAIN ramp-up (transient) phase
		try
		{
			detail::rampup_monitor_runnable<self_type> runner(this);
			rampup_thread_ = ::boost::thread(runner);
		}
		catch (::std::exception const& e)
		{
			::std::ostringstream oss;
			oss << "Unable to start ramp-up phase monitor thread for the RAIN workload driver: " << e.what();

			DCS_EXCEPTION_THROW(::std::runtime_error, oss.str());
		}
	}

	private: void do_stop()
	{
		proc_.terminate();

		if (this->rampup_thread_active())
		{
			try
			{
				rampup_thread_.interrupt();
				rampup_thread_.join();
			}
			catch (::std::exception const& e)
			{
				::std::ostringstream oss;
				oss << "Unable to join ramp-up phase monitor thread for the RAIN workload driver: " << e.what();

				DCS_EXCEPTION_THROW(::std::runtime_error, oss.str());
			}
			this->rampup_thread_active(false);
		}

		if (this->steady_state_thread_active())
		{
			try
			{
				steady_thread_.interrupt();
				steady_thread_.join();
			}
			catch (::std::exception const& e)
			{
				::std::ostringstream oss;
				oss << "Unable to join steady-state phase monitor thread for the RAIN workload driver: " << e.what();

				DCS_EXCEPTION_THROW(::std::runtime_error, oss.str());
			}
			this->steady_state_thread_active(false);
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
				oss << "Unable to join steady-state phase output logger thread for the RAIN workload driver: " << e.what();

				DCS_EXCEPTION_THROW(::std::runtime_error, oss.str());
			}
			this->logger_thread_active(false);
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

	private: bool do_has_observation() const
	{
		::boost::lock_guard<mutex_type> lock(obs_mutex_);

		return !obs_.empty();
	}

	private: ::std::vector<observation_type> do_observations() const
	{
		::std::vector<observation_type> obs;

		::boost::lock_guard<mutex_type> lock(obs_mutex_);

		while (!obs_.empty())
		{
			obs.push_back(obs_.front());
			obs_.pop_front();
		}

		return obs;
	}


	private: ::std::string cmd_;
	private: ::std::vector< ::std::string > args_;
	private: ::std::string metrics_path_;
	private: bool ready_;
	private: bool rampup_thread_active_;
	private: bool steady_thread_active_;
	private: bool logger_thread_active_;
	private: sys_process_type proc_;
	private: mutable ::std::list<observation_type> obs_;
	private: ::boost::thread rampup_thread_;
	private: ::boost::thread steady_thread_;
	private: ::boost::thread logger_thread_;
	private: mutable mutex_type ready_mutex_;
	private: mutable mutex_type obs_mutex_;
	private: mutable mutex_type rampup_thread_mutex_;
	private: mutable mutex_type steady_thread_mutex_;
	private: mutable mutex_type logger_thread_mutex_;
}; // workload_driver


namespace detail {

template <typename DriverT>
struct rampup_monitor_runnable
{
	rampup_monitor_runnable(DriverT* p_drv)
	: p_drv_(p_drv)
	{
	}

	void operator()()
	{
		DCS_DEBUG_TRACE("RAMP-UP THREAD -- Entering");

		p_drv_->rampup_thread_active(true);

		::std::istream& is = p_drv_->process().output_stream();

		while (is.good())
		{
			::std::string line;

			::std::getline(is, line);

			// Look for "Ramp up finished!" string
			if (line.find("Ramp up finished") != ::std::string::npos)
			{
				p_drv_->ready(true);
				break;
			}
		}

		p_drv_->rampup_thread_active(false);

		// Run a thread to monitor RAIN steady-state phase
		try
		{
			p_drv_->steady_thread_ = ::boost::thread(steady_state_monitor_runnable<DriverT>(p_drv_));
		}
		catch (::std::exception const& e)
		{
			::std::ostringstream oss;
			oss << "Unable to start steady-state monitor thread for the RAIN workload driver: " << e.what();

			DCS_EXCEPTION_THROW(::std::runtime_error, oss.str());
		}
		// Run a thread to log the RAIN standard output during the steady-state phase
		try
		{
			p_drv_->logger_thread_ = ::boost::thread(steady_state_logger_runnable<DriverT>(p_drv_));
		}
		catch (::std::exception const& e)
		{
			::std::ostringstream oss;
			oss << "Unable to start steady-state output logger thread for the RAIN workload driver: " << e.what();

			DCS_EXCEPTION_THROW(::std::runtime_error, oss.str());
		}

		DCS_DEBUG_TRACE("RAMP-UP THREAD -- Leaving");
	}


	DriverT* p_drv_;
}; // rampup_monitor_runnable

template <typename DriverT>
struct steady_state_monitor_runnable
{
	steady_state_monitor_runnable(DriverT* p_drv)
	: p_drv_(p_drv)
	{
	}

	void operator()()
	{
		// Available fields in a row (each field is separated by one or more white-spaces):
		// - '[' <generated-during> ']'
		// - <timestamp>
		// - <operation name>
		// - <response time>
		// - '[' <operation request> ']'
		// - <total response time>
		// - <number of observations>

	DCS_DEBUG_TRACE("STEADY-STATE THREAD -- Entering");
		const ::std::size_t timestamp_field(2);
		const ::std::size_t operation_field(3);
		const ::std::size_t response_time_field(4);
		const ::std::size_t max_open_trials(50);
		const unsigned int min_zzz_time(2);
		const unsigned int max_zzz_time(10);

		p_drv_->steady_state_thread_active(true);

		::std::size_t trial(0);
		unsigned int zzz_time(min_zzz_time);
		::std::ifstream ifs;
		do
		{
			++trial;

	DCS_DEBUG_TRACE("STEADY-STATE THREAD -- Waiting... (Trial: " << trial << "/" << max_open_trials << ", Zzz: " << zzz_time << ")");
			::sleep(zzz_time);
			++zzz_time;

			ifs.open(p_drv_->metrics_file_path().c_str());
		}
		while (trial < max_open_trials && !ifs.good());

		//if (!ifs.is_open() || !ifs.good())
		if (!ifs.good())
		{
			::std::ostringstream oss;
			oss << "Cannot open file '" << p_drv_->metrics_file_path() << "'";

			DCS_EXCEPTION_THROW(::std::runtime_error, oss.str());
		}

		// Emulate the behavior of 'tail -f' command
		::std::ifstream::pos_type fpos(0);
		bool new_data(false);
		do
		{
			while (ifs.good())
			{
				fpos = ifs.tellg();

				::std::string line;

				::std::getline(ifs, line);

	DCS_DEBUG_TRACE("STEADY-STATE THREAD -- IFS STREAM -- LINE: " << line << " - POS: " << fpos << " - GOOD: " << ifs.good() << " - EOF: " << ifs.eof() << " - FAIL: " << ifs.fail() << " - BAD: " << ifs.bad() << " - !(): " << !static_cast<bool>(ifs));

				const ::std::size_t n(line.size());

				if (!ifs.good() || n == 0)
				{
					continue;
				}

				::std::time_t obs_ts(0); // timestamp (in secs from Epoch)
				::std::string obs_op; // Operation label
				long obs_rtms(0); // response time (in ms)
				::std::size_t field(0);
				for (::std::size_t pos = 0; pos < n; ++pos)
				{
					// eat all heading space
					for (; pos < n && ::std::isspace(line[pos]); ++pos)
					{
						;
					}
					if (pos < n)
					{
						++field;

						switch (field)
						{
							case timestamp_field:
							{
								::std::size_t pos2(pos);
								for (; pos2 < n && ::std::isdigit(line[pos2]); ++pos2)
								{
									;
								}
								::std::istringstream iss(line.substr(pos, pos2-pos));
								iss >> obs_ts;
	DCS_DEBUG_TRACE("STEADY-STATE THREAD -- Timestamp: " << obs_ts);
								pos = pos2;
								break;
							}
							case operation_field:
							{
								::std::size_t pos2(pos);
								for (; pos2 < n && ::std::isalpha(line[pos2]); ++pos2)
								{
									;
								}
								obs_op = line.substr(pos, pos2-pos);
	DCS_DEBUG_TRACE("STEADY-STATE THREAD -- Operation: " << obs_op);
								pos = pos2;
								break;
							}
							case response_time_field:
							{
								::std::size_t pos2(pos);
								for (; pos2 < n && ::std::isdigit(line[pos2]); ++pos2)
								{
									;
								}
								::std::istringstream iss(line.substr(pos, pos2-pos));
								iss >> obs_rtms;
	DCS_DEBUG_TRACE("STEADY-STATE THREAD -- Response Time: " << obs_rtms);
								pos = pos2;
								break;
							}
							default:
								// skip these fields
								for (; pos < n && !::std::isspace(line[pos]); ++pos)
								{
									;
								}
								break;
						}
					}
				}

				p_drv_->add_observation(obs_ts, obs_op, obs_rtms);
			}

			// Found EOF. Two possible reasons:
			// 1. There is no data to read
			// 2. There is new data but we need to refresh input buffers
			// Investigate...

			zzz_time = min_zzz_time;
			new_data = false;
			do
			{
	DCS_DEBUG_TRACE("STEADY-STATE THREAD -- Zzz... (" << zzz_time << ")");
				// Sleep for a while, just to let new data arrive
				::sleep(zzz_time);
				zzz_time = ::std::max((zzz_time+1) % max_zzz_time, min_zzz_time);
	DCS_DEBUG_TRACE("STEADY-STATE THREAD -- Cheking for new data");

				// Close to stream in order to avoid possible failures during the following call to open
				ifs.close();
				// Reopen the stream "at end"
				ifs.open(p_drv_->metrics_file_path().c_str(), ::std::ios_base::ate);
				if (ifs.good())
				{
					ifs.sync();
					::std::ifstream::pos_type new_fpos(ifs.tellg());
	DCS_DEBUG_TRACE("STEADY-STATE THREAD -- REOPENED (good) -- OLD POS: " << fpos << " - NEW POS: " << new_fpos << " - GOOD: " << ifs.good() << " - EOF: " << ifs.eof() << " - FAIL: " << ifs.fail() << " - BAD: " << ifs.bad() << " - !(): " << !static_cast<bool>(ifs) << " - IN_AVAIL: " << ifs.rdbuf()->in_avail());
					if (fpos != new_fpos)
					{
						// The file has changed, we are in case #2

						// Restart to read file from the old position
						ifs.seekg(fpos);
						new_data = true;
	DCS_DEBUG_TRACE("STEADY-STATE THREAD -- SOUGHT IFS STREAM -- OLD POS: " << fpos << " - NEW POS: " << new_fpos << " - GOOD: " << ifs.good() << " - EOF: " << ifs.eof() << " - FAIL: " << ifs.fail() << " - BAD: " << ifs.bad() << " - !(): " << !static_cast<bool>(ifs));
					}
	//				else
	//				{
	//					// We are here because
	//					// - new_fpos==-1 -> Failed to get new position.
	//					//   This may be due to a temporary situation.
	//					//   So, instead of stop reading, try to reopen the file later.
	//					// - fpos==new_fpos -> The file has not changed, maybe we are in case #1 (or simply we have to wait for other few secs)
	//					new_data = false;
	//				}
				}
	//			else
	//			{
	//DCS_DEBUG_TRACE("STEADY-STATE THREAD -- REOPENED (failed) -- OLD POS: " << fpos << " - GOOD: " << ifs.good() << " - EOF: " << ifs.eof() << " - FAIL: " << ifs.fail() << " - BAD: " << ifs.bad() << " - !(): " << !static_cast<bool>(ifs));
	//				new_data = false;
	//			}
	DCS_DEBUG_TRACE("STEADY-STATE THREAD -- Cheking for new data --> " << new_data);
			}
			//while (!p_drv_->done() && !new_data);
			while (!new_data);
		}
		//while (!p_drv_->done() && new_data);
		while (new_data);

	DCS_DEBUG_TRACE("STEADY-STATE THREAD -- OUT-OF-LOOP IFS STREAM -- EOF: " << ifs.eof() << " - FAIL: " << ifs.fail() << " - BAD: " << ifs.bad() << " - !(): " << !static_cast<bool>(ifs));
		if (ifs.is_open())
		{
			ifs.close();
		}
	DCS_DEBUG_TRACE("STEADY-STATE THREAD -- Leaving");

		p_drv_->steady_state_thread_active(false);
	}


	DriverT* p_drv_;
}; // steady_state_monitor_runnable

template <typename DriverT>
struct steady_state_logger_runnable
{
	steady_state_logger_runnable(DriverT* p_drv)
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
}; // steady_state_logger_runnable

} // Namespace detail

}}} // Namespace dcs::testbed::rain

#endif // DCS_TESTBED_RAIN_WORKLOAD_DRIVER_HPP
