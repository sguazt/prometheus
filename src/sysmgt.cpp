/**
 * \file rubis_mgmt.hpp
 *
 * \brief Driver for managing a RUBiS instance.
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

#include <boost/smart_ptr.hpp>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <dcs/cli.hpp>
#include <dcs/logging.hpp>
#include <dcs/testbed/registry.hpp>
#include <dcs/testbed/system_management.hpp>
#include <dcs/testbed/system_managers.hpp>
#include <dcs/testbed/traits.hpp>
#include <dcs/testbed/virtual_machine_managers.hpp>
#include <dcs/testbed/virtual_machines.hpp>
#include <dcs/testbed/workload_category.hpp>
#include <dcs/testbed/workload_drivers.hpp>
#include <dcs/testbed/workload_generator_category.hpp>
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

const dcs::testbed::workload_category default_workload(dcs::testbed::olio_workload);
const dcs::testbed::workload_generator_category default_workload_driver(dcs::testbed::rain_workload_generator);
const ::std::string default_workload_driver_rain_path("/usr/local/opt/rain-workload-toolkit");
const ::std::string default_out_dat_file("./sysmgt-out.dat");
const double default_sampling_time(10);
const double default_ewma_smooth_factor(0.9);

void usage(char const* progname)
{
	::std::cerr << "Usage: " << progname << " [options]" << ::std::endl
				<< " --help" << ::std::endl
				<< "   Show this message." << ::std::endl
				<< " --out-dat-file <file path>" << ::std::endl
				<< "   The path to the output data file." << ::std::endl
				<< "   [default: '" << default_out_dat_file << "']." << ::std::endl
				<< " --ts <time in secs>" << ::std::endl
				<< "   Sampling time (in seconds)." << ::std::endl
				<< "   [default: " << default_sampling_time << "]." << ::std::endl
				<< " --verbose" << ::std::endl
				<< "   Show verbose messages." << ::std::endl
				<< "   [default: disabled]." << ::std::endl
				<< " --vm-uri <URI>" << ::std::endl
				<< "   The VM URI to connect." << ::std::endl
				<< "   Repeat this option as many times as is the number of your VMs."
				<< " --wkl <name>" << ::std::endl
				<< "   The workload to generate. Possible values are: 'olio', 'rubis'." << ::std::endl
				<< "   [default: '" << default_workload << "']." << ::std::endl
				<< " --wkl-driver <name>" << ::std::endl
				<< "   The workload driver to use. Possible values are: 'rain'." << ::std::endl
				<< "   [default: '" << default_workload_driver << "']." << ::std::endl
				<< " --wkl-driver-rain-path <name>" << ::std::endl
				<< "   The full path to the RAIN workload driver." << ::std::endl
				<< "   [default: '" << default_workload_driver_rain_path << "']." << ::std::endl
				<< ::std::endl;
}

}} // Namespace detail::<unnamed>


int main(int argc, char *argv[])
{
	namespace testbed = ::dcs::testbed;

	typedef double real_type;
	typedef unsigned int uint_type;
	typedef testbed::traits<real_type,uint_type> traits_type;

	bool help(false);
	std::string out_dat_file;
	real_type ts;
	bool verbose(false);
	testbed::workload_category wkl;
	testbed::workload_generator_category wkl_driver;
	std::string wkl_driver_rain_path;
	std::vector<std::string> vm_uris;

	// Parse command line options
	try
	{
		help = dcs::cli::simple::get_option(argv, argv+argc, "--help");
		out_dat_file = dcs::cli::simple::get_option<std::string>(argv, argv+argc, "--out-dat-file", detail::default_out_dat_file);
		ts = dcs::cli::simple::get_option<real_type>(argv, argv+argc, "--ts", detail::default_sampling_time);
		verbose = dcs::cli::simple::get_option(argv, argv+argc, "--verbose");
		vm_uris = dcs::cli::simple::get_options<std::string>(argv, argv+argc, "--vm-uri");
		wkl = dcs::cli::simple::get_option<testbed::workload_category>(argv, argv+argc, "--wkl", detail::default_workload);
		wkl_driver = dcs::cli::simple::get_option<testbed::workload_generator_category>(argv, argv+argc, "--wkl-driver", detail::default_workload_driver);
		wkl_driver_rain_path = dcs::cli::simple::get_option<std::string>(argv, argv+argc, "--wkl-driver-rain-path", detail::default_workload_driver_rain_path);
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

		for (std::size_t i = 0; i < vm_uris.size(); ++i)
		{
			if (i > 0)
			{
				oss << ", ";
			}
			oss << "VM URI: " << vm_uris[i];
		}
		dcs::log_info(DCS_LOGGING_AT, oss.str());
		oss.str("");

		oss << "Output data file: " << out_dat_file;
		dcs::log_info(DCS_LOGGING_AT, oss.str());
		oss.str("");

		oss << "Sampling time: " << ts;
		dcs::log_info(DCS_LOGGING_AT, oss.str());
		oss.str("");

		oss << "Workload: " << wkl;
		dcs::log_info(DCS_LOGGING_AT, oss.str());
		oss.str("");

		oss << "Workload driver: " << wkl_driver;
		dcs::log_info(DCS_LOGGING_AT, oss.str());
		oss.str("");

		oss << "Workload driver RAIN path: " << wkl_driver_rain_path;
		dcs::log_info(DCS_LOGGING_AT, oss.str());
		oss.str("");
	}

	typedef testbed::base_virtual_machine<traits_type> vm_type;
	typedef boost::shared_ptr<vm_type> vm_pointer;
	typedef vm_type::identifier_type vm_identifier_type;
	typedef testbed::base_virtual_machine_manager<traits_type> vmm_type;
	typedef boost::shared_ptr<vmm_type> vmm_pointer;
	typedef vmm_type::identifier_type vmm_identifier_type;

	try
	{
		const std::size_t nt(vm_uris.size()); // Number of tiers

		testbed::registry<traits_type>& reg = testbed::registry<traits_type>::instance();

		std::vector<vm_pointer> vms(nt);

		std::vector<std::string>::const_iterator uri_end_it(vm_uris.end());
		for (std::vector<std::string>::const_iterator it = vm_uris.begin();
			 it != uri_end_it;
			 ++it)
		{
			std::string const& uri(*it);

			vmm_pointer p_vmm;
			if (!reg.exists_vmm(uri))
			{
				p_vmm = boost::make_shared< testbed::libvirt::virtual_machine_manager<traits_type> >(uri);
				reg.add_vmm(p_vmm);
			}
			else
			{
				p_vmm = reg.vmm(uri);
			}

			vm_pointer p_vm(p_vmm->vm(uri));
			vms.push_back(p_vm);
		}

		boost::shared_ptr< testbed::base_workload_driver<traits_type> > p_driver;

		switch (wkl_driver)
		{
			case testbed::rain_workload_generator:
				p_driver = ::boost::make_shared< testbed::rain_workload_driver<traits_type> >(wkl, wkl_driver_rain_path);
				break;
		}

		boost::shared_ptr< testbed::base_system_manager<traits_type> > p_mgr;

		//TODO: set system manager

		testbed::system_management<traits_type> sysmgt(vms.begin(), vms.end(), p_driver, p_mgr);
		sysmgt.output_data_file(out_dat_file);
		sysmgt.sampling_time(ts);

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
