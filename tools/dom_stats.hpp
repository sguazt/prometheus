#ifndef DOM_STATS_HPP
#define DOM_STATS_HPP


#include <boost/cstdint.hpp>
#include <cassert>
#include <cstring>
#include <ctime>
#include <dcs/testbed/libvirt/detail/utility.hpp>
#include <libvirt/libvirt.h>
#include <sstream>
#include <stdexcept>
#include <string>

#if DCS_TESTBED_SENSOR_HAVE_MEMINFO_SERVER
# include <boost/array.hpp>
# include <boost/asio.hpp>
# include <boost/cstdint.hpp>
# include <boost/system/error_code.hpp>
# include <boost/system/system_error.hpp>
# include <boost/tuple/tuple.hpp>
# include <jsoncpp/json/value.h>
# include <jsoncpp/json/reader.h>
#endif // DCS_TESTBED_SENSOR_HAVE_MEMINFO_SERVER


namespace detail {

#if DCS_TESTBED_SENSOR_HAVE_MEMINFO_SERVER

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

#endif // DCS_TESTBED_SENSOR_HAVE_MEMINFO_SERVER

} // Namespace detail


class dom_stats
{
public:
	dom_stats(::virConnectPtr conn, ::virDomainPtr dom)
	: conn_(conn),
	  dom_(dom),
	  first_(true),
	  cpu_util_(0),
	  mem_util_(0)
	{
	}

	void collect()
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
			std::ostringstream oss;
			oss << "Failed to get clock time info: " << std::strerror(errno);

			throw std::runtime_error(oss.str());
		}

		// Get the CPU time used (in ns)
		ret = ::virDomainGetInfo(dom_, &cur_node_info_);
		if (-1 == ret)
		{
			std::ostringstream oss;
			oss << "Failed to get domain info: " << dcs::testbed::libvirt::detail::last_error(conn_);

			throw std::runtime_error(oss.str());
		}

		if (first_)
		{
			first_ = false;
			
			prev_time_ = cur_time_;
			prev_node_info_ = cur_node_info_;
			cpu_util_ = 0;
			mem_util_ = 0;
		}
		else
		{
			bool ok = true;

			// Update CPU utilization stats
			const boost::uint64_t ns_elapsed = (cur_time_.tv_sec-prev_time_.tv_sec)*1.0e9+(cur_time_.tv_nsec-prev_time_.tv_nsec);
			const boost::uint64_t ns_used = cur_node_info_.cpuTime-prev_node_info_.cpuTime;
			const int nvcpus = dcs::testbed::libvirt::detail::num_vcpus(conn_, dom_, VIR_DOMAIN_VCPU_MAXIMUM);
			cpu_util_ = static_cast<double>(ns_used/static_cast<long double>(ns_elapsed));
			cpu_util_ /= static_cast<double>(nvcpus);

			// Update RAM utilization stats
			const unsigned long cfg_max_mem = dcs::testbed::libvirt::detail::config_max_memory(conn_, dom_);
			const unsigned long cur_max_mem = dcs::testbed::libvirt::detail::max_memory(conn_, dom_);
			const unsigned long cur_mem = dcs::testbed::libvirt::detail::current_memory(conn_, dom_);
			::virDomainMemoryStatStruct stats[VIR_DOMAIN_MEMORY_STAT_NR];
			int nr_stats = ::virDomainMemoryStats(dom_, stats, VIR_DOMAIN_MEMORY_STAT_NR, VIR_DOMAIN_AFFECT_CURRENT);
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
						//std::cerr << "DEBUG> MEM available " << stats[i].val << std::endl;
						mem_avail = stats[i].val;
					}

					mem_util_ = static_cast<double>((cur_max_mem-mem_avail)/static_cast<long double>(cfg_max_mem));
				}
			}
			else
			{
				//std::cerr << "DEBUG> CONFIG MAX MEM: " << cfg_max_mem << std::endl;
				//std::cerr << "DEBUG> CURRENT MAX MEM: " << cur_max_mem << std::endl;
				//std::cerr << "DEBUG> CURRENT MEM: " << cur_mem << std::endl;

#ifdef DCS_TESTBED_SENSOR_HAVE_MEMINFO_SERVER
				try
				{
					namespace asio = boost::asio;

					std::string server_addr = dcs::testbed::libvirt::detail::domain_name(conn_, dom_);
					std::string server_port = DCS_TESTBED_SENSOR_MEMINFO_SERVER_PORT;

					//std::cerr << "Connecting to " << server_addr << " on port " << server_port << std::endl;

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

						//std::cerr << "DEBUG> MEMINFO: " << meminfo << std::endl;

						Json::Value root;   // will contains the root value after parsing
						Json::Reader reader;
						bool parse_ok = reader.parse(meminfo, root);
						if (parse_ok)
						{
							long double mem_avail = 0;
							if (root.isMember("MemAvailable"))
							{
								std::istringstream iss;
								iss.str(root.get("MemAvailable", "").asString());
								iss >> mem_avail;
							}
							else if (root.isMember("MemFree"))
							{
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
								mem_tot = cur_mem;
							}

							if (root.isMember("Committed_AS"))
							{
								long double mem_committed = 0;
								std::istringstream iss;
								iss.str(root.get("Committed_AS", "").asString());
								iss >> mem_committed;

								if (mem_avail > (mem_tot-mem_committed))
								{
									//std::cerr << "DEBUG> COMMITTED: " << mem_committed << " => Adjust Available Memory: "  << mem_avail << " -> " << (mem_tot-mem_committed) << std::endl;
									mem_avail = mem_tot-mem_committed;
								}
							}

							mem_util_ = static_cast<double>((mem_tot-mem_avail)/static_cast<long double>(cfg_max_mem));
							//std::cerr << "DEBUG> Mem Tot: " << mem_tot << ", Mem Avail: " << mem_avail << ", Cfg Max Mem: " << cfg_max_mem << " -> MEM%: " << mem_util_ << std::endl;
						}
						else
						{
							throw std::runtime_error("Unexpected format for JSON file");
						}
					}
					else
					{
						ok = false;
					}
				}
				catch (std::exception const& e)
				{
					ok = false;
				}
#else // DCS_TESTBED_SENSOR_HAVE_MEMINFO_SERVER
				ok = false
#endif // DCS_TESTBED_SENSOR_HAVE_MEMINFO_SERVER
			}

			if (!ok)
			{
				//std::ostringstream oss;
				//oss << "Failed to get domain memory stats: " << utility::last_error(conn_);
				//throw std::runtime_error(oss.str());

				std::clog << "[warning] Failed to get precise domain memory stats: " << dcs::testbed::libvirt::detail::last_error(conn_) << std::endl;

				//mem_util_ = 100.0*static_cast<double>(cur_node_info_.memory/static_cast<long double>(cur_node_info_.maxMem));
				mem_util_ = static_cast<double>(cur_mem/static_cast<long double>(cfg_max_mem));
			}
		}
	}

	public: double cpu_util() const
	{
		return cpu_util_;
	}

	public: double percent_cpu() const
	{
		return 100.0*cpu_util_;
	}

	public: double memory_util() const
	{
		return mem_util_;
	}

	double percent_ram() const
	{
		return 100.0*mem_util_;
	}

private:
	::virConnectPtr conn_;
	::virDomainPtr dom_;
	bool first_;
	::timespec prev_time_;
	::virDomainInfo prev_node_info_;
	::timespec cur_time_;
	::virDomainInfo cur_node_info_;
	double cpu_util_;
	double mem_util_;
}; // dom_stats

#endif // DOM_STATS_HPP
