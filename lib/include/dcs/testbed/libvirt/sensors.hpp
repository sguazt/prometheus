/**
 * \file dcs/testbed/libvirt/sensors.hpp
 *
 * \brief A set of sensor classes for libvirt-based Virtual Machines.
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
#include <dcs/logging.hpp>
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


#ifdef DCS_TESTBED_SENSOR_HAVE_MEMINFO_SERVER
# include <boost/array.hpp>
# include <boost/asio.hpp>
# include <boost/cstdint.hpp>
# include <boost/system/error_code.hpp>
# include <boost/system/system_error.hpp>
# include <boost/tuple/tuple.hpp>
//# include <jsoncpp/json/value.h>
//# include <jsoncpp/json/reader.h>
# include <json/value.h>
# include <json/reader.h>
#endif // DCS_TESTBED_SENSOR_HAVE_MEMINFO_SERVER


namespace dcs { namespace testbed { namespace libvirt {

#ifdef DCS_TESTBED_SENSOR_HAVE_MEMINFO_SERVER
namespace detail { namespace /*<unnamed>*/ {

inline
boost::tuple<boost::uint32_t, std::string> meminfo_unpack(char const* input)
{
	boost::tuple<boost::uint32_t, std::string> result;

	boost::uint32_t sz = ntohl(*reinterpret_cast<boost::uint32_t const*>(input));
	boost::get<0>(result) = sz;
	input += 1 * sizeof(boost::uint32_t);

	boost::get<1>(result) = std::string(reinterpret_cast<char const*>(input), sz);

	return result;
}

}} // Namespace detail::<unnamed>
#endif // DCS_TESTBED_SENSOR_HAVE_MEMINFO_SERVER

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
DCS_DEBUG_TRACE("nsec-used: " << ns_used << " - nsec-elaps: " << ns_elapsed << " - #vCPUs: " << nvcpus << " --> UTIL: " << cpu_util_);//XXX
			}
			else
			{
DCS_DEBUG_TRACE("nsec-used: " << ns_used << " - nsec-elaps: " << ns_elapsed << " --> UTIL: " << cpu_util_);//XXX
			}
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


template <typename TraitsT>
class memory_utilization_sensor: public base_sensor<TraitsT>
{
	private: typedef base_sensor<TraitsT> base_type;
	public: typedef typename base_type::traits_type traits_type;
	public: typedef typename base_type::observation_type observation_type;
	private: typedef typename traits_type::real_type real_type;


	public: memory_utilization_sensor(::virConnectPtr p_conn, ::virDomainPtr p_dom, bool use_meminfo_server = false)
	: p_conn_(p_conn),
	  p_dom_(p_dom),
	  use_meminfo_srv_(use_meminfo_server),
	  mem_util_(0),
	  first_(true)
	{
	}

	private: void do_sense()
	{
		int ret;

		// Get the max RAM assignable
		const unsigned long cfg_max_mem = detail::config_max_memory(p_conn_, p_dom_);
		const unsigned long cur_max_mem = detail::max_memory(p_conn_, p_dom_);
		unsigned long dom_mem_tot = 0;

//XXX: moved below
//		// Get the RAM used
//		::virDomainInfo node_info;
//		ret = ::virDomainGetInfo(p_dom_, &node_info);
//		if (-1 == ret)
//		{
//			::std::ostringstream oss;
//			oss << "Failed to get domain info: " << detail::last_error(p_conn_);
//
//			DCS_EXCEPTION_THROW(::std::runtime_error, oss.str());
//		}

		bool ok = true;

		//NOTE: not all domains support virDomainMemoryStats. So be prepared to get a failure
		virDomainMemoryStatStruct stats[VIR_DOMAIN_MEMORY_STAT_NR];
		int nr_stats = virDomainMemoryStats(p_dom_, stats, VIR_DOMAIN_MEMORY_STAT_NR, VIR_DOMAIN_AFFECT_CURRENT);
		if (0 <= nr_stats)
		{
			// Currently libvirt offers these stats:
			// - ..._SWAP_IN: The total amount of data read from swap space (in kB).
			// - ..._SWAP_OUT: The total amount of memory written out to swap space (in kB).
			// - ..._MAJOR_FAULT: Number of page faults that have occured when a process makes a valid access to virtual memory that is not available, for which disk I/O is required.
			// - ..._MINOR_FAULT: Number of page faults that have occurred when a process makes a valid access to virtual memory that is not available, for which disk I/O is'nt required.
			// - ..._UNUSED: The amount of memory left completely unused by the system (in kB). Memory that is available but used for reclaimable caches should NOT be reported as free.
			// - ..._AVAILABLE: The total amount of usable memory as seen by the domain (in kB). This value may be less than the amount of memory assigned to the domain if a balloon driver is in use or if the guest OS does not initialize all assigned pages.
			// - ..._ACTUAL_BALLOON: Current balloon value (in KB).
			// - ..._RSS: Resident Set Size of the process running the domain (in kB).

			long double mem_avail = 0;
			for (int i = 0; i < nr_stats; ++i)
			{
				if (stats[i].tag == VIR_DOMAIN_MEMORY_STAT_AVAILABLE)
				{
					mem_avail = stats[i].val;
				}
			}

			////NOTE: Currently, it seems that 'maxMem' field gives a too high value.
			////      So use the 'memory' field which should give a reasonable value.
			////mem_util_ = 1.0-static_cast<double>(mem_avail/static_cast<long double>(node_info.maxMem));
			//mem_util_ = 1.0-static_cast<double>(mem_avail/static_cast<long double>(node_info.memory));
			mem_util_ = static_cast<double>((cur_max_mem-mem_avail)/static_cast<long double>(cfg_max_mem));
			dom_mem_tot = cur_max_mem; //FIXME: what is the best value to use?
		}
		else
		{
#ifdef DCS_TESTBED_SENSOR_HAVE_MEMINFO_SERVER
			if (use_meminfo_srv_)
			{
				try
				{
					namespace asio = boost::asio;

					//std::string server_addr = DCS_TESTBED_SENSOR_MEMINFO_SERVER_ADDRESS;
					//std::string server_port = DCS_TESTBED_SENSOR_MEMINFO_SERVER_PORT;
					std::string server_addr = detail::domain_name(p_conn_, p_dom_);
					std::string server_port = DCS_TESTBED_SENSOR_MEMINFO_SERVER_PORT;

					DCS_DEBUG_TRACE("Connecting to " << server_addr << " on port " << server_port);

					asio::io_service io_service;

					asio::ip::tcp::resolver resolver(io_service);
					asio::ip::tcp::resolver::query query(server_addr, server_port);
					asio::ip::tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);

					asio::ip::tcp::socket socket(io_service);
					boost::asio::connect(socket, endpoint_iterator);

					std::ostringstream oss;
					for (;;)
					{
						boost::array<char, 1024> buf;
						boost::system::error_code error;

						const std::size_t len = socket.read_some(boost::asio::buffer(buf), error);
						if (error == boost::asio::error::eof)
						{
							break; // Connection closed cleanly by peer.
						}
						else if (error)
						{
							throw boost::system::system_error(error); // Some other error.
						}

						oss << std::string(buf.data(), len);
					}
					socket.close();
					
					if (!oss.str().empty())
					{
						boost::uint32_t sz;
						std::string meminfo;
						
						boost::tie(sz, meminfo) = detail::meminfo_unpack(oss.str().c_str());

						DCS_DEBUG_TRACE("CONFIG MAX MEM: " << cfg_max_mem);
						DCS_DEBUG_TRACE("CURRENT MAX MEM: " << cur_max_mem);
						DCS_DEBUG_TRACE("CURRENT MEM: " << detail::current_memory(p_conn_, p_dom_));
						DCS_DEBUG_TRACE("MEMINFO: " << meminfo);

						Json::Value root;   // will contains the root value after parsing
						Json::Reader reader;
						bool parse_ok = reader.parse(meminfo, root);
						if (parse_ok)
						{
							// From linux kernel 3.x:
							//  Many load balancing and workload placing programs check /proc/meminfo to
							//  estimate how much free memory is available.
							//  They generally do this by adding up "free" and "cached", which was fine ten
							//  years ago, but is pretty much guaranteed to be wrong today.
							//  It is wrong because Cached includes memory that is not freeable as page
							//  cache, for example shared memory segments, tmpfs, and ramfs, and it does not
							//  include reclaimable slab memory, which can take up a large fraction of
							//  system memory on mostly idle systems with lots of files.
							//  Currently, the amount of memory that is available for a new workload,
							//  without pushing the system into swap, can be estimated from MemFree,
							//  Active(file), Inactive(file), and SReclaimable, as well as the "low"
							//  watermarks from /proc/zoneinfo.
							//  See also '<linux-kernel-source>/fs/proc/meminfo.c'.

							long double mem_avail = 0;
							if (root.isMember("MemAvailable"))
							{
								//long double mem_tot = 0;
								std::istringstream iss;
								//iss.str(root.get("MemTotal", "").asString());
								//iss >> mem_tot;
								iss.str(root.get("MemAvailable", "").asString());
								iss >> mem_avail;
							}
							else if (root.isMember("MemFree"))
							{
								// Use the old (and unreliable) method

								long double mem_free = 0;
								std::istringstream iss;
								iss.str(root.get("MemFree", "").asString());
								iss >> mem_free;
								long double mem_cache = 0;
								iss.str(root.get("Cached", "0").asString());
								iss >> mem_cache;

								mem_avail = mem_free+mem_cache;
							}

							long double mem_tot = 0;
							if (root.isMember("MemTotal"))
							{
								std::istringstream iss;
								iss.str(root.get("MemTotal", "").asString());
								iss >> mem_tot;
							}
							else
							{
								//mem_tot = static_cast<long double>(cur_max_mem);
								mem_tot = detail::current_memory(p_conn_, p_dom_);
							}

							if (root.isMember("Committed_AS"))
							{
								long double mem_committed = 0;
								std::istringstream iss;
								iss.str(root.get("Committed_AS", "").asString());
								iss >> mem_committed;

								if (mem_avail > (mem_tot-mem_committed))
								{
									DCS_DEBUG_TRACE("COMMITTED: " << mem_committed << " => Adjust Available Memory: "  << mem_avail << " -> " << (mem_tot-mem_committed));
									mem_avail = mem_tot-mem_committed;
								}
							}

							//NOTE: Currently, it seems that 'maxMem' field gives a too high value.
							//      So use the 'memory' field which should give a reasonable value.
							//////mem_util_ = static_cast<double>(mem_avail/static_cast<long double>(node_info.maxMem));
							//////mem_util_ = 1.0-static_cast<double>(mem_avail/static_cast<long double>(node_info.memory));
							////mem_util_ = 1.0-static_cast<double>(mem_avail/mem_tot);
							//mem_util_ = static_cast<double>((cur_max_mem-mem_avail)/static_cast<long double>(cfg_max_mem));
#if 1
							// Use internal+external information (needs the scale factor, see below at the end of the function)
							mem_util_ = static_cast<double>((mem_tot-mem_avail)/static_cast<long double>(cfg_max_mem));
#else
							// Only use internal information (no need to use the scale factor, see below at the end of the function)
							mem_util_ = 1.0 - mem_avail/static_cast<long double>(mem_tot);
#endif
							dom_mem_tot = mem_tot;
						}
						else
						{
							DCS_EXCEPTION_THROW(std::runtime_error, "Unexpected format for JSON file");
						}
					}
					else
					{
						ok = false;
					}
				}
				catch (std::exception const& e)
				{
					dcs::log_warn(DCS_LOGGING_AT, std::string("Failed to get precise domain memory stats: ") + e.what());
					ok = false;
				}
			}
			else
			{
				dcs::log_warn(DCS_LOGGING_AT, std::string("Unsupported precise domain memory stats: ") + detail::last_error(p_conn_));

				//// The most supported solution in libvirt is to use the following information:
				//// - memory: the memory used by the domain (in kB)
				//// - maxMem: the maximum memory allowed by the domain (in kB)
				//// However, note that 'memory' is a static value (i.e., the currently allocated memory) that doesn't reflect the amount of memory effectively used by the domain
				//mem_util_ = static_cast<double>(node_info.memory/static_cast<long double>(node_info.maxMem));

				ok = false; // see below
			}
#else // DCS_TESTBED_SENSOR_HAVE_MEMINFO_SERVER
			dcs::log_warn(DCS_LOGGING_AT, std::string("Unsupported precise domain memory stats: ") + detail::last_error(p_conn_));

			//// The most supported solution in libvirt is to use the following information:
			//// - memory: the memory used by the domain (in kB)
			//// - maxMem: the maximum memory allowed by the domain (in kB)
			//// However, note that 'memory' is a static value (i.e., the currently allocated memory) that doesn't reflect the amount of memory effectively used by the domain
			//mem_util_ = static_cast<double>(node_info.memory/static_cast<long double>(node_info.maxMem));

			ok = false; // see below
#endif // DCS_TESTBED_SENSOR_HAVE_MEMINFO_SERVER
		}

		if (!ok)
		{
			// Get the RAM used
			::virDomainInfo node_info;
			ret = ::virDomainGetInfo(p_dom_, &node_info);
			if (-1 == ret)
			{
				::std::ostringstream oss;
				oss << "Failed to get domain info: " << detail::last_error(p_conn_);

				DCS_EXCEPTION_THROW(::std::runtime_error, oss.str());
			}

			// The most supported solution in libvirt is to use the following information:
			// - memory: the memory used by the domain (in kB)
			// - maxMem: the maximum memory allowed by the domain (in kB)
			// However, note that 'memory' is a static value (i.e., the currently allocated memory) that doesn't reflect the amount of memory effectively used by the domain
			//mem_util_ = static_cast<double>(node_info.memory/static_cast<long double>(node_info.maxMem));
			mem_util_ = static_cast<double>(node_info.memory/static_cast<long double>(cfg_max_mem));
			dom_mem_tot = cur_max_mem; //FIXME: what is the best value to use?
		}

#if 1
		// The just computed utilization have to adjusted in order to take
		// into account of the overhead introduced by the hypervisor.
		// Indeed, the memory set outside the VM is usually different from
		// the one seen inside the VM.
		// To cope with this issue we compute an adjustment factor in this
		// way:
		//    scale_factor = <CurrentMaxMemory>/<MemTotal>
		//    Util_{inside} = mem_util_
		//    Util_{outside} = adj_factor * Util_{inside}
		const double scale_factor = static_cast<double>(cur_max_mem/static_cast<long double>(dom_mem_tot));
		DCS_DEBUG_TRACE("SCALE FACTOR for UTILIZATION: " << scale_factor);
		if (scale_factor > 1)
		{
			mem_util_ *= scale_factor;
		}
#endif

		if (first_)
		{
			first_ = false;
		}
	}

	private: void do_reset()
	{
		mem_util_ = 0;
		first_ = true;
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
			observation_type ob(0, "", mem_util_);
			obs.push_back(ob);
		}

		return obs;
	}


	private: ::virConnectPtr p_conn_;
	private: ::virDomainPtr p_dom_;
	private: bool use_meminfo_srv_;
	private: real_type mem_util_;
	private: bool first_;
	//private: ::virDomainInfo prev_node_info_;
	//private: ::virDomainInfo cur_node_info_;
}; // memory_utilization_sensor

}}} // Namespace dcs::testbed::libvirt

#endif // DCS_TESTBED_LIBVIRT_SENSORS_HPP
