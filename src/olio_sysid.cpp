/**
 * \file olio_sysid.hpp
 *
 * \brief Driver for performing system identification against an Apache Olio
 *  instance.
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
 * along with dcsxx-testbed.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <boost/numeric/ublas/vector.hpp>
#include <boost/smart_ptr.hpp>
#include <cstddef>
#include <cstring>
#include <dcs/logging.hpp>
#include <dcs/testbed/signal_generators.hpp>
#include <dcs/testbed/system_identification.hpp>
#include <dcs/testbed/virtual_machines.hpp>
#include <dcs/testbed/workload_drivers.hpp>
#include <iostream>
#include <string>
#include <stdexcept>
#include <vector>


namespace /*<unnamed>*/ {

void usage(char const* progname)
{
	std::cerr << "Usage: " << progname << " [options]" << std::endl
			  << " --db-uri <URI>" << std::endl
			  << "   The URI used to connect to the libvirtd server where the OlioDB VM is running." << std::endl
			  << "   [default: default URI of this machine]." << std::endl
			  << " --db-name <name>" << std::endl
			  << "   The name of the domain running the OlioDB VM." << std::endl
			  << "   [default: OlioDB]." << std::endl
			  << " --help" << std::endl
			  << "   Show this message." << std::endl
			  << " --verbose" << std::endl
			  << "   Show verbose messages." << std::endl
			  << " --web-uri <URI>" << std::endl
			  << "   The URI used to connect to the libvirtd server where the OlioWeb VM is running." << std::endl
			  << "   [default: default URI of this machine]." << std::endl
			  << " --web-name <name>" << std::endl
			  << "   The name of the domain running the OlioWeb VM." << std::endl
			  << "   [default: OlioWeb]." << std::endl
			  << " --wkl-driver-path <name>" << std::endl
			  << "   The full path to the workload driver for Olio." << std::endl
			  << "   [default: /usr/local/rain-workload-toolkit]." << std::endl
			  << std::endl;
}

} // Namespace <unnamed>


int main(int argc, char *argv[])
{
	typedef double real_type;

//	::std::cout << "Attempting to connect to OlioDB and OlioWeb hypervisors" << ::std::endl;

	bool verbose(false);
	::std::string oliodb_uri;
	::std::string olioweb_uri;
	::std::string oliodb_name;
	::std::string olioweb_name;
	::std::string wkl_driver_path;

	// Parse command line options
	bool ok(true);
	for (int arg = 1; arg < argc; ++arg)
	{
		if (!std::strcmp("--db-uri", argv[arg]))
		{
			++arg;
			if (arg < argc)
			{
				oliodb_uri = argv[arg];
			}
			else
			{
				ok = false;
				break;
			}
		}
		else if (!std::strcmp("--db-name", argv[arg]))
		{
			++arg;
			if (arg < argc)
			{
				oliodb_name = argv[arg];
			}
			else
			{
				ok = false;
				break;
			}
		}
		else if (!std::strcmp("--help", argv[arg]))
		{
			usage(argv[0]);
			return 0;
		}
		else if (!std::strcmp("--verbose", argv[arg]))
		{
			verbose = true;
		}
		else if (!std::strcmp("--web-uri", argv[arg]))
		{
			++arg;
			if (arg < argc)
			{
				olioweb_uri = argv[arg];
			}
			else
			{
				ok = false;
				break;
			}
		}
		else if (!std::strcmp("--web-name", argv[arg]))
		{
			++arg;
			if (arg < argc)
			{
				olioweb_name = argv[arg];
			}
			else
			{
				ok = false;
				break;
			}
		}
		else if (!std::strcmp("--wkl-driver-path", argv[arg]))
		{
			++arg;
			if (arg < argc)
			{
				wkl_driver_path = argv[arg];
			}
			else
			{
				ok = false;
				break;
			}
		}
	}
	if (!ok)
	{
		usage(argv[0]);
		return 1;
	}
	if (oliodb_name.empty())
	{
		oliodb_name = "OlioDB";
	}
	if (olioweb_name.empty())
	{
		olioweb_name = "OlioWeb";
	}
	if (wkl_driver_path.empty())
	{
		wkl_driver_path = "/usr/local/rain-workload-toolkit";
	}

	int ret(0);

	if (verbose)
	{
		std::ostringstream oss;

		oss << "OlioDB URI: " << oliodb_uri;
		dcs::log_info(DCS_LOGGING_AT, oss.str());
		oss.str("");

		oss << "OlioDB VM name: " << oliodb_name;
		dcs::log_info(DCS_LOGGING_AT, oss.str());
		oss.str("");

		oss << "OlioWeb URI: " << olioweb_uri;
		dcs::log_info(DCS_LOGGING_AT, oss.str());
		oss.str("");

		oss << "OlioWeb VM name: " << olioweb_name;
		dcs::log_info(DCS_LOGGING_AT, oss.str());
		oss.str("");

		oss << "Workload driver path: " << wkl_driver_path;
		dcs::log_info(DCS_LOGGING_AT, oss.str());
		oss.str("");
	}

	namespace testbed = ::dcs::testbed;
	namespace ublas = ::boost::numeric::ublas;

	typedef boost::shared_ptr< testbed::base_virtual_machine<real_type> > vm_pointer;

	try
	{
		vm_pointer p_oliodb_vm(new testbed::libvirt_virtual_machine<real_type>(oliodb_uri, oliodb_name));
		vm_pointer p_olioweb_vm(new testbed::libvirt_virtual_machine<real_type>(olioweb_uri, olioweb_name));

		const std::size_t nu(2);

		std::vector<vm_pointer> vms(nu);
		vms[0] = p_olioweb_vm;
		vms[1] = p_oliodb_vm;

		ublas::scalar_vector<real_type> ampl(nu, 0.5);
		ublas::scalar_vector<real_type> freq(nu, 8);
		ublas::scalar_vector<real_type> phase(nu, 0);
		ublas::scalar_vector<real_type> bias(nu, 0.5);

		boost::shared_ptr< testbed::base_workload_driver > p_driver(new testbed::rain_workload_driver(testbed::rain_workload_driver::olio_workload, wkl_driver_path));

		boost::shared_ptr< testbed::base_signal_generator<real_type> > p_sig_gen(new testbed::sinusoidal_mesh_signal_generator<real_type>(ampl, freq, phase, bias));

		testbed::system_identification<real_type> sysid(vms.begin(), vms.end(), p_driver, p_sig_gen);

		sysid.run();
	}
	catch (std::exception const& e)
	{
		ret = 1;
		dcs::log_error(DCS_LOGGING_AT, e.what());
	}
	catch (...)
	{
		ret = 1;
		dcs::log_error(DCS_LOGGING_AT, "Unknown error");
	}

	return ret;
}
