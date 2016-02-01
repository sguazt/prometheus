#include <boost/cstdint.hpp>
#include <cassert>
#include <cerrno>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <dcs/testbed/libvirt/detail/utility.hpp>
#include "dom_stats.hpp"
#include <iomanip>
#include <iostream>
#include <libvirt/libvirt.h>
#include <libvirt/virterror.h>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unistd.h>


namespace testbed = dcs::testbed;


#if 0 /* DEPRECATED */

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


namespace utility {

::std::string last_error(virConnectPtr conn)
{
	assert( conn );

	::std::string err_str;
	virErrorPtr err = new virError();

	int ret = virConnCopyLastError(conn, err);
	switch (ret)
	{
		case 0:
			// No error found
			break;
		case -1:
			err_str = "Parameter error when attempting to get last error";
			break;
		default:
			err_str = err->message;
			break;
	}

	virResetError(err);
	delete err;

	return err_str;
}

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


virConnectPtr connect(::std::string const& uri)
{
	// Connect to libvirtd daemons
	//
	// virConnectOpenAuth is called here with all default parameters,
	// except, possibly, the URI of the hypervisor.
	//
	virConnectPtr conn = virConnectOpenAuth((!uri.empty() ? uri.c_str() : 0), virConnectAuthPtrDefault, 0);
	if (0 == conn)
	{
		::std::ostringstream oss;
		oss << "No connection to hypervisor with URI '" << uri << "': " << last_error(0);
		throw ::std::runtime_error(oss.str());
	}

	return conn;
}

virDomainPtr connect_domain(virConnectPtr conn, ::std::string const& name)
{
	assert( conn );

	virDomainPtr dom = virDomainLookupByName(conn, name.c_str());
	if (0 == dom)
	{
		::std::ostringstream oss;
		oss << "Failed to get Domain for \"" << name << "\": " << last_error(conn);
		throw ::std::runtime_error(oss.str());
	}

	return dom;
}

std::string domain_hostname(virConnectPtr conn, virDomainPtr dom)
{
	const char* hn = virDomainGetHostname(dom, 0);
	if (0 == hn)
	{
		::std::ostringstream oss;
		oss << "Failed to get hostname for domain \"" << virDomainGetName(dom) << "\": " << last_error(conn);
		throw ::std::runtime_error(oss.str());
	}

	return hn;
}

} // Namespace utility

class dom_stats
{
	public: dom_stats(virConnectPtr conn, virDomainPtr dom)
	: conn_(conn),
	  dom_(dom),
	  first_(true),
	  pct_cpu_(0)
	{
		try
		{
			hname_ = utility::domain_hostname(conn_, dom_);
		}
		catch (...)
		{
			hname_ = "<NA>";
		}
	}

	public: int num_vcpus(int flags)
	{
		assert( conn_ );
		assert( dom_ );

		int ret = 0;

		//if (checkState &&
		//  ((flags & VIR_DOMAIN_AFFECT_LIVE && virDomainIsActive(dom) < 1) ||
		//   (flags & VIR_DOMAIN_AFFECT_CONFIG && virDomainIsPersistent(dom) < 1)))
		//  return -1;

		// In all cases, try the new API first; if it fails because we are talking
		// to an older daemon, generally we try a fallback API before giving up.
		// The flag VIR_DOMAIN_AFFECT_CURRENT requires the new API, since we don't
		// know whether the domain is running or inactive.
		ret = virDomainGetVcpusFlags(dom_, flags);
		if (ret < 0)
		{
			if (flags & VIR_DOMAIN_VCPU_GUEST)
			{
				::std::ostringstream oss;
				oss << "Failed to retrieve vCPU count from \"" << virDomainGetName(dom_) << "\": " << utility::last_error(conn_);
				throw ::std::runtime_error(oss.str());
			}

			if (!(flags & (VIR_DOMAIN_AFFECT_LIVE | VIR_DOMAIN_AFFECT_CONFIG)) && virDomainIsActive(dom_) == 1)
			{
				 flags |= VIR_DOMAIN_AFFECT_LIVE;
			}

			if (flags & VIR_DOMAIN_AFFECT_LIVE)
			{
				if (flags & VIR_DOMAIN_VCPU_MAXIMUM)
				{
					ret = virDomainGetMaxVcpus(dom_);
				}
				else
				{
					virDomainInfo info;
					ret = virDomainGetInfo(dom_, &info);
					if (ret < 0)
					{
						::std::ostringstream oss;
						oss << "Failed to query information for domain \"" << virDomainGetName(dom_) << "\": " << utility::last_error(conn_);
						throw ::std::runtime_error(oss.str());
					}

					ret = info.nrVirtCpu;
				}
			}
			else
			{
/*TODO
				char *def = NULL;
				xmlDocPtr xml = NULL;
				xmlXPathContextPtr ctxt = NULL;

				def = virDomainGetXMLDesc(dom_, VIR_DOMAIN_XML_INACTIVE);
				if (def == 0)
				{
					::std::ostringstream oss;
					oss << "Failed to query XML for domain \"" << virDomainGetName(dom_) << "\": " << utility::last_error(conn_);
					throw ::std::runtime_error(oss.str());
				}

				xml = virXMLParseStringCtxt(def, _("(domain_definition)"), &ctxt);
				if (xml == 0)
				{
					::std::ostringstream oss;
					oss << "Failed to parse XML for domain \"" << virDomainGetName(dom_) << "\": " << utility::last_error(conn_);
					throw ::std::runtime_error(oss.str());
				}

				if (flags & VIR_DOMAIN_VCPU_MAXIMUM)
				{
					if (virXPathInt("string(/domain/vcpus)", ctxt, &count) < 0)
					{
						::std::ostringstream oss;
						oss << "Failed to query retrieve maximum vcpu count for domain \"" << virDomainGetName(dom_) << "\": " << utility::last_error(conn_);
						throw ::std::runtime_error(oss.str());
					}
				}
				else
				{
					if (virXPathInt("string(/domain/vcpus/@current)", ctxt, &count) < 0)
					{
						::std::ostringstream oss;
						oss << "Failed to query retrieve current vcpu count for domain \"" << virDomainGetName(dom_) << "\": " << utility::last_error(conn_);
						throw ::std::runtime_error(oss.str());
					}
				}
*/
				::std::ostringstream oss;
				oss << "Failed to query retrieve vCPU count for domain \"" << virDomainGetName(dom_) << "\": " << utility::last_error(conn_);
				throw ::std::runtime_error(oss.str());
			}
		}

		return ret;
	}

	public: int num_cpus(int flags)
	{
		assert( conn_ );
		assert( dom_ );

		int ret = 0;

		virDomainInfo info;
		ret = virDomainGetInfo(dom_, &info);
		if (0 > ret)
		{
			::std::ostringstream oss;
			oss << "Failed to query information for domain \"" << virDomainGetName(dom_) << "\": " << utility::last_error(conn_);
			throw ::std::runtime_error(oss.str());
		}

		const int nvcpus = info.nrVirtCpu;
		const int maxncpus = this->max_num_cpus();

		const ::std::size_t cpumaplen = VIR_CPU_MAPLEN(maxncpus);
		unsigned char* cpumaps = new unsigned char[nvcpus*cpumaplen];

		ret = virDomainGetVcpuPinInfo(dom_, info.nrVirtCpu, cpumaps, cpumaplen, flags);
		if (0 > ret)
		{
			ret = virDomainGetVcpusFlags(dom_, flags | VIR_DOMAIN_VCPU_MAXIMUM);

			if (0 > ret)
			{
				::std::ostringstream oss;
				oss << "Failed to query the number of vCPUs for domain \"" << virDomainGetName(dom_) << "\": " << utility::last_error(conn_);
				throw ::std::runtime_error(oss.str());
			}
		}

		int ncpu = 0;
		for (int cpu = 0; cpu < maxncpus; ++cpu)
		{
			for (int vcpu = 0; vcpu < cpumaplen; ++vcpu)
			{
				//if (VIR_CPU_USED(VIR_GET_CPUMAP(cpumaps, cpumaplen, vcpu), cpu))
				if (VIR_CPU_USABLE(cpumaps, cpumaplen, vcpu, cpu))
				{
					++ncpu;
					break;
				}
			}
		}

		delete[] cpumaps;

		return ncpu;
	}

	public: void collect()
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

			throw ::std::runtime_error(oss.str());
		}

		// Get the CPU time used (in ns)
		ret = ::virDomainGetInfo(dom_, &cur_node_info_);
		if (-1 == ret)
		{
			::std::ostringstream oss;
			oss << "Failed to get domain info: " << utility::last_error(conn_);

			throw ::std::runtime_error(oss.str());
		}

		if (first_)
		{
			first_ = false;
			
			prev_time_ = cur_time_;
			prev_node_info_ = cur_node_info_;
			pct_cpu_ = 0;
			pct_ram_ = 0;
		}
		else
		{
			// Update CPU utilization stats
			::boost::uint64_t ns_elapsed = (cur_time_.tv_sec-prev_time_.tv_sec)*1.0e9+(cur_time_.tv_nsec-prev_time_.tv_nsec);
			::boost::uint64_t ns_used = cur_node_info_.cpuTime-prev_node_info_.cpuTime;
//	::std::cerr << "DEBUG> %CPU: " << (ns_used/ns_elapsed) << ::std::endl;
			pct_cpu_ = 100.0*static_cast<double>(ns_used/static_cast<long double>(ns_elapsed));

			// Update RAM utilization stats
//	::std::cerr << "DEBUG> %RAM: " << (cur_node_info_.memory/cur_node_info_.maxMem) << ::std::endl;
			const unsigned long cfg_max_mem = this->config_max_memory();
			const unsigned long cur_max_mem = this->max_memory();
			const unsigned long cur_mem = this->current_memory();
			bool ok = true;
			virDomainMemoryStatStruct stats[VIR_DOMAIN_MEMORY_STAT_NR];
			int nr_stats = virDomainMemoryStats(dom_, stats, VIR_DOMAIN_MEMORY_STAT_NR, VIR_DOMAIN_AFFECT_CURRENT);
			if (0 <= nr_stats)
			{
				long double mem_avail = 0;
				for (int i = 0; i < nr_stats; ++i)
				{
					if (stats[i].tag == VIR_DOMAIN_MEMORY_STAT_SWAP_IN)
					{
						std::cerr << "DEBUG> MEM swap_in " << stats[i].val << std::endl;
					}
					if (stats[i].tag == VIR_DOMAIN_MEMORY_STAT_SWAP_OUT)
					{
						std::cerr << "DEBUG> MEM swap_out " << stats[i].val << std::endl;
					}
					if (stats[i].tag == VIR_DOMAIN_MEMORY_STAT_MAJOR_FAULT)
					{
						std::cerr << "DEBUG> MEM major_fault " << stats[i].val << std::endl;
					}
					if (stats[i].tag == VIR_DOMAIN_MEMORY_STAT_MINOR_FAULT)
					{
						std::cerr << "DEBUG> MEM minor_fault " << stats[i].val << std::endl;
					}
					if (stats[i].tag == VIR_DOMAIN_MEMORY_STAT_UNUSED)
					{
						std::cerr << "DEBUG> MEM unused " << stats[i].val << std::endl;
					}
					if (stats[i].tag == VIR_DOMAIN_MEMORY_STAT_AVAILABLE)
					{
						std::cerr << "DEBUG> MEM available " << stats[i].val << std::endl;
						mem_avail = stats[i].val;
					}
					if (stats[i].tag == VIR_DOMAIN_MEMORY_STAT_ACTUAL_BALLOON)
					{
						std::cerr << "DEBUG> MEM actual " << stats[i].val << std::endl;
					}
					if (stats[i].tag == VIR_DOMAIN_MEMORY_STAT_RSS)
					{
						std::cerr << "DEBUG> MEM rss " << stats[i].val << std::endl;
					}

					pct_ram_ = 100.0*static_cast<double>((cur_max_mem-mem_avail)/static_cast<long double>(cfg_max_mem));
				}
			}
			else
			{
				std::cerr << "DEBUG> CONFIG MAX MEM: " << cfg_max_mem << std::endl;
				std::cerr << "DEBUG> CURRENT MAX MEM: " << cur_max_mem << std::endl;
				std::cerr << "DEBUG> CURRENT MEM: " << cur_mem << std::endl;

#ifdef DCS_TESTBED_SENSOR_HAVE_MEMINFO_SERVER
				try
				{
					namespace asio = boost::asio;

					std::string server_addr = this->domain_name();
					std::string server_port = DCS_TESTBED_SENSOR_MEMINFO_SERVER_PORT;

					std::cerr << "Connecting to " << server_addr << " on port " << server_port << std::endl;

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

						boost::tie(sz, meminfo) = utility::meminfo_unpack(oss.str().c_str());

						std::cerr << "DEBUG> MEMINFO: " << meminfo << std::endl;

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
									std::cerr << "DEBUG> COMMITTED: " << mem_committed << " => Adjust Available Memory: "  << mem_avail << " -> " << (mem_tot-mem_committed) << std::endl;
									mem_avail = mem_tot-mem_committed;
								}
							}

							pct_ram_ = 100.0*static_cast<double>((mem_tot-mem_avail)/static_cast<long double>(cfg_max_mem));
							//std::cerr << "DEBUG> Mem Tot: " << mem_tot << ", Mem Avail: " << mem_avail << ", Cfg Max Mem: " << cfg_max_mem << " -> MEM%: " << pct_ram_ << std::endl;
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
				//::std::ostringstream oss;
				//oss << "Failed to get domain memory stats: " << utility::last_error(conn_);
				//throw ::std::runtime_error(oss.str());

				std::clog << "[warning] Failed to get precise domain memory stats: " << utility::last_error(conn_) << std::endl;

				//pct_ram_ = 100.0*static_cast<double>(cur_node_info_.memory/static_cast<long double>(cur_node_info_.maxMem));
				pct_ram_ = 100.0*static_cast<double>(cur_mem/static_cast<long double>(cfg_max_mem));
			}
		}
	}

	public: double percent_cpu() const
	{
		return pct_cpu_;
	}

	public: double percent_ram() const
	{
		return pct_ram_;
	}

	public: std::string hostname() const
	{
		return hname_;
	}

	public: std::string domain_name() const
	{
		char const* ret = 0;

		ret = virDomainGetName(dom_);
		if (0 == ret)
		{
			std::ostringstream oss;
			oss << "Failed to query the name for domain: " << utility::last_error(conn_);
			throw std::runtime_error(oss.str());
		}

		return ret;
	}

	public: unsigned long config_max_memory() const
	{
		unsigned long mem = ::virDomainGetMaxMemory(dom_);
		if (0 == mem)
		{
			std::ostringstream oss;
			oss << "Failed to query the config max memory for domain \"" << ::virDomainGetName(dom_) << "\": " << utility::last_error(conn_);
			throw std::runtime_error(oss.str());
		}

		return mem;
	}

	public: unsigned long max_memory() const
	{
		::virDomainInfo node_info;
		int ret = ::virDomainGetInfo(dom_, &node_info);
		if (0 > ret)
		{
			std::ostringstream oss;
			oss << "Failed to query the max memory for domain \"" << ::virDomainGetName(dom_) << "\": " << utility::last_error(conn_);
			throw std::runtime_error(oss.str());
		}

		return node_info.maxMem;
	}

	public: unsigned long current_memory() const
	{
		::virDomainInfo node_info;
		int ret = ::virDomainGetInfo(dom_, &node_info);
		if (0 > ret)
		{
			std::ostringstream oss;
			oss << "Failed to query the current memory for domain \"" << ::virDomainGetName(dom_) << "\": " << utility::last_error(conn_);
			throw std::runtime_error(oss.str());
		}

		return node_info.memory;
	}

	private: int max_num_cpus()
	{
		assert( conn_ );

		int ret;

		// First try to use virNodeGetCPUMap since it is the lightest way
		ret = virNodeGetCPUMap(conn_, 0, 0, 0);
		if (-1 == ret)
		{
			// Fall back to virNodeGetInfo
			virNodeInfo info;

			ret = virNodeGetInfo(conn_, &info);
			if (-1 == ret)
			{
				::std::ostringstream oss;
				oss << "Failed to get node info: " << utility::last_error(conn_);

				throw ::std::runtime_error(oss.str());
			}

			ret = VIR_NODEINFO_MAXCPUS(info);
		}

		return ret;
	}


	private: ::virConnectPtr conn_;
	private: ::virDomainPtr dom_;
	private: bool first_;
	private: ::timespec prev_time_;
	private: ::virDomainInfo prev_node_info_;
	private: ::timespec cur_time_;
	private: ::virDomainInfo cur_node_info_;
	private: double pct_cpu_;
	private: double pct_ram_;
	private: std::string hname_; ///< The domain host name
}; // dom_stats

#endif // DEPRECATED


int main(int argc, char* argv[])
{
	std::string uri = "xen:///";
	std::string dom_name = "rubis-c63_64";

	if (argc > 1)
	{
		uri = argv[1];
		if (argc > 2)
		{
			dom_name = argv[2];
		}
	}

	try
	{
		virConnectPtr conn = testbed::libvirt::detail::connect(uri);

		virDomainPtr dom = testbed::libvirt::detail::connect_domain(conn, dom_name);

		dom_stats stats(conn, dom);

		std::cout << "DOMAIN: " << dom_name << " (hostname: " << testbed::libvirt::detail::domain_hostname(conn, dom) << ")" << std::endl;

		std::cout << "#vCPUs: " << testbed::libvirt::detail::num_vcpus(conn, dom, VIR_DOMAIN_AFFECT_CURRENT) << std::endl;
		std::cout << "#CPUs: " << testbed::libvirt::detail::num_cpus(conn, dom, VIR_DOMAIN_AFFECT_CURRENT) << std::endl;

		for (std::size_t i = 0; /*i < n*/true; ++i)
		{
			::sleep(1);
			stats.collect();
			std::cout << "#" << i << " STATS:" << std::endl;
			std::cout << "  Time: " << std::time(0) << std::endl;
			std::cout << "  %vCPU: " << std::fixed << stats.percent_cpu() << "%" << std::endl;
			std::cout << "  %RAM: " << std::fixed << stats.percent_ram() << "%" << std::endl;
		}
	}
	catch (std::exception const& e)
	{
		std::cerr << "Caught error: " << e.what() << std::endl;
	}
	catch (...)
	{
		std::cerr << "Unknown error." << std::endl;
	}
}
