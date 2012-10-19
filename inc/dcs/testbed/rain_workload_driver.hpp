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
#include <cctype>
#include <cerrno>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <dcs/exception.hpp>
#include <dcs/system/posix_process.hpp>
#include <dcs/system/process_status_category.hpp>
#include <dcs/testbed/base_workload_driver.hpp>
#include <fstream>
#include <istream>
#include <list>
extern "C" {
#include <pthread.h>
}
#include <sstream>
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

inline
::std::string make_rain_metrics_file_path(::std::string const& path = ".", ::std::string const& suffix = "")
{
	  return path + "/metrics-snapshots-cloudstone-001-" + suffix + ".log";
}

template <typename T>
inline
T atomic_get(::pthread_mutex_t& mutex, T const& attr)
{
	T ret;

	::pthread_mutex_lock(&mutex);
		ret = attr;
	::pthread_mutex_unlock(&mutex);

	return ret;
}

template <typename T>
inline
void atomic_set(::pthread_mutex_t& mutex, T& attr, T val)
{
	::pthread_mutex_lock(&mutex);
		attr = val;
	::pthread_mutex_unlock(&mutex);
}

} // Namespace <unnamed>

static void* thread_monitor_rain_rampup(void* arg);

static void* thread_monitor_rain_steady_state(void* arg);

} // Namespace detail


class rain_workload_driver: public base_workload_driver
{
	private: typedef ::dcs::system::posix_process sys_process_type;
	private: typedef ::boost::shared_ptr<sys_process_type> sys_process_pointer;


	friend void* detail::thread_monitor_rain_rampup(void*);
	friend void* detail::thread_monitor_rain_steady_state(void*);


	// We need to use such variable to initialize mutexes until C++11.
	// Instead, since C++11, we can replace it with 'extended initializer
	// lists' in the constructor, that is:
	//   public: rain_workload_driver(...)
	//   : ...,
	//     ready_mutex_(PTHREAD_MUTEX_INITIALIZER),
	//     obs_mutex_(PTHREAD_MUTEX_INITIALIZER)
	//   {...}
	private: static const ::pthread_mutex_t mutex_init_val_;


	public: enum workload_category
	{
		olio_workload
	};


	public: rain_workload_driver(workload_category wkl_cat)
	: cmd_(detail::make_java_command()),
	  args_(detail::make_rain_args(to_string(wkl_cat))),
	  metrics_path_(detail::make_rain_metrics_file_path()),
	  ready_(false),
	  rampup_thread_active_(false),
	  steady_thread_active_(false),
	  ready_mutex_(mutex_init_val_),
	  obs_mutex_(mutex_init_val_)
	{
	}

	public: rain_workload_driver(workload_category wkl_cat,
								 ::std::string const& rain_home)
	: cmd_(detail::make_java_command()),
	  args_(detail::make_rain_args(to_string(wkl_cat), rain_home)),
	  metrics_path_(detail::make_rain_metrics_file_path()),
	  ready_(false),
	  rampup_thread_active_(false),
	  steady_thread_active_(false),
	  ready_mutex_(mutex_init_val_),
	  obs_mutex_(mutex_init_val_)
	{
	}

	public: rain_workload_driver(workload_category wkl_cat,
								 ::std::string const& rain_home,
								 ::std::string const& java_home)
	: cmd_(detail::make_java_command(java_home)),
	  args_(detail::make_rain_args(to_string(wkl_cat), rain_home)),
	  metrics_path_(detail::make_rain_metrics_file_path()),
	  ready_(false),
	  rampup_thread_active_(false),
	  steady_thread_active_(false),
	  ready_mutex_(mutex_init_val_),
	  obs_mutex_(mutex_init_val_)
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
	  metrics_path_(detail::make_rain_metrics_file_path()),
	  ready_(false),
	  rampup_thread_active_(false),
	  steady_thread_active_(false),
	  ready_mutex_(mutex_init_val_),
	  obs_mutex_(mutex_init_val_)
	{
	}

	public: ~rain_workload_driver()
	{
		try
		{
			proc_.terminate();
		}
		catch (...)
		{
			// empty
		}
		if (rampup_thread_active_)
		{
			::pthread_cancel(rampup_thread_);
			::pthread_join(rampup_thread_, 0);
		}
		if (steady_thread_active_)
		{
			::pthread_cancel(steady_thread_);
			::pthread_join(steady_thread_, 0);
		}
	}

	public: ::std::string metrics_file_path() const
	{
		return metrics_path_;
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
		detail::atomic_set(ready_mutex_, ready_, val);
	}

	private: sys_process_type& process()
	{
		return proc_;
	}

	private: sys_process_type const& process() const
	{
		return proc_;
	}

	private: void add_observation(real_type obs)
	{
		::pthread_mutex_lock(&obs_mutex_);
			obs_.push_back(obs);
		::pthread_mutex_unlock(&obs_mutex_);
	}

	private: ::pthread_t& rampup_thread()
	{
		return rampup_thread_;
	}

	private: ::pthread_t const& rampup_thread() const
	{
		return rampup_thread_;
	}

	private: void do_start()
	{
		// Stop previously running process and thread (if any)
		if (proc_.alive())
		{
			proc_.terminate();
		}
		if (rampup_thread_active_)
		{
			pthread_cancel(rampup_thread_);
			pthread_join(rampup_thread_, 0);
		}
		if (steady_thread_active_)
		{
			pthread_cancel(steady_thread_);
			pthread_join(steady_thread_, 0);
		}

		// Run a new process
		ready_ = false;
		rampup_thread_active_ = false;
		steady_thread_active_ = false;
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
		if (::pthread_create(&rampup_thread_, 0, &detail::thread_monitor_rain_rampup, this) != 0)
		{
			::std::ostringstream oss;
			oss << "Unable to start ramp-up phase monitor thread for the RAIN workload driver: " << ::strerror(errno);

			DCS_EXCEPTION_THROW(::std::runtime_error, oss.str());
		}
		rampup_thread_active_ = true;
		// Run a thread to monitor RAIN steady-state phase
		if (::pthread_create(&steady_thread_, 0, &detail::thread_monitor_rain_steady_state, this) != 0)
		{
			::std::ostringstream oss;
			oss << "Unable to start steady-state monitor thread for the RAIN workload driver: " << ::strerror(errno);

			DCS_EXCEPTION_THROW(::std::runtime_error, oss.str());
		}
		steady_thread_active_ = true;
	}

	private: void do_stop()
	{
		proc_.terminate();

		if (rampup_thread_active_)
		{
			if (::pthread_cancel(rampup_thread_) != 0)
			{
				::std::ostringstream oss;
				oss << "Unable to cancel ramp-up phase monitor thread for the RAIN workload driver: " << ::strerror(errno);

				DCS_EXCEPTION_THROW(::std::runtime_error, oss.str());
			}
			if (::pthread_join(rampup_thread_, 0) != 0)
			{
				::std::ostringstream oss;
				oss << "Unable to join ramp-up phase monitor thread for the RAIN workload driver: " << ::strerror(errno);

				DCS_EXCEPTION_THROW(::std::runtime_error, oss.str());
			}
			rampup_thread_active_ = false;
		}

		if (steady_thread_active_)
		{
			if (::pthread_cancel(steady_thread_) != 0)
			{
				::std::ostringstream oss;
				oss << "Unable to cancel ramp-up phase monitor thread for the RAIN workload driver: " << ::strerror(errno);

				DCS_EXCEPTION_THROW(::std::runtime_error, oss.str());
			}
			if (::pthread_join(steady_thread_, 0) != 0)
			{
				::std::ostringstream oss;
				oss << "Unable to join ramp-up phase monitor thread for the RAIN workload driver: " << ::strerror(errno);

				DCS_EXCEPTION_THROW(::std::runtime_error, oss.str());
			}
			steady_thread_active_ = false;
		}
	}

	private: bool do_done() const
	{
		bool ret(false);

		::pthread_mutex_lock(&obs_mutex_);
			ret = !proc_.alive();
		::pthread_mutex_unlock(&obs_mutex_);

		return ret;
	}

	private: bool do_ready() const
	{
		return detail::atomic_get(ready_mutex_, ready_);
	}

	private: bool do_has_observation() const
	{
		bool ret(false);

		::pthread_mutex_lock(&obs_mutex_);
			ret = !obs_.empty();
		::pthread_mutex_unlock(&obs_mutex_);

		return ret;
	}

	private: ::std::vector<real_type> do_observations() const
	{
		::std::vector<real_type> obs;

		::pthread_mutex_lock(&obs_mutex_);
			while (!obs_.empty())
			{
				obs.push_back(obs_.front());
				obs_.pop_front();
			}
		::pthread_mutex_unlock(&obs_mutex_);

		return obs;
	}

/*
	private: real_type do_observation() const
	{
		//FIXME: parameterize the type of statistics the user want
		::boost::accumulators::accumulator_set< real_type, ::boost::accumulators::stats< ::boost::accumulators::tag::mean > > acc;

		::pthread_mutex_lock(&obs_mutex_);
			while (!obs_.empty())
			{
				acc(obs_.front());
				obs_.pop_front();
			}
		::pthread_mutex_unlock(&obs_mutex_);

		return ::boost::accumulators::mean(acc);
	}
*/


	private: ::std::string cmd_;
	private: ::std::vector< ::std::string > args_;
	private: ::std::string metrics_path_;
	private: bool ready_;
	private: bool rampup_thread_active_;
	private: bool steady_thread_active_;
	private: sys_process_type proc_;
	private: mutable ::std::list<real_type> obs_;
	private: ::pthread_t rampup_thread_;
	private: ::pthread_t steady_thread_;
	private: mutable ::pthread_mutex_t ready_mutex_;
	private: mutable ::pthread_mutex_t obs_mutex_;
}; // rain_workload_driver

const ::pthread_mutex_t rain_workload_driver::mutex_init_val_ = PTHREAD_MUTEX_INITIALIZER;

namespace detail {

void* thread_monitor_rain_rampup(void* arg)
{
	rain_workload_driver* p_driver = static_cast<rain_workload_driver*>(arg);

	::std::istream& is = p_driver->process().output_stream();

	while (is.good())
	{
		::std::string line;

		::std::getline(is, line);

		// Look for "Ramp up finished!" string
		if (line.find("Ramp up finished") != ::std::string::npos)
		{
			p_driver->ready(true);
			break;
		}
	}

	return 0;
}

void* thread_monitor_rain_steady_state(void* arg)
{
	// Parse the RAIN metric snapshot file
	// Available fields in a row (each field is separated by one or more white-spaces):
	// - '[' <generated-during> ']'
	// - <timestamp>
	// - <operation name>
	// - <response time>
	// - '[' <operation request> ']'
	// - <total response time>
	// - <number of observations>

DCS_DEBUG_TRACE("STEADY-STATE THREAD -- Entering");
	const ::std::size_t response_time_field(4);
	const ::std::size_t max_open_trials(5);
	const unsigned int min_zzz_time(2);
	const unsigned int max_zzz_time(10);

	rain_workload_driver* p_driver = static_cast<rain_workload_driver*>(arg);

	if (pthread_join(p_driver->rampup_thread(), 0) != 0)
	{
		::std::ostringstream oss;
		oss << "Error while joining RAIN ramp-up thread: " << ::strerror(errno);

		DCS_EXCEPTION_THROW(::std::runtime_error, oss.str());
	}

	::std::size_t trial(0);
	unsigned int zzz_time(min_zzz_time);
	::std::ifstream ifs;
	do
	{
		++trial;

DCS_DEBUG_TRACE("STEADY-STATE THREAD -- Waiting... (Trial: " << trial << "/" << max_open_trials << ", Zzz: " << zzz_time << ")");
		::sleep(zzz_time);
		++zzz_time;

		ifs.open(p_driver->metrics_file_path().c_str());
	}
	while (trial < max_open_trials && !ifs.good());

	//if (!ifs.is_open() || !ifs.good())
	if (!ifs.good())
	{
		::std::ostringstream oss;
		oss << "Cannot open file '" << p_driver->metrics_file_path() << "'";

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

DCS_DEBUG_TRACE("STEADY-STATE THREAD -- IFS STREAM -- POS: " << fpos << " - GOOD: " << ifs.good() << " - EOF: " << ifs.eof() << " - FAIL: " << ifs.fail() << " - BAD: " << ifs.bad() << " - !(): " << !static_cast<bool>(ifs));
			::std::size_t n(line.size());
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
					if (field < response_time_field)
					{
						// skip these fields
						for (; pos < n && !::std::isspace(line[pos]); ++pos)
						{
							;
						}
					}
					else
					{
						::std::size_t pos2 = pos;
						for (; pos2 < n && ::std::isdigit(line[pos2]); ++pos2)
						{
							;
						}
						long rt_ms(0); // response time is given in multiple of ms
						::std::istringstream iss(line.substr(pos, pos2-pos));
						iss >> rt_ms;
DCS_DEBUG_TRACE("STEADY-STATE THREAD -- Response Time: " << rt_ms);
						p_driver->add_observation(static_cast<double>(rt_ms));
						break;
					}
				}
			}
		}
		ifs.close();

		// Found EOF. Two possible reasons:
		// 1. There is no data to read
		// 2. There is new data but we need to refresh input buffers
		// Investigate...

		zzz_time = min_zzz_time;
		do
		{
			::sleep(zzz_time);
			zzz_time = ::std::max((zzz_time+1) % max_zzz_time, min_zzz_time);

			ifs.open(p_driver->metrics_file_path().c_str());
			ifs.seekg(0, ::std::ios_base::end);
			::std::ifstream::pos_type new_fpos(ifs.tellg());
			if (new_fpos == -1)
			{
				// Failed to get new position.
				// This may be due to a temporary situation.
				// So, instead of stop reading, try to reopen the file later,
				// until we reach a number of trials equals to max_open_trials.
				new_data = false;
			}
			else if (fpos != new_fpos)
			{
				// The file has changed, we are in case #2

				// Restart to read file from the old position
				ifs.seekg(fpos);
				new_data = true;
DCS_DEBUG_TRACE("STEADY-STATE THREAD -- SOUGHT IFS STREAM -- OLD POS: " << fpos << " - NEW POS: " << new_fpos << " - GOOD: " << ifs.good() << " - EOF: " << ifs.eof() << " - FAIL: " << ifs.fail() << " - BAD: " << ifs.bad() << " - !(): " << !static_cast<bool>(ifs));
			}
			else
			{
				// The file has not changed, maybe we are in case #1 (or simply we have to wait for other few secs)
				new_data = false;
			}
		}
		while (!p_driver->done() && !new_data);
	}
	while (!p_driver->done() && new_data);

DCS_DEBUG_TRACE("STEADY-STATE THREAD -- OUT-OF-LOOP IFS STREAM -- EOF: " << ifs.eof() << " - FAIL: " << ifs.fail() << " - BAD: " << ifs.bad() << " - !(): " << !static_cast<bool>(ifs));
	ifs.close();
DCS_DEBUG_TRACE("STEADY-STATE THREAD -- Leaving");

	return 0;
}

} // Namespace detail

}} // Namespace dcs::testbed

#endif // DCS_TESTBED_RAIN_WORKLOAD_DRIVER_HPP
