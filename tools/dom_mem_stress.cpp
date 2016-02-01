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


namespace testbed = dcs::testbed;


int main(int argc, char* argv[])
{
	const unsigned long quiet_counter_max_value = 10;

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
        ::virConnectPtr conn = testbed::libvirt::detail::connect(uri);

        ::virDomainPtr dom = testbed::libvirt::detail::connect_domain(conn, dom_name);

		dom_stats stats(conn, dom);

		const double share_incr = 0.1;
		float share = 1;
		unsigned long old_cur_memory = 0;
		unsigned long quiet_counter = quiet_counter_max_value;
		float mult = -1;

		std::cout << "\"#\",\"time\",\"CPU Cap\",\"CPU Share\",\"CPU Util\",\"Mem Target Share\",\"Mem Cap\",\"Mem Share\",\"Mem Util\",\"Mem Max Config\",\"Mem Max Current\",\"Mem Current\"" << std::endl;
		for (std::size_t i = 0; /*i < n*/true; ++i)
		{
			::sleep(1);

			if (i > 0 && old_cur_memory == testbed::libvirt::detail::current_memory(conn, dom))
			{
				if (quiet_counter == 0)
				{
					if (share <= share_incr && mult < 0)
					{
						// Go up
						mult = 1;
					}
					else if (share >= 1 && mult > 0)
					{
						// Go down
						mult = -1;
					}

					std::cerr << "DEBUG> Old share " << share << " - mult: " << mult << std::endl;
					share += mult*share_incr;
					std::cerr << "DEBUG> Setting share " << share << std::endl;

					testbed::libvirt::detail::memory_share(conn, dom, share);
					quiet_counter = quiet_counter_max_value;

					if (share <= stats.memory_util())
					{
						std::cerr << "[warning] The memory share " << share << " is less than or equal to memory utilization " << stats.memory_util() << std::endl;
					}
				}
				else
				{
					--quiet_counter;
				}
			}

			stats.collect();

			std::cout	<< i
						<< "," << std::time(0)
						<< "," << testbed::libvirt::detail::cpu_cap(conn, dom)
						<< "," << testbed::libvirt::detail::cpu_share(conn, dom)
						<< "," << stats.cpu_util()
						<< "," << share
						<< "," << testbed::libvirt::detail::memory_cap(conn, dom)
						<< "," << testbed::libvirt::detail::memory_share(conn, dom)
						<< "," << stats.memory_util()
						<< "," << testbed::libvirt::detail::config_max_memory(conn, dom)
						<< "," << testbed::libvirt::detail::max_memory(conn, dom)
						<< "," << testbed::libvirt::detail::current_memory(conn, dom)
						<< std::endl;

			old_cur_memory = testbed::libvirt::detail::current_memory(conn, dom);
		}

		testbed::libvirt::detail::memory_share(conn, dom, 1.0);
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
