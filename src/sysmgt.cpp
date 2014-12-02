/**
 * \file sysmgt.hpp
 *
 * \brief Driver for performing system experiments
 *
 * \author Marco Guazzone (marco.guazzone@gmail.com)
 *
 * <hr/>
 *
 * Copyright 2012 Marco Guazzone (marco.guazzone@gmail.com)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <boost/random.hpp>
#include <boost/smart_ptr.hpp>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <dcs/cli.hpp>
#include <dcs/debug.hpp>
#include <dcs/logging.hpp>
#include <dcs/testbed/configurators.hpp>
#include <dcs/testbed/experiment_stats_gatherer.hpp>
#include <dcs/testbed/system_experiment.hpp>
#include <iostream>
#include <sstream>
#include <string>
#include <stdexcept>


namespace detail { namespace /*<unnamed>*/ {

const bool default_verbose = false;
const std::string default_cfg_file("config.yaml");


void usage(char const* progname)
{
	std::cerr << "Usage: " << progname << " {options}" << ::std::endl
			  << " --config <filename>" << ::std::endl
			  << "   The path to the configuration file." << ::std::endl
			  << " --help" << ::std::endl
			  << "   Shows this message." << ::std::endl
			  << " --verbose" << ::std::endl
			  << "   Shows verbose messages." << ::std::endl
			  << "   [default: " << (default_verbose ? "enabled" : "disabled") << "]" << std::endl
			  << ::std::endl;
}

}} // Namespace detail::<unnamed>


int main(int argc, char *argv[])
{
	namespace testbed = ::dcs::testbed;


	typedef double real_type;
	typedef unsigned int uint_type;
	typedef boost::random::mt19937 rng_type;
	typedef testbed::traits<real_type,uint_type,rng_type> traits_type;


	std::string opt_cfg_file;
	bool opt_help = false;
	bool opt_verbose = false;


	// Parse command line options
	try
	{
		opt_cfg_file = dcs::cli::simple::get_option<std::string>(argv, argv+argc, "--config");
		opt_help = dcs::cli::simple::get_option(argv, argv+argc, "--help");
		opt_verbose = dcs::cli::simple::get_option(argv, argv+argc, "--verbose");
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

	int ret = 0;

	if (opt_verbose)
	{
	}

	try
	{
		testbed::conf::yaml_configurator<traits_type> config;

		boost::shared_ptr< testbed::system_experiment<traits_type> > p_sys_exp = config.configure(opt_cfg_file);

		// Set experiment trackers
		testbed::utility::experiment_stats_gatherer<traits_type> exp_stats;
		exp_stats.track(*p_sys_exp);


		//p_sys_exp->logger(...);
		//p_sys_exp->output_data_file(out_dat_file);

		// Run!
		p_sys_exp->run();
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
