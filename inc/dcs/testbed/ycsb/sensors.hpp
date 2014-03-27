/**
 * \file dcs/testbed/ycsb/sensors.hpp
 *
 * \brief Sensor for YCSB-driven applications.
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

#ifndef DCS_TESTBED_YCSB_SENSORS_HPP
#define DCS_TESTBED_YCSB_SENSORS_HPP


//#include <boost/date_time/gregorian/gregorian_types.hpp>
//#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <cctype>
#include <cstddef>
#include <ctime>
#include <dcs/debug.hpp>
#include <dcs/testbed/base_sensor.hpp>
#include <fstream>
#include <ios>
#include <string>
#include <vector>


namespace dcs { namespace testbed { namespace ycsb {

namespace detail { namespace /*<unnamed>*/ {

::std::time_t make_timestamp(int year, int month, int day, int hour, int min, int sec, int msec)
{
//	namespace bpt = boost::posix_time;
//	namespace bgr = boost::gregorian;
//	namespace bdt = boost::date_time;
//
//	bpt::ptime dt = bpt::ptime(bgr::date(year, month, day), bpt::hours(hour)+bpt::minutes(min)+bpt::seconds(sec)+bpt::milliseconds(msec));
//	bpt::ptime start_epoch(bgr::date(1970,1,1));
 //   bdt::time_duration diff = dt-start_epoch;
//	return static_cast< ::std::time_t >(diff.total_seconds());
	::std::tm t;
	t.tm_sec = (sec+msec/1000) % 60;
	t.tm_min = min % 60;
	t.tm_hour = hour % 24;
	t.tm_mday = day % 32;
	t.tm_mon = (month-1) % 12;
	t.tm_year = year-1900;
	t.tm_wday = t.tm_yday = t.tm_isdst = -1;

	return ::std::mktime(&t);
}

}} // Namespace detail::<unnamed>


template <typename TraitsT>
class throughput_sensor: public base_sensor<TraitsT>
{
	private: typedef base_sensor<TraitsT> base_type;
	public: typedef typename base_type::traits_type traits_type;
	public: typedef typename base_type::observation_type observation_type;
	private: typedef typename traits_type::real_type real_type;


	public: explicit throughput_sensor(::std::string const& status_file_path, bool interval_throughput=true)
	: status_file_(status_file_path),
	  int_tput_(interval_throughput),
	  fpos_(0)
//	  new_data_(false)
	{
	}

	private: void do_sense()
	{
		DCS_DEBUG_TRACE("BEGIN Do Sense");

		// Available fields in a row (each field is separated by one or more white-spaces):
		// - <timestamp> sec:
		// - <tot number of ops> operations;
		// - <last interval's throughput> current ops/sec;
		// - [UPDATE AverageLatency(us)=<update latency>]
		// - [READ AverageLatency(us)=<read latency>]

		const ::std::string noname_op("<no-name>");
		const ::std::size_t timestamp_field(1);
		const ::std::size_t elapsed_field(2);
		const ::std::size_t num_operations_field(3);
		const ::std::size_t relative_throughput_field(4);
		//const ::std::size_t update_latency_field(4);
		//const ::std::size_t read_latency_field(5);

		// reset last sensing
		obs_.clear();

		if (!ifs_.good() || !ifs_.is_open())
		{
			// Found EOF. Two possible reasons:
			// 1. There is no data to read
			// 2. There is new data but we need to refresh input buffers
			// Investigate...

			if (ifs_.is_open())
			{
				ifs_.close();
			}
			ifs_.open(status_file_.c_str(), ::std::ios_base::ate);
			if (ifs_.good())
			{
				ifs_.sync();
				::std::ifstream::pos_type new_fpos(ifs_.tellg());
//DCS_DEBUG_TRACE("REOPENED (good) -- OLD POS: " << fpos_ << " - NEW POS: " << new_fpos << " - GOOD: " << ifs_.good() << " - EOF: " << ifs_.eof() << " - FAIL: " << ifs_.fail() << " - BAD: " << ifs_.bad() << " - !(): " << !static_cast<bool>(ifs_) << " - IN_AVAIL: " << ifs_.rdbuf()->in_avail());
				if (fpos_ != new_fpos)
				{
					// The file has changed, we are in case #2

					// Restart to read file from the old position
					ifs_.seekg(fpos_);
//					new_data_ = true;
//DCS_DEBUG_TRACE("SOUGHT IFS STREAM -- OLD POS: " << fpos_ << " - NEW POS: " << new_fpos << " - GOOD: " << ifs_.good() << " - EOF: " << ifs_.eof() << " - FAIL: " << ifs_.fail() << " - BAD: " << ifs_.bad() << " - !(): " << !static_cast<bool>(ifs_));
				}
				else
				{
					ifs_.close();
				}
			}
		}

		// Collect all available status entries
		while (ifs_.good() && ifs_.is_open())
		{
			fpos_ = ifs_.tellg();

			::std::string line;

			::std::getline(ifs_, line);

//DCS_DEBUG_TRACE("IFS STREAM -- LINE: " << line << " - POS: " << fpos_ << " - GOOD: " << ifs_.good() << " - EOF: " << ifs_.eof() << " - FAIL: " << ifs_.fail() << " - BAD: " << ifs_.bad() << " - !(): " << !static_cast<bool>(ifs_));

			const ::std::size_t n(line.size());

			if (!ifs_.good() || n == 0)
			{
				continue;
			}

			::std::time_t obs_ts = 0; // timestamp (in secs)
			::std::time_t obs_elapsed = 0; // timestamp (in secs from the beginning of the experiment)
			unsigned long obs_nops = 0; // number of operations from the beginning of the experiment
			real_type obs_rel_tput = 0; // relative throughput (i.e., the throughput of the last sampling interval)
			::std::size_t field = 0;
			bool done = false;
			for (::std::size_t pos = 0; pos < n && !done; ++pos)
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
							// Format: yyyy-mm-dd HH:MM:SS:ttt

							unsigned short year = 0;
							unsigned short month = 0;
							unsigned short day = 0;
							unsigned short hour = 0;
							unsigned short min = 0;
							unsigned short sec = 0;
							unsigned short msec = 0;

							::std::size_t pos2(pos);

							// Parse year (yyyy)
							for (; pos2 < n && ::std::isdigit(line[pos2]); ++pos2)
							{
								;
							}
							if (pos2 == n)
							{
								// This line does not contain useful data
								continue;
							}
							::std::istringstream iss(line.substr(pos, pos2-pos));
							iss >> year;
							for (; pos2 < n && line[pos2] != '-'; ++pos2)
							{
								;
							}
							pos = pos2;
							// Parse month (mm)
							for (; pos2 < n && ::std::isdigit(line[pos2]); ++pos2)
							{
								;
							}
							if (pos2 == n)
							{
								// This line does not contain useful data
								continue;
							}
							iss.str("");
							iss.str(line.substr(pos, pos2-pos));
							iss >> month;
							for (; pos2 < n && line[pos2] != '-'; ++pos2)
							{
								;
							}
							pos = pos2;
							// Parse day (dd)
							for (; pos2 < n && ::std::isdigit(line[pos2]); ++pos2)
							{
								;
							}
							if (pos2 == n)
							{
								// This line does not contain useful data
								continue;
							}
							iss.str("");
							iss.str(line.substr(pos, pos2-pos));
							iss >> day;
							for (; pos2 < n && isspace(line[pos2]); ++pos2)
							{
								;
							}
							pos = pos2;
							// Parse hour (HH)
							for (; pos2 < n && ::std::isdigit(line[pos2]); ++pos2)
							{
								;
							}
							if (pos2 == n)
							{
								// This line does not contain useful data
								continue;
							}
							iss.str("");
							iss.str(line.substr(pos, pos2-pos));
							iss >> hour;
							for (; pos2 < n && line[pos2] != ':'; ++pos2)
							{
								;
							}
							pos = pos2;
							// Parse minutes (MM)
							for (; pos2 < n && ::std::isdigit(line[pos2]); ++pos2)
							{
								;
							}
							if (pos2 == n)
							{
								// This line does not contain useful data
								continue;
							}
							iss.str("");
							iss.str(line.substr(pos, pos2-pos));
							iss >> min;
							for (; pos2 < n && line[pos2] != ':'; ++pos2)
							{
								;
							}
							pos = pos2;
							// Parse seconds (SS)
							for (; pos2 < n && ::std::isdigit(line[pos2]); ++pos2)
							{
								;
							}
							if (pos2 == n)
							{
								// This line does not contain useful data
								continue;
							}
							iss.str("");
							iss.str(line.substr(pos, pos2-pos));
							iss >> sec;
							for (; pos2 < n && line[pos2] != ':'; ++pos2)
							{
								;
							}
							pos = pos2;
							// Parse milliseconds (ttt)
							for (; pos2 < n && ::std::isdigit(line[pos2]); ++pos2)
							{
								;
							}
							if (pos2 == n)
							{
								// This line does not contain useful data
								continue;
							}
							iss.str("");
							iss.str(line.substr(pos, pos2-pos));
							iss >> msec;
							obs_ts = detail::make_timestamp(year, month, day, hour, min, sec, msec);
//DCS_DEBUG_TRACE("Timestamp: " << obs_ts);
							pos = pos2;
							break;
						}
						case elapsed_field:
						{
							::std::size_t pos2(pos);
							for (; pos2 < n && ::std::isdigit(line[pos2]); ++pos2)
							{
								;
							}
							if (pos2 == n)
							{
								// This line does not contain useful data
								continue;
							}
							::std::istringstream iss(line.substr(pos, pos2-pos));
							iss >> obs_elapsed;
//DCS_DEBUG_TRACE("Timestamp: " << obs_elapsed);
							for (; pos2 < n && line[pos2] != ':'; ++pos2)
							{
								;
							}
							pos = pos2;
							break;
						}
						case num_operations_field:
						{
							::std::size_t pos2(pos);
							//for (; pos2 < n && ::std::isalpha(line[pos2]); ++pos2)
							for (; pos2 < n && !::std::isspace(line[pos2]); ++pos2)
							{
								;
							}
							if (pos2 == n)
							{
								// This line does not contain useful data
								continue;
							}
							::std::istringstream iss(line.substr(pos, pos2-pos));
							iss >> obs_nops;
//DCS_DEBUG_TRACE("# Operations: " << obs_nops);
							for (; pos2 < n && line[pos2] != ';'; ++pos2)
							{
								;
							}
							pos = pos2;
							if (!int_tput_)
							{
								done = true;
							}
							break;
						}
						case relative_throughput_field:
						{
							::std::size_t pos2(pos);
							//for (; pos2 < n && ::std::isalpha(line[pos2]); ++pos2)
							for (; pos2 < n && !::std::isspace(line[pos2]); ++pos2)
							{
								;
							}
							if (pos2 == n)
							{
								// This line does not contain useful data
								continue;
							}
							::std::istringstream iss(line.substr(pos, pos2-pos));
							iss >> obs_rel_tput;
//DCS_DEBUG_TRACE("Interval Throughput: " << rel_tput);
							for (; pos2 < n && line[pos2] != ';'; ++pos2)
							{
								;
							}
							pos = pos2;
							if (int_tput_)
							{
								done = true;
							}
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

			if (obs_elapsed > 0)
			{
				DCS_DEBUG_TRACE("Found observation: " << obs_ts << ", " << obs_elapsed << ", " << obs_nops << ", " << obs_rel_tput);

				if (int_tput_)
				{
					obs_.push_back(observation_type(obs_elapsed, noname_op, obs_nops/obs_elapsed));
				}
				else
				{
					obs_.push_back(observation_type(obs_elapsed, noname_op, obs_rel_tput));
				}
			}
		}

		DCS_DEBUG_TRACE("END Do Sense");
	}

	private: void do_reset()
	{
		if (ifs_.is_open())
		{
			ifs_.close();
		}
		fpos_ = 0;
		//new_data_ = false;
		obs_.clear();
	}

	private: bool do_has_observations() const
	{
		return obs_.size() > 0;
	}

	private: ::std::vector<observation_type> do_observations() const
	{
		return obs_;
	}


	private: ::std::string status_file_;
	private: bool int_tput_; ///< A flag which indicates if the the last sampling interval throughput (a \c true value) or the incremental throughput (a \c false value) should be sensed
	private: ::std::ifstream ifs_;
	private: ::std::ifstream::pos_type fpos_;
//	private: bool new_data_;
	private: ::std::vector<observation_type> obs_;
}; // throughput_sensor

}}} // Namespace dcs::testbed::ycsb

#endif // DCS_TESTBED_YCSB_SENSORS_HPP
