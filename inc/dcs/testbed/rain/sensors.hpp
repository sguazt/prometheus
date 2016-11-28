/**
 * \file dcs/testbed/rain/sensors.hpp
 *
 * \brief Sensor for RAIN-driven applications.
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

#ifndef DCS_TESTBED_RAIN_SENSORS_HPP
#define DCS_TESTBED_RAIN_SENSORS_HPP


#include <cctype>
#include <cstddef>
#include <ctime>
#include <dcs/debug.hpp>
#include <dcs/testbed/base_sensor.hpp>
#include <fstream>
#include <ios>
#include <string>
#include <vector>


namespace dcs { namespace testbed { namespace rain {

template <typename TraitsT>
class response_time_sensor: public base_sensor<TraitsT>
{
	private: typedef base_sensor<TraitsT> base_type;
	public: typedef typename base_type::traits_type traits_type;
	public: typedef typename base_type::observation_type observation_type;


	public: response_time_sensor(std::string const& metrics_file_path)
	: metrics_file_(metrics_file_path),
	  fpos_(0)
//	  new_data_(false)
	{
	}

	private: void do_sense()
	{
		DCS_DEBUG_TRACE("BEGIN Do Sense");

		// Available fields in a row (each field is separated by one or more white-spaces):
		// - '[' <generated-during> ']'
		// - <finished-timestamp>
		// - <operation name>
		// - <response time>
		// - '[' <operation request> ']'
		// - <total response time>
		// - <number of observations>

		const std::size_t timestamp_field(2);
		const std::size_t operation_field(3);
		const std::size_t response_time_field(4);
		//const std::size_t max_open_trials(50);

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
			ifs_.open(metrics_file_.c_str(), std::ios_base::ate);
			if (ifs_.good())
			{
				ifs_.sync();
				std::ifstream::pos_type new_fpos(ifs_.tellg());
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

		// Collect all available metrics entries
		while (ifs_.good() && ifs_.is_open())
		{
			fpos_ = ifs_.tellg();

			std::string line;

			std::getline(ifs_, line);

//DCS_DEBUG_TRACE("IFS STREAM -- LINE: " << line << " - POS: " << fpos_ << " - GOOD: " << ifs_.good() << " - EOF: " << ifs_.eof() << " - FAIL: " << ifs_.fail() << " - BAD: " << ifs_.bad() << " - !(): " << !static_cast<bool>(ifs_));

			const std::size_t n(line.size());

			if (!ifs_.good() || n == 0)
			{
				continue;
			}

			std::time_t obs_ts = 0; // timestamp (in secs from Epoch)
			std::string obs_op; // Operation label
			long obs_rtns = 0; // response time (in ns)
			std::size_t field = 0;
			for (std::size_t pos = 0; pos < n; ++pos)
			{
				// eat all heading space
				for (; pos < n && std::isspace(line[pos]); ++pos)
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
							std::size_t pos2(pos);
							for (; pos2 < n && std::isdigit(line[pos2]); ++pos2)
							{
								;
							}
							std::istringstream iss(line.substr(pos, pos2-pos));
							iss >> obs_ts;
//DCS_DEBUG_TRACE("Timestamp: " << obs_ts);
							pos = pos2;
							break;
						}
						case operation_field:
						{
							std::size_t pos2(pos);
							//for (; pos2 < n && std::isalpha(line[pos2]); ++pos2)
							for (; pos2 < n && !std::isspace(line[pos2]); ++pos2)
							{
								;
							}
							obs_op = line.substr(pos, pos2-pos);
//DCS_DEBUG_TRACE("Operation: " << obs_op);
							pos = pos2;
							break;
						}
						case response_time_field:
						{
							std::size_t pos2(pos);
							for (; pos2 < n && std::isdigit(line[pos2]); ++pos2)
							{
								;
							}
							std::istringstream iss(line.substr(pos, pos2-pos));
							iss >> obs_rtns;
//DCS_DEBUG_TRACE("Response Time (nsecs): " << obs_rtns);
							pos = pos2;
							break;
						}
						default:
							// skip these fields
							for (; pos < n && !std::isspace(line[pos]); ++pos)
							{
								;
							}
							break;
					}
				}
			}

			obs_.push_back(observation_type(obs_ts, obs_op, obs_rtns*1.0e-9));
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

	private: std::vector<observation_type> do_observations() const
	{
		return obs_;
	}


	private: std::string metrics_file_;
	private: std::ifstream ifs_;
	private: std::ifstream::pos_type fpos_;
//	private: bool new_data_;
	private: std::vector<observation_type> obs_;
}; // response_time_sensor

template <typename TraitsT>
class throughput_sensor: public base_sensor<TraitsT>
{
	private: typedef base_sensor<TraitsT> base_type;
	public: typedef typename base_type::traits_type traits_type;
	public: typedef typename base_type::observation_type observation_type;


	public: throughput_sensor(std::string const& metrics_file_path)
	: metrics_file_(metrics_file_path),
	  fpos_(0)
//	  new_data_(false)
	{
	}

	private: void do_sense()
	{
		DCS_DEBUG_TRACE("BEGIN Do Sense");

		// Available fields in a row (each field is separated by one or more white-spaces):
		// - '[' <generated-during> ']'
		// - <finished-timestamp>
		// - <operation name>
		// - <response time>
		// - '[' <operation request> ']'
		// - <total response time>
		// - <number of observations>

		const std::size_t timestamp_field(2);
		const std::size_t operation_field(3);
		const std::size_t num_observations_field(7);
		std::time_t first_obs_ts = -1; // timestamp (in secs from Epoch) of the first observation
		//const std::size_t max_open_trials(50);

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
			ifs_.open(metrics_file_.c_str(), std::ios_base::ate);
			if (ifs_.good())
			{
				ifs_.sync();
				std::ifstream::pos_type new_fpos(ifs_.tellg());
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

		// Collect all available metrics entries
		while (ifs_.good() && ifs_.is_open())
		{
			fpos_ = ifs_.tellg();

			std::string line;

			std::getline(ifs_, line);

//DCS_DEBUG_TRACE("IFS STREAM -- LINE: " << line << " - POS: " << fpos_ << " - GOOD: " << ifs_.good() << " - EOF: " << ifs_.eof() << " - FAIL: " << ifs_.fail() << " - BAD: " << ifs_.bad() << " - !(): " << !static_cast<bool>(ifs_));

			const std::size_t n = line.size();

			if (!ifs_.good() || n == 0)
			{
				continue;
			}

			std::time_t obs_ts = -1; // timestamp (in secs from Epoch)
			std::string obs_op; // Operation label
			unsigned long num_obs = 0; // number of observations
			std::size_t field = 0;
			for (std::size_t pos = 0; pos < n; ++pos)
			{
				// eat all heading space
				for (; pos < n && std::isspace(line[pos]); ++pos)
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
							std::size_t pos2(pos);
							for (; pos2 < n && std::isdigit(line[pos2]); ++pos2)
							{
								;
							}
							std::istringstream iss(line.substr(pos, pos2-pos));
							iss >> obs_ts;
//DCS_DEBUG_TRACE("Timestamp: " << obs_ts);
							pos = pos2;
							break;
						}
						case operation_field:
						{
							std::size_t pos2(pos);
							//for (; pos2 < n && std::isalpha(line[pos2]); ++pos2)
							for (; pos2 < n && !std::isspace(line[pos2]); ++pos2)
							{
								;
							}
							obs_op = line.substr(pos, pos2-pos);
//DCS_DEBUG_TRACE("Operation: " << obs_op);
							pos = pos2;
							break;
						}
						case num_observations_field:
						{
							std::size_t pos2(pos);
							for (; pos2 < n && std::isdigit(line[pos2]); ++pos2)
							{
								;
							}
							std::istringstream iss(line.substr(pos, pos2-pos));
							iss >> num_obs;
//DCS_DEBUG_TRACE("Number of Observations: " << num_obs);
							pos = pos2;
							break;
						}
						default:
							// skip these fields
							for (; pos < n && !std::isspace(line[pos]); ++pos)
							{
								;
							}
							break;
					}
				}
			}

			// Record this observation only if it is not the first observation
			if (first_obs_ts == -1)
			{
				first_obs_ts = obs_ts;
			}
			else
			{
				obs_.push_back(observation_type(obs_ts, obs_op, num_obs/((obs_ts-first_obs_ts)*1e-3)));
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

	private: std::vector<observation_type> do_observations() const
	{
		return obs_;
	}


	private: std::string metrics_file_;
	private: std::ifstream ifs_;
	private: std::ifstream::pos_type fpos_;
//	private: bool new_data_;
	private: std::vector<observation_type> obs_;
}; // response_time_sensor

}}} // Namespace dcs::testbed::rain

#endif // DCS_TESTBED_RAIN_SENSORS_HPP
