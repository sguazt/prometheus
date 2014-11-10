#include <boost/cstdint.hpp>
#include <cassert>
#include <cerrno>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <libvirt/libvirt.h>
#include <libvirt/virterror.h>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unistd.h>

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
			::std::ostringstream oss;
			oss << "Failed to query the number of vCPUs for domain \"" << virDomainGetName(dom_) << "\": " << utility::last_error(conn_);
			throw ::std::runtime_error(oss.str());
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
			::boost::uint64_t ns_elapsed = (cur_time_.tv_sec-prev_time_.tv_sec)*1.0e9+(cur_time_.tv_nsec-prev_time_.tv_nsec);
			::boost::uint64_t ns_used = cur_node_info_.cpuTime-prev_node_info_.cpuTime;
//	::std::cerr << "DEBUG> %CPU: " << (ns_used/ns_elapsed) << ::std::endl;
			pct_cpu_ = static_cast<double>(ns_used/static_cast<long double>(ns_elapsed))*100.0;
//	::std::cerr << "DEBUG> %RAM: " << (cur_node_info_.memory/cur_node_info_.maxMem) << ::std::endl;
			virDomainMemoryStatStruct stats[VIR_DOMAIN_MEMORY_STAT_NR];
			int nr_stats = virDomainMemoryStats(dom_, stats, VIR_DOMAIN_MEMORY_STAT_NR, VIR_DOMAIN_AFFECT_CURRENT);
			if (0 <= nr_stats)
			{
				long double mem_avail_MB = 0;
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
						mem_avail_MB = stats[i].val/1024.0;
					}
					if (stats[i].tag == VIR_DOMAIN_MEMORY_STAT_ACTUAL_BALLOON)
					{
						std::cerr << "DEBUG> MEM actual " << stats[i].val << std::endl;
					}
					if (stats[i].tag == VIR_DOMAIN_MEMORY_STAT_RSS)
					{
						std::cerr << "DEBUG> MEM rss " << stats[i].val << std::endl;
					}

					pct_ram_ = static_cast<double>(mem_avail_MB/cur_node_info_.maxMem)*100.0;
				}
			}
			else
			{
				//::std::ostringstream oss;
				//oss << "Failed to get domain memory stats: " << utility::last_error(conn_);
				//throw ::std::runtime_error(oss.str());

				std::clog << "[warning] Failed to get precise domain memory stats: " << utility::last_error(conn_) << std::endl;

				pct_ram_ = static_cast<double>(cur_node_info_.memory/static_cast<long double>(cur_node_info_.maxMem))*100.0;
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
};

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
		virConnectPtr conn = utility::connect(uri);

		virDomainPtr dom = utility::connect_domain(conn, dom_name);

		std::string hostname = utility::domain_hostname(conn, dom);
		std::cout << "DOMAIN: " << dom_name << " (hostname: " << hostname << ")" << std::endl;

		dom_stats stats(conn, dom);

		std::cout << "#CPUs: " << stats.num_cpus(VIR_DOMAIN_AFFECT_CURRENT) << std::endl;

		for (std::size_t i = 0; /*i < n*/true; ++i)
		{
			::sleep(5);
			stats.collect();
			std::cout << "#" << i << " STATS:" << std::endl;
			std::cout << "  %CPU: " << std::fixed << stats.percent_cpu() << "%" << std::endl;
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
