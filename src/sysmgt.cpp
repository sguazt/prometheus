/**
 * \file sysexp.hpp
 *
 * \brief Driver for performing system experiments
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <boost/numeric/ublas/banded.hpp>
#include <boost/smart_ptr.hpp>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <dcs/cli.hpp>
#include <dcs/debug.hpp>
#include <dcs/logging.hpp>
#include <dcs/math/traits/float.hpp>
#include <dcs/testbed/application.hpp>
#include <dcs/testbed/application_managers.hpp>
#include <dcs/testbed/base_application.hpp>
#include <dcs/testbed/system_experiment.hpp>
#include <dcs/testbed/system_identification_strategies.hpp>
//#include <dcs/testbed/system_managers.hpp>
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
//const ::std::string default_out_dat_file("./sysmgt-out.dat");
const double default_sampling_time(1000);
const double default_control_time(3*default_sampling_time);
const double default_ewma_smooth_factor(0.9);
const double default_slo_value(0);

void usage(char const* progname)
{
	::std::cerr << "Usage: " << progname << " [options]" << ::std::endl
				<< " --help" << ::std::endl
				<< "   Show this message." << ::std::endl
//				<< " --out-dat-file <file path>" << ::std::endl
//				<< "   The path to the output data file." << ::std::endl
//				<< "   [default: '" << default_out_dat_file << "']." << ::std::endl
				<< " --slo-value <value>" << ::std::endl
				<< "   The target value for the SLO metric." << ::std::endl
				<< "   [default: '" << default_slo_value << "']." << ::std::endl
				<< " --tc <time in millisecs>" << ::std::endl
				<< "   Control time (in milliseconds)." << ::std::endl
				<< "   [default: " << default_control_time << "]." << ::std::endl
				<< " --ts <time in millisecs>" << ::std::endl
				<< "   Sampling time (in milliseconds)." << ::std::endl
				<< "   [default: " << default_sampling_time << "]." << ::std::endl
				<< " --verbose" << ::std::endl
				<< "   Show verbose messages." << ::std::endl
				<< "   [default: disabled]." << ::std::endl
				<< " --vm-uri <URI>" << ::std::endl
				<< "   The VM URI to connect." << ::std::endl
				<< "   Repeat this option as many times as is the number of your VMs." << ::std::endl
				<< " --wkl <name>" << ::std::endl
				<< "   The workload to generate. Possible values are: 'olio', 'rubis'." << ::std::endl
				<< "   [default: '" << ::dcs::testbed::to_string(default_workload) << "']." << ::std::endl
				<< " --wkl-driver <name>" << ::std::endl
				<< "   The workload driver to use. Possible values are: 'rain'." << ::std::endl
				<< "   [default: '" << ::dcs::testbed::to_string(default_workload_driver) << "']." << ::std::endl
				<< " --wkl-driver-rain-path <name>" << ::std::endl
				<< "   The full path to the RAIN workload driver." << ::std::endl
				<< "   [default: '" << default_workload_driver_rain_path << "']." << ::std::endl
				<< ::std::endl;
}

template <typename RealT>
struct rt_slo_checker
{
	rt_slo_checker(RealT max_val, RealT rel_tol=0.05)
	: max_val_(max_val),
	  check_val_(max_val_*(1+rel_tol))
	{
	}

	bool operator()(RealT val)
	{
		return ::dcs::math::float_traits<RealT>::approximately_less_equal(val, check_val_);
	}

	private: RealT max_val_;
	private: RealT check_val_;
};

}} // Namespace detail::<unnamed>


int main(int argc, char *argv[])
{
	namespace testbed = ::dcs::testbed;
	namespace ublas = ::boost::numeric::ublas;

	typedef double real_type;
	typedef unsigned int uint_type;
	typedef testbed::traits<real_type,uint_type> traits_type;

	bool opt_help(false);
//	std::string opt_out_dat_file;
	real_type opt_ts;
	real_type opt_tc;
	bool opt_verbose(false);
	testbed::workload_category opt_wkl;
	testbed::workload_generator_category opt_wkl_driver;
	std::string opt_wkl_driver_rain_path;
	std::vector<std::string> opt_vm_uris;
	real_type opt_slo_value(0);

	// Parse command line options
	try
	{
		opt_help = dcs::cli::simple::get_option(argv, argv+argc, "--help");
//		opt_out_dat_file = dcs::cli::simple::get_option<std::string>(argv, argv+argc, "--out-dat-file", detail::default_out_dat_file);
		opt_tc = dcs::cli::simple::get_option<real_type>(argv, argv+argc, "--tc", detail::default_control_time);
		opt_ts = dcs::cli::simple::get_option<real_type>(argv, argv+argc, "--ts", detail::default_sampling_time);
		opt_verbose = dcs::cli::simple::get_option(argv, argv+argc, "--verbose");
		opt_vm_uris = dcs::cli::simple::get_options<std::string>(argv, argv+argc, "--vm-uri");
		opt_wkl = dcs::cli::simple::get_option<testbed::workload_category>(argv, argv+argc, "--wkl", detail::default_workload);
		opt_wkl_driver = dcs::cli::simple::get_option<testbed::workload_generator_category>(argv, argv+argc, "--wkl-driver", detail::default_workload_driver);
		opt_wkl_driver_rain_path = dcs::cli::simple::get_option<std::string>(argv, argv+argc, "--wkl-driver-rain-path", detail::default_workload_driver_rain_path);
		opt_slo_value = dcs::cli::simple::get_option<real_type>(argv, argv+argc, "--slo-value", detail::default_slo_value);
	}
	catch (std::exception const& e)
	{
		std::ostringstream oss;
		oss << "Error while parsing command-line options: " << e.what();
		dcs::log_error(DCS_LOGGING_AT, oss.str());

		detail::usage(argv[0]);
		return EXIT_FAILURE;
	}

	if (opt_help)
	{
		detail::usage(argv[0]);
		return EXIT_SUCCESS;
	}

	int ret(0);

	if (opt_verbose)
	{
		std::ostringstream oss;

		for (std::size_t i = 0; i < opt_vm_uris.size(); ++i)
		{
			if (i > 0)
			{
				oss << ", ";
			}
			oss << "VM URI: " << opt_vm_uris[i];
		}
		dcs::log_info(DCS_LOGGING_AT, oss.str());
		oss.str("");

//		oss << "Output data file: " << opt_out_dat_file;
//		dcs::log_info(DCS_LOGGING_AT, oss.str());
//		oss.str("");

		oss << "Control time: " << opt_tc;
		dcs::log_info(DCS_LOGGING_AT, oss.str());
		oss.str("");

		oss << "Sampling time: " << opt_ts;
		dcs::log_info(DCS_LOGGING_AT, oss.str());
		oss.str("");

		oss << "SLO value: " << opt_slo_value;
		dcs::log_info(DCS_LOGGING_AT, oss.str());
		oss.str("");

		oss << "Workload: " << opt_wkl;
		dcs::log_info(DCS_LOGGING_AT, oss.str());
		oss.str("");

		oss << "Workload driver: " << opt_wkl_driver;
		dcs::log_info(DCS_LOGGING_AT, oss.str());
		oss.str("");

		oss << "Workload driver RAIN path: " << opt_wkl_driver_rain_path;
		dcs::log_info(DCS_LOGGING_AT, oss.str());
		oss.str("");
	}

	typedef testbed::base_virtual_machine<traits_type> vm_type;
	typedef boost::shared_ptr<vm_type> vm_pointer;
	//typedef vm_type::identifier_type vm_identifier_type;
	typedef testbed::base_virtual_machine_manager<traits_type> vmm_type;
	typedef boost::shared_ptr<vmm_type> vmm_pointer;
	typedef vmm_type::identifier_type vmm_identifier_type;
	typedef testbed::base_application<traits_type> app_type;
	typedef boost::shared_ptr<app_type> app_pointer;
	typedef testbed::base_application_manager<traits_type> app_manager_type;
	typedef boost::shared_ptr<app_manager_type> app_manager_pointer;
	typedef testbed::base_workload_driver<traits_type> app_driver_type;
	typedef boost::shared_ptr<app_driver_type> app_driver_pointer;
	typedef testbed::base_arx_system_identification_strategy<traits_type> sysid_strategy_type;
	typedef boost::shared_ptr<sysid_strategy_type> sysid_strategy_pointer;

	try
	{
		const std::size_t nt(opt_vm_uris.size()); // Number of tiers
		//const real_type rt_q99(0.2870*1.e+3);
		//const real_type rt_mean(0.1034*1.e+3);

		testbed::system_experiment<traits_type> sys_exp;

		// Setup application experiment
		// - Setup application (and VMs)
		std::map<vmm_identifier_type,vmm_pointer> vmm_map;
		std::vector<vm_pointer> vms;
		std::vector<std::string>::const_iterator uri_end_it(opt_vm_uris.end());
		for (std::vector<std::string>::const_iterator it = opt_vm_uris.begin();
			 it != uri_end_it;
			 ++it)
		{
			std::string const& uri(*it);

			vmm_pointer p_vmm;
			if (!vmm_map.count(uri) > 0)
			{
				p_vmm = boost::make_shared< testbed::libvirt::virtual_machine_manager<traits_type> >(uri);
				vmm_map[uri] = p_vmm;
			}
			else
			{
				p_vmm = vmm_map.at(uri);
			}

			vm_pointer p_vm(p_vmm->vm(uri));
			// check: p_vm != null
			DCS_DEBUG_ASSERT( p_vm );
			vms.push_back(p_vm);
		}
		app_pointer p_app = boost::make_shared< testbed::application<traits_type> >(vms.begin(), vms.end());
		p_app->slo(testbed::response_time_application_performance, detail::rt_slo_checker<real_type>(opt_slo_value));

		// - Setup workload driver
		app_driver_pointer p_drv;
		switch (opt_wkl_driver)
		{
			case testbed::rain_workload_generator:
				{
					boost::shared_ptr< testbed::rain::workload_driver<traits_type> > p_drv_impl = boost::make_shared< testbed::rain::workload_driver<traits_type> >(opt_wkl, opt_wkl_driver_rain_path);
					p_app->register_sensor(testbed::response_time_application_performance, p_drv_impl->sensor(testbed::response_time_application_performance));
					//p_drv = boost::make_shared< testbed::rain::workload_driver<traits_type> >(drv_impl);
					p_drv = p_drv_impl;
				}
				break;
		}
		p_drv->app(p_app);

		// - Setup application manager
		app_manager_pointer p_mgr;
		//p_mgr = boost::make_shared< testbed::lqry_application_manager<traits_type> >();
		{
#if defined(DCS_TESTBED_USE_LQRY_APP_MGR)
			const std::size_t na(2);
			const std::size_t nb(2);
			const std::size_t nk(1);
			const std::size_t ny(1);
			const std::size_t nu(nt);
			const real_type ff(0.98);
			const real_type rho(1);

			sysid_strategy_pointer p_sysid_alg = boost::make_shared< testbed::rls_ff_arx_miso_proxy<traits_type> >(na, nb, nk, ny, nu, ff);
			ublas::matrix<real_type> Q = ublas::identity_matrix<real_type>(ny);
#if defined(DCS_TESTBED_EXP_LQ_APP_MGR_USE_ALT_SS) && DCS_TESTBED_EXP_LQ_APP_MGR_USE_ALT_SS == 'Y'
			ublas::matrix<real_type> R = rho*ublas::identity_matrix<real_type>(nu,nu);
#else // DCS_TESTBED_EXP_LQ_APP_MGR_USE_ALT_SS
# error TODO: Set matrix R with proper size
#endif // DCS_TESTBED_EXP_LQ_APP_MGR_USE_ALT_SS
			testbed::lqry_application_manager<traits_type> lqry_mgr(Q, R);
			lqry_mgr.sysid_strategy(p_sysid_alg);
			//lqry_mgr.target_value(testbed::response_time_application_performance, rt_q99*(1-0.20));
//			lqry_mgr.target_value(testbed::response_time_application_performance, opt_slo_value);

			p_mgr = boost::make_shared< testbed::lqry_application_manager<traits_type> >(lqry_mgr);
#elif defined(DCS_TESTBED_USE_PADALA2009_APP_MGR)
			const std::size_t na(2);
			const std::size_t nb(2);
			const std::size_t nk(1);
			const std::size_t ny(1);
			const std::size_t nu(nt);
			const real_type ff(0.98);
			const real_type q(2);

			sysid_strategy_pointer p_sysid_alg = boost::make_shared< testbed::rls_ff_arx_miso_proxy<traits_type> >(na, nb, nk, ny, nu, ff);
			testbed::padala2009_application_manager<traits_type> padala2009_mgr;
			padala2009_mgr.sysid_strategy(p_sysid_alg);
			//padala2009_mgr.target_value(testbed::response_time_application_performance, rt_q99*(1-0.20));
//			padala2009_mgr.target_value(testbed::response_time_application_performance, opt_slo_value);
			padala2009_mgr.stability_factor(q);

			p_mgr = boost::make_shared< testbed::padala2009_application_manager<traits_type> >(padala2009_mgr);
#else
# error Application Manager not recognized
#endif
			p_mgr->target_value(testbed::response_time_application_performance, opt_slo_value);
			p_mgr->sampling_time(opt_ts);
			p_mgr->control_time(opt_tc);
		}
		p_mgr->app(p_app);

		// Add to main experiment
		sys_exp.add_app(p_app, p_drv, p_mgr);


		//sys_exp.logger(...);
		//sys_exp.output_data_file(out_dat_file);

		sys_exp.run();
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
