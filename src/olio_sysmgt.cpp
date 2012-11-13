/**
 * \file olio_mgmt.hpp
 *
 * \brief Driver for managing an Apache Olio instance.
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

//#include <boost/random.hpp>
#include <boost/smart_ptr.hpp>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <dcs/cli.hpp>
#include <dcs/logging.hpp>
#include <dcs/testbed/system_management.hpp>
//#include <dcs/testbed/virtual_machines.hpp>
#include <dcs/testbed/workload_drivers.hpp>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <stdexcept>
#include <vector>


namespace detail { namespace /*<unnamed>*/ {

//enum aggregation_category
//{
//	mean_aggregation
//};

//const ::std::string default_oliodb_name("OlioDB");
//const ::std::string default_oliodb_uri("");
//const ::std::string default_olioweb_name("OlioWeb");
//const ::std::string default_olioweb_uri("");
const ::std::string default_workload_driver_path("/usr/local/rain-workload-toolkit");
const ::std::string default_out_dat_file("./olio-sysid-out.dat");
const double default_sampling_time(10);
const double default_ewma_smooth_factor(0.9);

void usage(char const* progname)
{
	::std::cerr << "Usage: " << progname << " [options]" << ::std::endl
//				<< " --db-uri <URI>" << ::std::endl
//				<< "   The URI used to connect to the libvirtd server where the OlioDB VM is running." << ::std::endl
//				<< "   [default: default URI of this machine]." << ::std::endl
//				<< " --db-name <name>" << ::std::endl
//				<< "   The name of the domain running the OlioDB VM." << ::std::endl
//				<< "   [default: OlioDB]." << ::std::endl
				<< " --help" << ::std::endl
				<< "   Show this message." << ::std::endl
				<< " --out-dat-file <file path>" << ::std::endl
				<< "   The path to the output data file." << ::std::endl
				<< "   [default: ./olio-sysid-out.dat]." << ::std::endl
				<< " --ts <time in secs>" << ::std::endl
				<< "   Sampling time (in seconds)." << ::std::endl
				<< "   [default: 10]." << ::std::endl
				<< " --verbose" << ::std::endl
				<< "   Show verbose messages." << ::std::endl
				<< "   [default: disabled]." << ::std::endl
//				<< " --web-uri <URI>" << ::std::endl
//				<< "   The URI used to connect to the libvirtd server where the OlioWeb VM is running." << ::std::endl
//				<< "   [default: default URI of this machine]." << ::std::endl
//				<< " --web-name <name>" << ::std::endl
//				<< "   The name of the domain running the OlioWeb VM." << ::std::endl
//				<< "   [default: OlioWeb]." << ::std::endl
				<< " --wkl-driver-path <name>" << ::std::endl
				<< "   The full path to the workload driver for Olio." << ::std::endl
				<< "   [default: /usr/local/rain-workload-toolkit]." << ::std::endl
				<< ::std::endl;
}

}} // Namespace detail::<unnamed>


int main(int argc, char *argv[])
{
	typedef double real_type;
	typedef unsigned int uint_type;

	bool help(false);
	std::string oliodb_uri;
	std::string olioweb_uri;
	std::string oliodb_name;
	std::string olioweb_name;
	std::string out_dat_file;
	real_type ewma_smooth_factor;
	real_type ts;
	bool verbose(false);
	std::string wkl_driver_path;

	// Parse command line options
	try
	{
//		oliodb_uri = dcs::cli::simple::get_option<std::string>(argv, argv+argc, "--db-uri", detail::default_oliodb_uri);
//		oliodb_name = dcs::cli::simple::get_option<std::string>(argv, argv+argc, "--db-name", detail::default_oliodb_name);
		help = dcs::cli::simple::get_option(argv, argv+argc, "--help");
		out_dat_file = dcs::cli::simple::get_option<std::string>(argv, argv+argc, "--out-dat-file", detail::default_out_dat_file);
		ewma_smooth_factor = dcs::cli::simple::get_option<real_type>(argv, argv+argc, "--aggr-ewma-factor", detail::default_ewma_smooth_factor);
		ts = dcs::cli::simple::get_option<real_type>(argv, argv+argc, "--ts", detail::default_sampling_time);
		verbose = dcs::cli::simple::get_option(argv, argv+argc, "--verbose");
//		olioweb_uri = dcs::cli::simple::get_option<std::string>(argv, argv+argc, "--web-uri", detail::default_olioweb_uri);
//		olioweb_name = dcs::cli::simple::get_option<std::string>(argv, argv+argc, "--web-name", detail::default_olioweb_name);
		wkl_driver_path = dcs::cli::simple::get_option<std::string>(argv, argv+argc, "--wkl-driver-path", detail::default_workload_driver_path);
	}
	catch (std::exception const& e)
	{
		std::ostringstream oss;
		oss << "Error while parsing command-line options: " << e.what();
		dcs::log_error(DCS_LOGGING_AT, oss.str());

		detail::usage(argv[0]);
		std::abort();
		return EXIT_FAILURE;
	}

	if (help)
	{
		detail::usage(argv[0]);
		return EXIT_SUCCESS;
	}

	int ret(0);

	if (verbose)
	{
		std::ostringstream oss;

//		oss << "OlioDB URI: " << oliodb_uri;
//		dcs::log_info(DCS_LOGGING_AT, oss.str());
//		oss.str("");

//		oss << "OlioDB VM name: " << oliodb_name;
//		dcs::log_info(DCS_LOGGING_AT, oss.str());
//		oss.str("");

//		oss << "OlioWeb URI: " << olioweb_uri;
//		dcs::log_info(DCS_LOGGING_AT, oss.str());
//		oss.str("");

//		oss << "OlioWeb VM name: " << olioweb_name;
//		dcs::log_info(DCS_LOGGING_AT, oss.str());
//		oss.str("");

		oss << "Output data file: " << out_dat_file;
		dcs::log_info(DCS_LOGGING_AT, oss.str());
		oss.str("");

		oss << "EWMA smoothing factor: " << ewma_smooth_factor;
		dcs::log_info(DCS_LOGGING_AT, oss.str());
		oss.str("");

		oss << "Sampling time: " << ts;
		dcs::log_info(DCS_LOGGING_AT, oss.str());
		oss.str("");

		oss << "Workload driver path: " << wkl_driver_path;
		dcs::log_info(DCS_LOGGING_AT, oss.str());
		oss.str("");
	}

	namespace testbed = ::dcs::testbed;

//	typedef boost::shared_ptr< testbed::base_virtual_machine<real_type> > vm_pointer;

	try
	{
//		vm_pointer p_oliodb_vm(new testbed::libvirt_virtual_machine<real_type>(oliodb_uri, oliodb_name));
//		vm_pointer p_olioweb_vm(new testbed::libvirt_virtual_machine<real_type>(olioweb_uri, olioweb_name));

//		const std::size_t nt(2); // Number of tiers

//		std::vector<vm_pointer> vms(nt);
//		vms[0] = p_olioweb_vm;
//		vms[1] = p_oliodb_vm;

		boost::shared_ptr< testbed::base_workload_driver > p_driver(new testbed::rain_workload_driver(testbed::rain_workload_driver::olio_workload, wkl_driver_path));

		testbed::system_management<real_type> sysmgt(p_driver);
		sysmgt.output_data_file(out_dat_file);
		sysmgt.sampling_time(ts);
		sysmgt.ewma_smoothing_factor(ewma_smooth_factor);

		sysmgt.run();
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
