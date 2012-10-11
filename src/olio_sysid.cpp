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
#include <boost/random.hpp>
#include <boost/smart_ptr.hpp>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <dcs/cli.hpp>
#include <dcs/logging.hpp>
#include <dcs/testbed/signal_generators.hpp>
#include <dcs/testbed/system_identification.hpp>
#include <dcs/testbed/virtual_machines.hpp>
#include <dcs/testbed/workload_drivers.hpp>
#include <iostream>
#include <sstream>
#include <string>
#include <stdexcept>
#include <vector>


namespace detail { namespace /*<unnamed>*/ {

enum signal_category
{
	constant_signal,
	gaussian_signal,
	sawtooth_signal,
	sinusoidal_signal,
	sinusoidal_mesh_signal,
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
const ::std::string default_workload_driver_path("/usr/local/rain-workload-toolkit");
const ::std::string default_out_dat_file("./olio-sysid-out.dat");
const double default_sampling_time(10);
const signal_category default_signal_category(constant_signal);
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
				<< " --db-uri <URI>" << ::std::endl
				<< "   The URI used to connect to the libvirtd server where the OlioDB VM is running." << ::std::endl
				<< "   [default: default URI of this machine]." << ::std::endl
				<< " --db-name <name>" << ::std::endl
				<< "   The name of the domain running the OlioDB VM." << ::std::endl
				<< "   [default: OlioDB]." << ::std::endl
				<< " --help" << ::std::endl
				<< "   Show this message." << ::std::endl
				<< " --out-dat-file <file path>" << ::std::endl
				<< "   The path to the output data file." << ::std::endl
				<< "   [default: ./olio-sysid-out.dat]." << ::std::endl
				<< " --sig <signal category>" << ::std::endl
				<< "   The type of signal used to excite the system under test." << ::std::endl
				<< "   Possible values are:" << ::std::endl
				<< "   - constant" << ::std::endl
				<< "   - gaussian" << ::std::endl
				<< "   - sawtooth" << ::std::endl
				<< "   - sine" << ::std::endl
				<< "   - sine-mesh" << ::std::endl
				<< "   - square" << ::std::endl
				<< "   - uniform" << ::std::endl
				<< "   [default: constant]." << ::std::endl
				<< " --ts <time in secs>" << ::std::endl
				<< "   Sampling time (in seconds)." << ::std::endl
				<< "   [default: 10]." << ::std::endl
				<< " --verbose" << ::std::endl
				<< "   Show verbose messages." << ::std::endl
				<< "   [default: disabled]." << ::std::endl
				<< " --web-uri <URI>" << ::std::endl
				<< "   The URI used to connect to the libvirtd server where the OlioWeb VM is running." << ::std::endl
				<< "   [default: default URI of this machine]." << ::std::endl
				<< " --web-name <name>" << ::std::endl
				<< "   The name of the domain running the OlioWeb VM." << ::std::endl
				<< "   [default: OlioWeb]." << ::std::endl
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
	uint_type rng_seed(5498);
	detail::signal_category sig;
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
	real_type sig_gauss_mean;
	real_type sig_gauss_sd;
	real_type ts;
	bool verbose(false);
	std::string wkl_driver_path;

	// Parse command line options
	try
	{
		oliodb_uri = dcs::cli::simple::get_option<std::string>(argv, argv+argc, "--db-uri", detail::default_oliodb_uri);
		oliodb_name = dcs::cli::simple::get_option<std::string>(argv, argv+argc, "--db-name", detail::default_oliodb_name);
		help = dcs::cli::simple::get_option(argv, argv+argc, "--help");
		out_dat_file = dcs::cli::simple::get_option<std::string>(argv, argv+argc, "--out-dat-file", detail::default_out_dat_file);
		sig = dcs::cli::simple::get_option<detail::signal_category>(argv, argv+argc, "--sig", detail::default_signal_category);
		sig_sawtooth_low = dcs::cli::simple::get_option<real_type>(argv, argv+argc, "--sig-sawtooth-low", detail::default_signal_sawtooth_low);
		sig_sawtooth_high = dcs::cli::simple::get_option<real_type>(argv, argv+argc, "--sig-sawtooth-high", detail::default_signal_sawtooth_high);
		sig_sawtooth_incr = dcs::cli::simple::get_option<real_type>(argv, argv+argc, "--sig-sawtooth-incr", detail::default_signal_sawtooth_incr);
		sig_sine_ampl = dcs::cli::simple::get_option<real_type>(argv, argv+argc, "--sig-sine-amplitude", detail::default_signal_sine_amplitude);
		sig_sine_freq = dcs::cli::simple::get_option<uint_type>(argv, argv+argc, "--sig-sine-frequency", detail::default_signal_sine_frequency);
		sig_sine_phase = dcs::cli::simple::get_option<uint_type>(argv, argv+argc, "--sig-sine-phase", detail::default_signal_sine_phase);
		sig_sine_bias = dcs::cli::simple::get_option<real_type>(argv, argv+argc, "--sig-sine-bias", detail::default_signal_sine_bias);
		sig_sine_mesh_ampl = dcs::cli::simple::get_option<real_type>(argv, argv+argc, "--sig-sine-mesh-amplitude", detail::default_signal_sine_mesh_amplitude);
		sig_sine_mesh_freq = dcs::cli::simple::get_option<uint_type>(argv, argv+argc, "--sig-sine-mesh-frequency", detail::default_signal_sine_mesh_frequency);
		sig_sine_mesh_phase = dcs::cli::simple::get_option<uint_type>(argv, argv+argc, "--sig-sine-mesh-phase", detail::default_signal_sine_mesh_phase);
		sig_sine_mesh_bias = dcs::cli::simple::get_option<real_type>(argv, argv+argc, "--sig-sine-mesh-bias", detail::default_signal_sine_mesh_bias);
		sig_square_low = dcs::cli::simple::get_option<real_type>(argv, argv+argc, "--sig-square-low", detail::default_signal_square_low);
		sig_square_high = dcs::cli::simple::get_option<real_type>(argv, argv+argc, "--sig-square-high", detail::default_signal_square_high);
		sig_unif_min = dcs::cli::simple::get_option<real_type>(argv, argv+argc, "--sig-uniform-min", detail::default_signal_uniform_min);
		sig_unif_max = dcs::cli::simple::get_option<real_type>(argv, argv+argc, "--sig-uniform-max", detail::default_signal_uniform_max);
		sig_gauss_mean = dcs::cli::simple::get_option<real_type>(argv, argv+argc, "--sig-gaussian-mean", detail::default_signal_gaussian_mean);
		sig_gauss_sd = dcs::cli::simple::get_option<real_type>(argv, argv+argc, "--sig-gaussian-sd", detail::default_signal_gaussian_sd);
		ts = dcs::cli::simple::get_option<real_type>(argv, argv+argc, "--ts", detail::default_sampling_time);
		verbose = dcs::cli::simple::get_option(argv, argv+argc, "--verbose");
		olioweb_uri = dcs::cli::simple::get_option<std::string>(argv, argv+argc, "--web-uri", detail::default_olioweb_uri);
		olioweb_name = dcs::cli::simple::get_option<std::string>(argv, argv+argc, "--web-name", detail::default_olioweb_name);
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

		oss << "Output data file: " << out_dat_file;
		dcs::log_info(DCS_LOGGING_AT, oss.str());
		oss.str("");

		oss << "Signal category: " << sig;
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
	namespace ublas = ::boost::numeric::ublas;

	typedef boost::shared_ptr< testbed::base_virtual_machine<real_type> > vm_pointer;
	//typedef boost::random::mt19937 random_generator_type;
	typedef boost::random::mt19937 random_generator_type;

	try
	{
		vm_pointer p_oliodb_vm(new testbed::libvirt_virtual_machine<real_type>(oliodb_uri, oliodb_name));
		vm_pointer p_olioweb_vm(new testbed::libvirt_virtual_machine<real_type>(olioweb_uri, olioweb_name));

		const std::size_t nt(2); // Number of tiers

		std::vector<vm_pointer> vms(nt);
		vms[0] = p_olioweb_vm;
		vms[1] = p_oliodb_vm;

		boost::shared_ptr< testbed::base_workload_driver > p_driver(new testbed::rain_workload_driver(testbed::rain_workload_driver::olio_workload, wkl_driver_path));

		random_generator_type rng(rng_seed);

		//boost::shared_ptr< testbed::base_signal_generator<real_type> > p_sig_gen(new testbed::sinusoidal_mesh_signal_generator<real_type>(ampl, freq, phase, bias));
		boost::shared_ptr< testbed::base_signal_generator<real_type> > p_sig_gen;
		switch (sig)
		{
			case detail::constant_signal:
				{
					ublas::scalar_vector<real_type> u0(nt, 1);
					p_sig_gen = boost::make_shared< testbed::constant_signal_generator<real_type> >(u0);
				}
				break;
			case detail::gaussian_signal:
				{
					ublas::scalar_vector<real_type> mean(nt, sig_gauss_mean);
					ublas::scalar_vector<real_type> sd(nt, sig_gauss_sd);
					p_sig_gen = boost::shared_ptr< testbed::base_signal_generator<real_type> >(new testbed::gaussian_signal_generator<real_type, random_generator_type>(mean, sd, rng));
				}
				break;
			case detail::sawtooth_signal:
				{
					ublas::scalar_vector<real_type> low(nt, sig_sawtooth_low);
					ublas::scalar_vector<real_type> high(nt, sig_sawtooth_high);
					ublas::scalar_vector<real_type> incr(nt, sig_sawtooth_incr);
					p_sig_gen = boost::make_shared< testbed::sawtooth_signal_generator<real_type> >(low, high, incr);
				}
				break;
			case detail::sinusoidal_signal:
				{
					ublas::scalar_vector<real_type> ampl(nt, sig_sine_ampl);
					ublas::scalar_vector<real_type> freq(nt, sig_sine_freq);
					ublas::scalar_vector<real_type> phase(nt, sig_sine_phase);
					ublas::scalar_vector<real_type> bias(nt, sig_sine_bias);
					p_sig_gen = boost::make_shared< testbed::sinusoidal_signal_generator<real_type> >(ampl, freq, phase, bias);
				}
				break;
			case detail::sinusoidal_mesh_signal:
				{
					ublas::scalar_vector<real_type> ampl(nt, sig_sine_mesh_ampl);
					ublas::scalar_vector<real_type> freq(nt, sig_sine_mesh_freq);
					ublas::scalar_vector<real_type> phase(nt, sig_sine_mesh_phase);
					ublas::scalar_vector<real_type> bias(nt, sig_sine_mesh_bias);
					p_sig_gen = boost::make_shared< testbed::sinusoidal_mesh_signal_generator<real_type> >(ampl, freq, phase, bias);
				}
				break;
			case detail::square_signal:
				{
					ublas::scalar_vector<real_type> low(nt, sig_square_low);
					ublas::scalar_vector<real_type> high(nt, sig_square_high);
					p_sig_gen = boost::make_shared< testbed::square_signal_generator<real_type> >(low, high);
				}
				break;
			case detail::uniform_signal:
				{
					ublas::scalar_vector<real_type> min(nt, sig_unif_min);
					ublas::scalar_vector<real_type> max(nt, sig_unif_max);
					//p_sig_gen = boost::make_shared< testbed::uniform_signal_generator<real_type,random_generator_type> >(min, max, rng);
					p_sig_gen = boost::shared_ptr< testbed::base_signal_generator<real_type> >(new testbed::uniform_signal_generator<real_type, random_generator_type>(min, max, rng));
				}
				break;
			default:
				DCS_EXCEPTION_THROW(::std::runtime_error, "Unknown signal generator");
				break;
		}

		testbed::system_identification<real_type> sysid(vms.begin(), vms.end(), p_driver, p_sig_gen);
		sysid.output_data_file(out_dat_file);
		sysid.sampling_time(ts);

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
