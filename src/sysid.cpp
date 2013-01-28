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

#include <boost/random.hpp>
#include <boost/smart_ptr.hpp>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <dcs/cli.hpp>
#include <dcs/logging.hpp>
#include <dcs/testbed/application.hpp>
#include <dcs/testbed/base_application.hpp>
#include <dcs/testbed/signal_generators.hpp>
#include <dcs/testbed/system_identification.hpp>
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

enum signal_category
{
	constant_signal,
	half_sinusoidal_mesh_signal,
	half_sinusoidal_signal,
	gaussian_signal,
	sawtooth_signal,
	sinusoidal_mesh_signal,
	sinusoidal_signal,
	square_signal,
	uniform_signal
};

enum aggregation_category
{
	mean_aggregation
};

const ::std::string default_oliodb_name("OlioDB");
const ::std::string default_oliodb_uri("");
const ::std::string default_olioweb_name("OlioWeb");
const ::std::string default_olioweb_uri("");
const dcs::testbed::workload_category default_workload(dcs::testbed::olio_workload);
const dcs::testbed::workload_generator_category default_workload_driver(dcs::testbed::rain_workload_generator);
const ::std::string default_workload_driver_rain_path("/usr/local/opt/rain-workload-toolkit");
const ::std::string default_out_dat_file("./sysid-out.dat");
const double default_sampling_time(10);
const signal_category default_signal_category(constant_signal);
const double default_signal_common_upper_bound(std::numeric_limits<double>::infinity());
const double default_signal_common_lower_bound(-std::numeric_limits<double>::infinity());
const double default_signal_const_val(1);
const double default_signal_sawtooth_low(0);
const double default_signal_sawtooth_high(1);
const double default_signal_sawtooth_incr(0.1);
const double default_signal_sine_amplitude(0.5);
const unsigned int default_signal_sine_frequency(8);
const unsigned int default_signal_sine_phase(0);
const double default_signal_sine_bias(0.5);
const double default_signal_sine_mesh_amplitude(0.5);
const unsigned int default_signal_sine_mesh_frequency(8);
const unsigned int default_signal_sine_mesh_phase(0);
const double default_signal_sine_mesh_bias(0.5);
const double default_signal_half_sine_amplitude(0.5);
const unsigned int default_signal_half_sine_frequency(8);
const unsigned int default_signal_half_sine_phase(0);
const double default_signal_half_sine_bias(0.5);
const double default_signal_half_sine_mesh_amplitude(0.5);
const unsigned int default_signal_half_sine_mesh_frequency(8);
const unsigned int default_signal_half_sine_mesh_phase(0);
const double default_signal_half_sine_mesh_bias(0.5);
const double default_signal_square_low(0);
const double default_signal_square_high(1);
const double default_signal_uniform_min(0);
const double default_signal_uniform_max(1);
const double default_signal_gaussian_mean(0);
const double default_signal_gaussian_sd(1);

template <typename CharT, typename CharTraitsT>
::std::basic_istream<CharT,CharTraitsT>& operator>>(::std::basic_istream<CharT,CharTraitsT>& is, signal_category& sig)
{
	::std::string str;

	is >> str;

	if (!str.compare("constant"))
	{
		sig = constant_signal;
	}
	else if (!str.compare("half-sine"))
	{
		sig = half_sinusoidal_signal;
	}
	else if (!str.compare("half-sine-mesh"))
	{
		sig = half_sinusoidal_mesh_signal;
	}
	else if (!str.compare("gaussian"))
	{
		sig = gaussian_signal;
	}
	else if (!str.compare("sawtooth"))
	{
		sig = sawtooth_signal;
	}
	else if (!str.compare("sine"))
	{
		sig = sinusoidal_signal;
	}
	else if (!str.compare("sine-mesh"))
	{
		sig = sinusoidal_mesh_signal;
	}
	else if (!str.compare("square"))
	{
		sig = square_signal;
	}
	else if (!str.compare("uniform"))
	{
		sig = uniform_signal;
	}
	else
	{
		DCS_EXCEPTION_THROW(::std::invalid_argument,
							"Cannot find a valid signal category");
	}

	return is;
}

template <typename CharT, typename CharTraitsT>
::std::basic_ostream<CharT,CharTraitsT>& operator>>(::std::basic_ostream<CharT,CharTraitsT>& os, signal_category sig)
{
	::std::string str;

	os >> str;

	switch (sig)
	{
		case constant_signal:
			os << "constant";
			break;
		case half_sinusoidal_signal:
			os << "half-sine";
			break;
		case half_sinusoidal_mesh_signal:
			os << "half-sine-mesh";
			break;
		case gaussian_signal:
			os << "gaussian";
			break;
		case sawtooth_signal:
			os << "sawtooth";
			break;
		case sinusoidal_signal:
			os << "sine";
			break;
		case sinusoidal_mesh_signal:
			os << "sine-mesh";
			break;
		case square_signal:
			os << "square";
			break;
		case uniform_signal:
			os << "uniform";
		default:
			DCS_EXCEPTION_THROW(::std::invalid_argument,
								"Cannot find a valid signal category");
	}

	return os;
}

void usage(char const* progname)
{
	::std::cerr << "Usage: " << progname << " [options]" << ::std::endl
				<< " --help" << ::std::endl
				<< "   Show this message." << ::std::endl
				<< " --out-dat-file <file path>" << ::std::endl
				<< "   The path to the output data file." << ::std::endl
				<< "   [default: '" << default_out_dat_file << "']" << ::std::endl
				<< " --sig <signal category>" << ::std::endl
				<< "   The type of signal used to excite the system under test." << ::std::endl
				<< "   Possible values are:" << ::std::endl
				<< "   - constant" << ::std::endl
				<< "   - half-sine" << ::std::endl
				<< "   - half-sine-mesh" << ::std::endl
				<< "   - gaussian" << ::std::endl
				<< "   - sawtooth" << ::std::endl
				<< "   - sine" << ::std::endl
				<< "   - sine-mesh" << ::std::endl
				<< "   - square" << ::std::endl
				<< "   - uniform" << ::std::endl
				<< "   [default: '" << default_signal_category << "']." << ::std::endl
				<< " --ts <time in secs>" << ::std::endl
				<< "   Sampling time (in seconds)." << ::std::endl
				<< "   [default: " << default_sampling_time << "]." << ::std::endl
				<< " --verbose" << ::std::endl
				<< "   Show verbose messages." << ::std::endl
				<< "   [default: disabled]." << ::std::endl
				<< " --vm-uri <URI>" << ::std::endl
				<< "   The URI used to connect to a VM." << ::std::endl
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
	std::vector<std::string> vm_uris;
	std::string out_dat_file;
	uint_type rng_seed(5498);
	detail::signal_category sig;
	real_type sig_common_up_bound;
	real_type sig_common_lo_bound;
	real_type sig_const_val;
	real_type sig_gauss_mean;
	real_type sig_gauss_sd;
	real_type sig_half_sine_ampl;
	uint_type sig_half_sine_freq;
	uint_type sig_half_sine_phase;
	real_type sig_half_sine_bias;
	real_type sig_half_sine_mesh_ampl;
	uint_type sig_half_sine_mesh_freq;
	uint_type sig_half_sine_mesh_phase;
	real_type sig_half_sine_mesh_bias;
	real_type sig_sawtooth_low;
	real_type sig_sawtooth_high;
	real_type sig_sawtooth_incr;
	real_type sig_sine_ampl;
	uint_type sig_sine_freq;
	uint_type sig_sine_phase;
	real_type sig_sine_bias;
	real_type sig_sine_mesh_ampl;
	uint_type sig_sine_mesh_freq;
	uint_type sig_sine_mesh_phase;
	real_type sig_sine_mesh_bias;
	real_type sig_square_low;
	real_type sig_square_high;
	real_type sig_unif_min;
	real_type sig_unif_max;
	real_type ts;
	bool verbose(false);
	testbed::workload_category wkl;
	testbed::workload_generator_category wkl_driver;
	std::string wkl_driver_rain_path;

	// Parse command line options
	try
	{
		help = dcs::cli::simple::get_option(argv, argv+argc, "--help");
		out_dat_file = dcs::cli::simple::get_option<std::string>(argv, argv+argc, "--out-dat-file", detail::default_out_dat_file);
		sig = dcs::cli::simple::get_option<detail::signal_category>(argv, argv+argc, "--sig", detail::default_signal_category);
		sig_common_up_bound = dcs::cli::simple::get_option<real_type>(argv, argv+argc, "--sig-upper-bound", detail::default_signal_common_upper_bound);
		sig_common_lo_bound = dcs::cli::simple::get_option<real_type>(argv, argv+argc, "--sig-lower-bound", detail::default_signal_common_lower_bound);
		sig_const_val = dcs::cli::simple::get_option<real_type>(argv, argv+argc, "--sig-constant-val", detail::default_signal_const_val);
		sig_sawtooth_low = dcs::cli::simple::get_option<real_type>(argv, argv+argc, "--sig-sawtooth-low", detail::default_signal_sawtooth_low);
		sig_sawtooth_high = dcs::cli::simple::get_option<real_type>(argv, argv+argc, "--sig-sawtooth-high", detail::default_signal_sawtooth_high);
		sig_sawtooth_incr = dcs::cli::simple::get_option<real_type>(argv, argv+argc, "--sig-sawtooth-incr", detail::default_signal_sawtooth_incr);
		sig_sine_ampl = dcs::cli::simple::get_option<real_type>(argv, argv+argc, "--sig-sine-ampl", detail::default_signal_sine_amplitude);
		sig_sine_freq = dcs::cli::simple::get_option<uint_type>(argv, argv+argc, "--sig-sine-freq", detail::default_signal_sine_frequency);
		sig_sine_phase = dcs::cli::simple::get_option<uint_type>(argv, argv+argc, "--sig-sine-phase", detail::default_signal_sine_phase);
		sig_sine_bias = dcs::cli::simple::get_option<real_type>(argv, argv+argc, "--sig-sine-bias", detail::default_signal_sine_bias);
		sig_sine_mesh_ampl = dcs::cli::simple::get_option<real_type>(argv, argv+argc, "--sig-sine-mesh-ampl", detail::default_signal_sine_mesh_amplitude);
		sig_sine_mesh_freq = dcs::cli::simple::get_option<uint_type>(argv, argv+argc, "--sig-sine-mesh-freq", detail::default_signal_sine_mesh_frequency);
		sig_sine_mesh_phase = dcs::cli::simple::get_option<uint_type>(argv, argv+argc, "--sig-sine-mesh-phase", detail::default_signal_sine_mesh_phase);
		sig_sine_mesh_bias = dcs::cli::simple::get_option<real_type>(argv, argv+argc, "--sig-sine-mesh-bias", detail::default_signal_sine_mesh_bias);
		sig_half_sine_ampl = dcs::cli::simple::get_option<real_type>(argv, argv+argc, "--sig-half-sine-ampl", detail::default_signal_half_sine_amplitude);
		sig_half_sine_freq = dcs::cli::simple::get_option<uint_type>(argv, argv+argc, "--sig-half-sine-freq", detail::default_signal_half_sine_frequency);
		sig_half_sine_phase = dcs::cli::simple::get_option<uint_type>(argv, argv+argc, "--sig-half-sine-phase", detail::default_signal_half_sine_phase);
		sig_half_sine_bias = dcs::cli::simple::get_option<real_type>(argv, argv+argc, "--sig-half-sine-bias", detail::default_signal_half_sine_bias);
		sig_half_sine_mesh_ampl = dcs::cli::simple::get_option<real_type>(argv, argv+argc, "--sig-half-sine-mesh-ampl", detail::default_signal_half_sine_mesh_amplitude);
		sig_half_sine_mesh_freq = dcs::cli::simple::get_option<uint_type>(argv, argv+argc, "--sig-half-sine-mesh-freq", detail::default_signal_half_sine_mesh_frequency);
		sig_half_sine_mesh_phase = dcs::cli::simple::get_option<uint_type>(argv, argv+argc, "--sig-half-sine-mesh-phase", detail::default_signal_half_sine_mesh_phase);
		sig_half_sine_mesh_bias = dcs::cli::simple::get_option<real_type>(argv, argv+argc, "--sig-half-sine-mesh-bias", detail::default_signal_half_sine_mesh_bias);
		sig_square_low = dcs::cli::simple::get_option<real_type>(argv, argv+argc, "--sig-square-low", detail::default_signal_square_low);
		sig_square_high = dcs::cli::simple::get_option<real_type>(argv, argv+argc, "--sig-square-high", detail::default_signal_square_high);
		sig_unif_min = dcs::cli::simple::get_option<real_type>(argv, argv+argc, "--sig-uniform-min", detail::default_signal_uniform_min);
		sig_unif_max = dcs::cli::simple::get_option<real_type>(argv, argv+argc, "--sig-uniform-max", detail::default_signal_uniform_max);
		sig_gauss_mean = dcs::cli::simple::get_option<real_type>(argv, argv+argc, "--sig-gaussian-mean", detail::default_signal_gaussian_mean);
		sig_gauss_sd = dcs::cli::simple::get_option<real_type>(argv, argv+argc, "--sig-gaussian-sd", detail::default_signal_gaussian_sd);
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

		oss << "Signal category: " << sig;
		dcs::log_info(DCS_LOGGING_AT, oss.str());
		oss.str("");

		oss << "Signal lower bound: " << sig_common_lo_bound;
		dcs::log_info(DCS_LOGGING_AT, oss.str());
		oss.str("");

		oss << "Signal upper bound: " << sig_common_up_bound;
		dcs::log_info(DCS_LOGGING_AT, oss.str());
		oss.str("");

		switch (sig)
		{
			case detail::constant_signal:
				oss << "Constant signal - value: " << sig_const_val;
				dcs::log_info(DCS_LOGGING_AT, oss.str());
				oss.str("");
				break;
			case detail::gaussian_signal:
				oss << "Gaussian signal -"
					<< "  mean: " << sig_gauss_mean
					<< ", standard deviation: " << sig_gauss_sd;
				dcs::log_info(DCS_LOGGING_AT, oss.str());
				oss.str("");
				break;
			case detail::half_sinusoidal_signal:
				oss << "Half-sinusoidal signal -"
					<< "  amplitude: " << sig_half_sine_ampl
					<< ", frequency: " << sig_half_sine_freq
					<< ", phase: " << sig_half_sine_phase
					<< ", bias: " << sig_half_sine_bias;
				dcs::log_info(DCS_LOGGING_AT, oss.str());
				oss.str("");
				break;
			case detail::half_sinusoidal_mesh_signal:
				oss << "Half-sinusoidal mesh signal -"
					<< "  amplitude: " << sig_half_sine_mesh_ampl
					<< ", frequency: " << sig_half_sine_mesh_freq
					<< ", phase: " << sig_half_sine_mesh_phase
					<< ", bias: " << sig_half_sine_mesh_bias;
				dcs::log_info(DCS_LOGGING_AT, oss.str());
				oss.str("");
				break;
			case detail::sawtooth_signal:
				oss << "Sawtooth signal -"
					<< "  lower value: " << sig_sawtooth_low
					<< ", higher value: " << sig_sawtooth_high
					<< ", increment: " << sig_sawtooth_incr;
				dcs::log_info(DCS_LOGGING_AT, oss.str());
				oss.str("");
				break;
			case detail::sinusoidal_signal:
				oss << "Sinusoidal signal -"
					<< "  amplitude: " << sig_sine_ampl
					<< ", frequency: " << sig_sine_freq
					<< ", phase: " << sig_sine_phase
					<< ", bias: " << sig_sine_bias;
				dcs::log_info(DCS_LOGGING_AT, oss.str());
				oss.str("");
				break;
			case detail::sinusoidal_mesh_signal:
				oss << "Sinusoidal mesh signal -"
					<< "  amplitude: " << sig_sine_mesh_ampl
					<< ", frequency: " << sig_sine_mesh_freq
					<< ", phase: " << sig_sine_mesh_phase
					<< ", bias: " << sig_sine_mesh_bias;
				dcs::log_info(DCS_LOGGING_AT, oss.str());
				oss.str("");
				break;
			case detail::square_signal:
				oss << "Square signal -"
					<< "  lower value: " << sig_square_low
					<< ", higher value: " << sig_square_high;
				dcs::log_info(DCS_LOGGING_AT, oss.str());
				oss.str("");
				break;
			case detail::uniform_signal:
				oss << "Uniform signal -"
					<< "  minimum value: " << sig_unif_min
					<< ", maximum value: " << sig_unif_max;
				dcs::log_info(DCS_LOGGING_AT, oss.str());
				oss.str("");
				break;
			default:
				break;
		}

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
    typedef testbed::base_application<traits_type> app_type;
    typedef boost::shared_ptr<app_type> app_pointer;
	typedef testbed::base_workload_driver<traits_type> app_driver_type;
	typedef boost::shared_ptr<app_driver_type> app_driver_pointer;
	//typedef boost::random::mt19937 random_generator_type;
	typedef boost::random::mt19937 random_generator_type;

	try
	{
		const std::size_t nt(vm_uris.size()); // Number of tiers

		// Setup application experiment
		//  - Setup application (and VMs)
		std::map<vmm_identifier_type,vmm_pointer> vmm_map;
		std::vector<vm_pointer> vms;
		std::vector<std::string>::const_iterator uri_end_it(vm_uris.end());
		for (std::vector<std::string>::const_iterator it = vm_uris.begin();
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

		// - Setup workload driver
		app_driver_pointer p_drv;
		switch (wkl_driver)
		{
			case testbed::rain_workload_generator:
				{
					boost::shared_ptr< testbed::rain::workload_driver<traits_type> > p_drv_impl = boost::make_shared< testbed::rain::workload_driver<traits_type> >(wkl, wkl_driver_rain_path);
					p_app->register_sensor(testbed::response_time_application_performance, p_drv_impl->sensor(testbed::response_time_application_performance));
					p_drv = p_drv_impl;
				}
				break;
		}
		p_drv->app(p_app);

		// - Setup signal generator
		random_generator_type rng(rng_seed);
		boost::shared_ptr< testbed::base_signal_generator<real_type> > p_sig_gen;
		// Specialized params
		switch (sig)
		{
			case detail::constant_signal:
				{
					std::vector<real_type> u0(nt, sig_const_val);
					p_sig_gen = boost::make_shared< testbed::constant_signal_generator<real_type> >(u0);
				}
				break;
			case detail::gaussian_signal:
				{
					std::vector<real_type> mean(nt, sig_gauss_mean);
					std::vector<real_type> sd(nt, sig_gauss_sd);
					//p_sig_gen = boost::make_shared< testbed::gaussian_signal_generator<real_type,random_generator_type> >(mean, sd, rng);
					p_sig_gen = boost::shared_ptr< testbed::base_signal_generator<real_type> >(new testbed::gaussian_signal_generator<real_type,random_generator_type>(mean, sd, rng));
				}
				break;
			case detail::half_sinusoidal_signal:
				{
					std::vector<real_type> ampl(nt, sig_half_sine_ampl);
					std::vector<uint_type> freq(nt, sig_half_sine_freq);
					std::vector<uint_type> phase(nt, sig_half_sine_phase);
					std::vector<real_type> bias(nt, sig_half_sine_bias);
					p_sig_gen = boost::make_shared< testbed::half_sinusoidal_signal_generator<real_type,uint_type> >(ampl, freq, phase, bias);
				}
				break;
			case detail::half_sinusoidal_mesh_signal:
				{
					std::vector<real_type> ampl(nt, sig_half_sine_mesh_ampl);
					std::vector<uint_type> freq(nt, sig_half_sine_mesh_freq);
					std::vector<uint_type> phase(nt, sig_half_sine_mesh_phase);
					std::vector<real_type> bias(nt, sig_half_sine_mesh_bias);
					p_sig_gen = boost::make_shared< testbed::half_sinusoidal_mesh_signal_generator<real_type,uint_type> >(ampl, freq, phase, bias);
				}
				break;
			case detail::sawtooth_signal:
				{
					std::vector<real_type> low(nt, sig_sawtooth_low);
					std::vector<real_type> high(nt, sig_sawtooth_high);
					std::vector<real_type> incr(nt, sig_sawtooth_incr);
					p_sig_gen = boost::make_shared< testbed::sawtooth_signal_generator<real_type> >(low, high, incr);
				}
				break;
			case detail::sinusoidal_signal:
				{
					std::vector<real_type> ampl(nt, sig_sine_ampl);
					std::vector<uint_type> freq(nt, sig_sine_freq);
					std::vector<uint_type> phase(nt, sig_sine_phase);
					std::vector<real_type> bias(nt, sig_sine_bias);
					p_sig_gen = boost::make_shared< testbed::sinusoidal_signal_generator<real_type,uint_type> >(ampl, freq, phase, bias);
				}
				break;
			case detail::sinusoidal_mesh_signal:
				{
					std::vector<real_type> ampl(nt, sig_sine_mesh_ampl);
					std::vector<uint_type> freq(nt, sig_sine_mesh_freq);
					std::vector<uint_type> phase(nt, sig_sine_mesh_phase);
					std::vector<real_type> bias(nt, sig_sine_mesh_bias);
					p_sig_gen = boost::make_shared< testbed::sinusoidal_mesh_signal_generator<real_type,uint_type> >(ampl, freq, phase, bias);
				}
				break;
			case detail::square_signal:
				{
					std::vector<real_type> low(nt, sig_square_low);
					std::vector<real_type> high(nt, sig_square_high);
					p_sig_gen = boost::make_shared< testbed::square_signal_generator<real_type> >(low, high);
				}
				break;
			case detail::uniform_signal:
				{
					std::vector<real_type> min(nt, sig_unif_min);
					std::vector<real_type> max(nt, sig_unif_max);
					//p_sig_gen = boost::make_shared< testbed::uniform_signal_generator<real_type,random_generator_type> >(min, max, rng);
					p_sig_gen = boost::shared_ptr< testbed::base_signal_generator<real_type> >(new testbed::uniform_signal_generator<real_type,random_generator_type>(min, max, rng));
				}
				break;
			default:
				DCS_EXCEPTION_THROW(::std::runtime_error, "Unknown signal generator");
				break;
		}
		// Common params
		p_sig_gen->upper_bound(sig_common_up_bound);
		p_sig_gen->lower_bound(sig_common_lo_bound);

		testbed::system_identification<traits_type> sysid(p_app, p_drv, p_sig_gen);
		sysid.output_data_file(out_dat_file);
		sysid.sampling_time(ts);
		sysid.output_extended_format(true);

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
