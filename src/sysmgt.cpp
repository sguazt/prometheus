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

//#include <boost/numeric/ublas/banded.hpp>
#include <boost/smart_ptr.hpp>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <dcs/cli.hpp>
#include <dcs/debug.hpp>
#include <dcs/logging.hpp>
#include <dcs/math/traits/float.hpp>
#include <dcs/testbed/application.hpp>
#include <dcs/testbed/application_experiment.hpp>
#include <dcs/testbed/application_managers.hpp>
#include <dcs/testbed/application_performance_category.hpp>
#include <dcs/testbed/base_application.hpp>
#include <dcs/testbed/data_estimators.hpp>
#include <dcs/testbed/data_smoothers.hpp>
#include <dcs/testbed/experiment_stats_gatherer.hpp>
#include <dcs/testbed/system_experiment.hpp>
#include <dcs/testbed/system_identification_strategies.hpp>
//#include <dcs/testbed/system_managers.hpp>
#include <dcs/testbed/traits.hpp>
#include <dcs/testbed/virtual_machine_managers.hpp>
#include <dcs/testbed/virtual_machine_performance_category.hpp>
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

enum data_estimator_category
{
	mean_estimator,
	chen2000_ewma_quantile_estimator,
	chen2000_ewsa_quantile_estimator,
	chen2000_sa_quantile_estimator,
	jain1985_p2_algorithm_quantile_estimator,
	most_recently_observed_estimator,
	true_quantile_estimator,
	welsh2003_ewma_quantile_estimator,
	welsh2003_ewma_ext_quantile_estimator
};

enum data_smoother_category
{
	dummy_smoother,
	brown_single_exponential_smoother,
	brown_double_exponential_smoother,
	holt_winters_double_exponential_smoother
};

enum app_manager_category
{
	albano2013_fuzzyqe_app_manager,
	anglano2014_fc2q_app_manager,
	dummy_app_manager,
	padala2009_autocontrol_app_manager,
	rao2013_dynaqos_app_manager
};


const dcs::testbed::workload_category default_workload = dcs::testbed::olio_workload;
const dcs::testbed::workload_generator_category default_workload_driver = dcs::testbed::rain_workload_generator;
const ::std::string default_workload_driver_rain_path("/usr/local/opt/rain-workload-toolkit");
const ::std::string default_workload_driver_ycsb_path("/usr/local/opt/YCSB");
const ::std::string default_workload_ycsb_prop_path("workloads/workloada");
const ::std::string default_workload_ycsb_classpath;
const ::std::string default_workload_ycsb_db_class;
//const ::std::string default_out_dat_file("./sysmgt-out.dat");
const double default_sampling_time = 10;
const double default_control_time = 3*default_sampling_time;
const data_estimator_category default_data_estimator = mean_estimator;
const double default_quantile_prob = 0.99;
const double default_jain1985_p2_quantile_prob = default_quantile_prob;
const double default_chen2000_ewma_w = 0.05;
const double default_chen2000_ewma_quantile_prob = default_quantile_prob;
const double default_chen2000_ewsa_w = 0.05;
const double default_chen2000_ewsa_quantile_prob = default_quantile_prob;
const double default_chen2000_sa_quantile_prob = default_quantile_prob;
const double default_true_quantile_prob = default_quantile_prob;
const double default_welsh2003_ewma_alpha = 0.7;
const double default_welsh2003_ewma_quantile_prob = default_quantile_prob;
const data_smoother_category default_data_smoother = dummy_smoother;
const double default_brown_single_exponential_alpha = 0.7;
const double default_brown_double_exponential_alpha = 0.7;
const double default_holt_winters_double_exponential_alpha = 0.8;
const double default_holt_winters_double_exponential_beta = 0.3;
const double default_holt_winters_double_exponential_delta = 0.7;
const std::string default_slo_metric_str("rt");
const double default_slo_value = 0;
const bool default_no_restore_vms = false;
const bool default_verbose = false;
const app_manager_category default_app_manager = dummy_app_manager;


template <typename CharT, typename CharTraitsT>
inline
::std::basic_istream<CharT,CharTraitsT>& operator>>(::std::basic_istream<CharT,CharTraitsT>& is, data_smoother_category& cat)
{
	::std::string s;
	is >> s;
	::dcs::string::to_lower(s);

	if (!s.compare("brown_ses"))
	{
		cat = brown_single_exponential_smoother;
	}
	else if (!s.compare("dummy"))
	{
		cat = dummy_smoother;
	}
	else if (!s.compare("brown_des"))
	{
		cat = brown_double_exponential_smoother;
	}
	else if (!s.compare("holt_winters_des"))
	{
		cat = holt_winters_double_exponential_smoother;
	}
	else
	{
		DCS_EXCEPTION_THROW(::std::runtime_error,
							"Unknown data smoother category");
	}

	return is;
}

template <typename CharT, typename CharTraitsT>
inline
::std::basic_ostream<CharT,CharTraitsT>& operator>>(::std::basic_ostream<CharT,CharTraitsT>& os, data_smoother_category cat)
{
	switch (cat)
	{
		case brown_single_exponential_smoother:
			os << "brown_ses";
			break;
		case brown_double_exponential_smoother:
			os << "brown_des";
			break;
		case dummy_smoother:
			os << "dummy";
			break;
		case holt_winters_double_exponential_smoother:
			os << "holt_winters_des";
			break;
	}

	return os;
}

template <typename CharT, typename CharTraitsT>
inline
::std::basic_istream<CharT,CharTraitsT>& operator>>(::std::basic_istream<CharT,CharTraitsT>& is, data_estimator_category& cat)
{
	::std::string s;
	is >> s;
	::dcs::string::to_lower(s);

	if (!s.compare("mean"))
	{
		cat = mean_estimator;
	}
	else if (!s.compare("mro"))
	{
		cat = most_recently_observed_estimator;
	}
	else if (!s.compare("chen2000_ewma_quantile"))
	{
		cat = chen2000_ewma_quantile_estimator;
	}
	else if (!s.compare("chen2000_ewsa_quantile"))
	{
		cat = chen2000_ewsa_quantile_estimator;
	}
	else if (!s.compare("chen2000_sa_quantile"))
	{
		cat = chen2000_sa_quantile_estimator;
	}
	else if (!s.compare("jain1985_p2_algorithm_quantile"))
	{
		cat = jain1985_p2_algorithm_quantile_estimator;
	}
	else if (!s.compare("true_quantile"))
	{
		cat = true_quantile_estimator;
	}
	else if (!s.compare("welsh2003_ewma_quantile"))
	{
		cat = welsh2003_ewma_quantile_estimator;
	}
	else if (!s.compare("welsh2003_ewma_ext_quantile"))
	{
		cat = welsh2003_ewma_ext_quantile_estimator;
	}
	else
	{
		DCS_EXCEPTION_THROW(::std::runtime_error,
							"Unknown data estimator category");
	}

	return is;
}

template <typename CharT, typename CharTraitsT>
inline
::std::basic_ostream<CharT,CharTraitsT>& operator>>(::std::basic_ostream<CharT,CharTraitsT>& os, data_estimator_category cat)
{
	switch (cat)
	{
		case mean_estimator:
			os << "mean";
			break;
		case most_recently_observed_estimator:
			os << "mro";
			break;
		case chen2000_ewma_quantile_estimator:
				os << "chen2000_ewma_quantile";
				break;
		case chen2000_ewsa_quantile_estimator:
				os << "chen2000_ewsa_quantile";
				break;
		case chen2000_sa_quantile_estimator:
				os << "chen2000_sa_quantile";
				break;
		case jain1985_p2_algorithm_quantile_estimator:
				os << "jain1985_p2_algorithm_quantile";
				break;
		case true_quantile_estimator:
				os << "true_quantile";
				break;
		case welsh2003_ewma_quantile_estimator:
				os << "welsh2003_ewma_quantile";
				break;
		case welsh2003_ewma_ext_quantile_estimator:
				os << "welsh2003_ewma_ext_quantile";
				break;
	}

	return os;
}

template <typename CharT, typename CharTraitsT>
inline
::std::basic_istream<CharT,CharTraitsT>& operator>>(::std::basic_istream<CharT,CharTraitsT>& is, app_manager_category& cat)
{
	::std::string s;
	is >> s;
	::dcs::string::to_lower(s);

	if (!s.compare("albano2013"))
	{
		cat = albano2013_fuzzyqe_app_manager;
	}
	else if (!s.compare("anglano2014_fc2q"))
	{
		cat = anglano2014_fc2q_app_manager;
	}
	else if (!s.compare("dummy"))
	{
		cat = dummy_app_manager;
	}
	else if (!s.compare("padala2009_autocontrol"))
	{
		cat = padala2009_autocontrol_app_manager;
	}
	else if (!s.compare("rao2013_dynaqos"))
	{
		cat = rao2013_dynaqos_app_manager;
	}
	else
	{
		DCS_EXCEPTION_THROW(::std::runtime_error,
							"Unknown application manager category");
	}

	return is;
}

template <typename CharT, typename CharTraitsT>
inline
::std::basic_ostream<CharT,CharTraitsT>& operator>>(::std::basic_ostream<CharT,CharTraitsT>& os, app_manager_category cat)
{
	switch (cat)
	{
		case albano2013_fuzzyqe_app_manager:
			os << "albano2013";
			break;
		case anglano2014_fc2q_app_manager:
			os << "anglano2014_fc2q";
			break;
		case dummy_app_manager:
			os << "dummy";
			break;
		case padala2009_autocontrol_app_manager:
			os << "padala2009_autocontrol";
			break;
		case rao2013_dynaqos_app_manager:
			os << "rao2013_dynaqos";
			break;
	}

	return os;
}

void usage(char const* progname)
{
	::std::cerr << "Usage: " << progname << " [options]" << ::std::endl
				<< " --help" << ::std::endl
				<< "   Show this message." << ::std::endl
//				<< " --out-dat-file <file path>" << ::std::endl
//				<< "   The path to the output data file." << ::std::endl
//				<< "   [default: '" << default_out_dat_file << "']." << ::std::endl
				<< " --app-manager <name>" << ::std::endl
				<< "   The name of the application manager to use to manage applications." << ::std::endl
				<< "   Possible values are:" << ::std::endl
				<< "   - 'albano2013': the fuzzy controller described in (Albano et al., 2013)" << ::std::endl
				<< "   - 'anglano2014_fc2q': a variant of the fuzzy controller described in (Albano et al., 2013)" << ::std::endl
				<< "   - 'dummy': a 'do-nothing' application manager" << ::std::endl
				<< "   - 'padala2009_autocontrol': the LQ controller described in (Padala et al., 2009)" << ::std::endl
				<< "   - 'rao2013_dynaqos': the fuzzy controller described in (Rao et al., 2013)" << ::std::endl
				<< "   [default: '" << default_app_manager << "']." << ::std::endl
				<< " --data-estimator <name>" << ::std::endl
				<< "   The name of the estimator to use to estimate summary statistics from observed data." << ::std::endl
				<< "   Possible values are:" << ::std::endl
				<< "   - 'chen2000_ewma_quantile': quantile estimation according to the EWMA method by (Chen et al., 2000)" << ::std::endl
				<< "   - 'chen2000_ewsa_quantile': quantile estimation according to the EWSA method by (Chen et al., 2000)" << ::std::endl
				<< "   - 'chen2000_sa_quantile': quantile estimation according to the SA method by (Chen et al., 2000)" << ::std::endl
				<< "   - 'jain1985_p2_algorithm_quantile': quantile estimation according to the P^2 algorithm by (Jain et al., 1985)" << ::std::endl
				<< "   - 'mean': sample mean" << ::std::endl
				<< "   - 'mro': most recently observed data" << ::std::endl
				<< "   - 'true_ewma_quantile': true quantile estimation" << ::std::endl
				<< "   - 'welsh2003_ewma_quantile': quantile estimation according to the EWMA method by (Welsh et al., 2003)" << ::std::endl
				<< "   - 'welsh2003_ewma_ext_quantile': quantile estimation according to the extended EWMA method by (Welsh et al., 2003)" << ::std::endl
				<< "   [default: '" << default_data_estimator << "']." << ::std::endl
//				<< " --quantile-prob <value>" << ::std::endl
//				<< "   The probability value for the quantile-based data estimator." << ::std::endl
//				<< "   [default: '" << default_quantile_prob << "']." << ::std::endl
				<< " --chen2000_ewma-quantile <value>" << ::std::endl
				<< "   The probability value for the (Chen el al.,2000) EWMA quantile estimator." << ::std::endl
				<< "   [default: '" << default_chen2000_ewma_quantile_prob << "']." << ::std::endl
				<< " --chen2000_ewma-w <value>" << ::std::endl
				<< "   The w parameter for the (Chen el al.,2000) EWMA quantile estimator." << ::std::endl
				<< "   [default: '" << default_chen2000_ewma_w << "']." << ::std::endl
				<< " --chen2000_ewsa-quantile <value>" << ::std::endl
				<< "   The probability value for the (Chen el al.,2000) EWSA quantile estimator." << ::std::endl
				<< "   [default: '" << default_chen2000_ewsa_quantile_prob << "']." << ::std::endl
				<< " --chen2000_ewsa-w <value>" << ::std::endl
				<< "   The w parameter for the (Chen el al.,2000) EWSA quantile estimator." << ::std::endl
				<< "   [default: '" << default_chen2000_ewsa_w << "']." << ::std::endl
				<< " --chen2000_sa-quantile <value>" << ::std::endl
				<< "   The probability value for the (Chen el al.,2000) SA quantile estimator." << ::std::endl
				<< "   [default: '" << default_chen2000_sa_quantile_prob << "']." << ::std::endl
				<< " --jain1985_p2-quantile <value>" << ::std::endl
				<< "   The probability value for the (Jain et al.,1985) P^2 quantile estimator." << ::std::endl
				<< "   [default: '" << default_jain1985_p2_quantile_prob << "']." << ::std::endl
				<< " --true-quantile <value>" << ::std::endl
				<< "   The probability value for the true quantile estimator." << ::std::endl
				<< "   [default: '" << default_true_quantile_prob << "']." << ::std::endl
				<< " --welsh2003_ewma-alpha <value>" << ::std::endl
				<< "   The alpha parameter for the (Welsh el al.,2003) EWMA quantile estimator." << ::std::endl
				<< "   [default: '" << default_welsh2003_ewma_alpha << "']." << ::std::endl
				<< " --welsh2003_ewma-quantile <value>" << ::std::endl
				<< "   The probability value for the (Welsh el al.,2003) EWMA quantile estimator." << ::std::endl
				<< "   [default: '" << default_welsh2003_ewma_quantile_prob << "']." << ::std::endl
				<< " --data-smoother {'brown_ses'|'brown_des'|'dummy'|'holt_winters_des'}" << ::std::endl
				<< "   The name of the smoother to use to smooth observed data." << ::std::endl
				<< "   [default: '" << default_data_smoother << "']." << ::std::endl
				<< " --brown_ses-alpha <value>" << ::std::endl
				<< "   The smoothing factor parameter for the Brown Single Exponential data smoother." << ::std::endl
				<< "   [default: '" << default_brown_single_exponential_alpha << "']." << ::std::endl
				<< " --brown_des-alpha <value>" << ::std::endl
				<< "   The smoothing factor parameter for the Brown Double Exponential data smoother." << ::std::endl
				<< "   [default: '" << default_brown_double_exponential_alpha << "']." << ::std::endl
				<< " --holt_winters_des-alpha <value>" << ::std::endl
				<< "   The alpha parameter for the Holt-Winters Double Exponential data smoother." << ::std::endl
				<< "   [default: '" << default_holt_winters_double_exponential_alpha << "']." << ::std::endl
				<< " --holt_winters_des-beta <value>" << ::std::endl
				<< "   The beta parameter for the Holt-Winters Double Exponential data smoother." << ::std::endl
				<< "   [default: '" << default_holt_winters_double_exponential_beta << "']." << ::std::endl
				<< " --holt_winters_des-delta <value>" << ::std::endl
				<< "   The delta parameter for the Holt-Winters Double Exponential data smoother." << ::std::endl
				<< "   [default: '" << default_holt_winters_double_exponential_delta << "']." << ::std::endl
				<< " --no-restore-vms" << ::std::endl
				<< "   Don't restore the resource allocations of all VMS after experiment's completion" << ::std::endl
				<< "   [default: " << (default_no_restore_vms ? "enabled" : "disabled") << "]." << ::std::endl
				<< " --slo-metric <name>" << ::std::endl
				<< "   The SLO metric. Possible values are: 'rt' (response time), 'tput' (throughput)" << ::std::endl
				<< "   [default: '" << default_slo_metric_str << "']." << ::std::endl
				<< " --slo-value <value>" << ::std::endl
				<< "   The target value for the SLO metric." << ::std::endl
				<< "   [default: '" << default_slo_value << "']." << ::std::endl
				<< " --tc <value>" << ::std::endl
				<< "   Control time (in seconds)." << ::std::endl
				<< "   [default: " << default_control_time << "]." << ::std::endl
				<< " --ts <value>" << ::std::endl
				<< "   Sampling time (in seconds)." << ::std::endl
				<< "   [default: " << default_sampling_time << "]." << ::std::endl
				<< " --verbose" << ::std::endl
				<< "   Show verbose messages." << ::std::endl
				<< "   [default: " << (default_verbose ? "enabled" : "disabled") << "]." << ::std::endl
				<< " --vm-uri <URI>" << ::std::endl
				<< "   The VM URI to connect." << ::std::endl
				<< "   Repeat this option as many times as is the number of your VMs." << ::std::endl
				<< " --wkl <name>" << ::std::endl
				<< "   The workload to generate. Possible values are: 'cassandra', 'olio', 'rubis'." << ::std::endl
				<< "   [default: '" << ::dcs::testbed::to_string(default_workload) << "']." << ::std::endl
				<< " --wkl-driver <name>" << ::std::endl
				<< "   The workload driver to use. Possible values are: 'rain', 'ycsb'." << ::std::endl
				<< "   [default: '" << ::dcs::testbed::to_string(default_workload_driver) << "']." << ::std::endl
				<< " --wkl-driver-rain-path <name>" << ::std::endl
				<< "   The full path to the RAIN workload driver." << ::std::endl
				<< "   [default: '" << default_workload_driver_rain_path << "']." << ::std::endl
				<< " --wkl-driver-ycsb-path <name>" << ::std::endl
				<< "   The full path to the YCSB workload driver." << ::std::endl
				<< "   [default: '" << default_workload_driver_ycsb_path << "']." << ::std::endl
				<< " --wkl-ycsb-prop-path <name>" << ::std::endl
				<< "   The full path to a YCSB workload property file." << ::std::endl
				<< "   Repeat this option as many times as is the number of property files you want to use." << ::std::endl
				<< "   [default: '" << default_workload_ycsb_prop_path << "']." << ::std::endl
				<< " --wkl-ycsb-classpath <name>" << ::std::endl
				<< "   The classpath string to pass to the JAVA command when invoking the YCSB workload." << ::std::endl
				<< "   [default: '" << default_workload_ycsb_classpath << "']." << ::std::endl
				<< " --wkl-ycsb-db-class<name>" << ::std::endl
				<< "   The fully-qualified JAVA class of the YCSB database workload." << ::std::endl
				<< "   [default: '" << default_workload_ycsb_db_class << "']." << ::std::endl
				<< ::std::endl;
}

::dcs::testbed::application_performance_category make_slo_metric(std::string const& s)
{
	if (!s.compare("rt") || !s.compare("response-time"))
	{
		return ::dcs::testbed::response_time_application_performance;
	}
	if (!s.compare("tput") || !s.compare("throughput"))
	{
		return ::dcs::testbed::throughput_application_performance;
	}

	DCS_EXCEPTION_THROW(::std::logic_error, "Unknown SLO metric");
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

template <typename RealT>
struct tput_slo_checker
{
	tput_slo_checker(RealT min_val, RealT rel_tol=0.05)
	: min_val_(min_val),
	  check_val_(min_val_*(1+rel_tol))
	{
	}

	bool operator()(RealT val)
	{
		return ::dcs::math::float_traits<RealT>::approximately_greater_equal(val, check_val_);
	}

	private: RealT min_val_;
	private: RealT check_val_;
};

}} // Namespace detail::<unnamed>


int main(int argc, char *argv[])
{
	namespace testbed = ::dcs::testbed;
	//namespace ublas = ::boost::numeric::ublas;

	typedef double real_type;
	typedef unsigned int uint_type;
	typedef testbed::traits<real_type,uint_type> traits_type;

	bool opt_help = false;
//	std::string opt_out_dat_file;
	detail::app_manager_category opt_app_manager;
	std::string opt_app_manager_stats_file;
	real_type opt_brown_single_exponential_alpha;
	real_type opt_brown_double_exponential_alpha;
	real_type opt_chen2000_ewma_quantile_prob;
	real_type opt_chen2000_ewma_w;
	real_type opt_chen2000_ewsa_quantile_prob;
	real_type opt_chen2000_ewsa_w;
	real_type opt_chen2000_sa_quantile_prob;
	detail::data_estimator_category opt_data_estimator;
	detail::data_smoother_category opt_data_smoother;
	real_type opt_holt_winters_double_exponential_alpha;
	real_type opt_holt_winters_double_exponential_beta;
	real_type opt_holt_winters_double_exponential_delta;
//	real_type opt_quantile_prob = 0;
	real_type opt_jain1985_p2_quantile_prob;
	testbed::application_performance_category opt_slo_metric;
	real_type opt_slo_value;
	std::string opt_str;
	real_type opt_ts;
	real_type opt_tc;
	real_type opt_true_quantile_prob;
	bool opt_verbose = detail::default_verbose;
	std::vector<std::string> opt_vm_uris;
	real_type opt_welsh2003_ewma_alpha;
	real_type opt_welsh2003_ewma_quantile_prob;
	testbed::workload_category opt_wkl;
	testbed::workload_generator_category opt_wkl_driver;
	std::string opt_wkl_driver_rain_path;
	std::string opt_wkl_driver_ycsb_path;
	std::vector<std::string> opt_wkl_ycsb_prop_paths;
	std::string opt_wkl_ycsb_classpath;
	std::string opt_wkl_ycsb_db_class;
	bool opt_no_restore_vms = detail::default_no_restore_vms;


	// Parse command line options
	try
	{
		opt_help = dcs::cli::simple::get_option(argv, argv+argc, "--help");
//		opt_out_dat_file = dcs::cli::simple::get_option<std::string>(argv, argv+argc, "--out-dat-file", detail::default_out_dat_file);
		opt_app_manager = dcs::cli::simple::get_option<detail::app_manager_category>(argv, argv+argc, "--app-manager", detail::default_app_manager);
		opt_app_manager_stats_file = dcs::cli::simple::get_option<std::string>(argv, argv+argc, "--app-manager-stats-file");
		opt_data_estimator = dcs::cli::simple::get_option<detail::data_estimator_category>(argv, argv+argc, "--data-estimator", detail::default_data_estimator);
//		opt_quantile_prob = dcs::cli::simple::get_option<real_type>(argv, argv+argc, "--quantile-prob", detail::default_quantile_prob);
		opt_chen2000_ewma_quantile_prob = dcs::cli::simple::get_option<real_type>(argv, argv+argc, "--chen2000_ewma-quantile", detail::default_chen2000_ewma_quantile_prob);
		opt_chen2000_ewma_w = dcs::cli::simple::get_option<real_type>(argv, argv+argc, "--chen2000_ewma-w", detail::default_chen2000_ewma_w);
		opt_chen2000_ewsa_quantile_prob = dcs::cli::simple::get_option<real_type>(argv, argv+argc, "--chen2000_ewsa-quantile", detail::default_chen2000_ewsa_quantile_prob);
		opt_chen2000_ewsa_w = dcs::cli::simple::get_option<real_type>(argv, argv+argc, "--chen2000_ewsa-w", detail::default_chen2000_ewsa_w);
		opt_chen2000_sa_quantile_prob = dcs::cli::simple::get_option<real_type>(argv, argv+argc, "--chen2000_sa-quantile", detail::default_chen2000_sa_quantile_prob);
		opt_jain1985_p2_quantile_prob = dcs::cli::simple::get_option<real_type>(argv, argv+argc, "--jain1985_p2-quantile", detail::default_jain1985_p2_quantile_prob);
		opt_true_quantile_prob = dcs::cli::simple::get_option<real_type>(argv, argv+argc, "--true-quantile", detail::default_true_quantile_prob);
		opt_welsh2003_ewma_alpha = dcs::cli::simple::get_option<real_type>(argv, argv+argc, "--welsh2003_ewma-alpha", detail::default_welsh2003_ewma_alpha);
		opt_welsh2003_ewma_quantile_prob = dcs::cli::simple::get_option<real_type>(argv, argv+argc, "--welsh2003_ewma-quantile", detail::default_welsh2003_ewma_quantile_prob);
		opt_data_smoother = dcs::cli::simple::get_option<detail::data_smoother_category>(argv, argv+argc, "--data-smoother", detail::default_data_smoother);
		opt_brown_single_exponential_alpha = dcs::cli::simple::get_option<real_type>(argv, argv+argc, "--brown_ses-alpha", detail::default_brown_single_exponential_alpha);
		opt_brown_double_exponential_alpha = dcs::cli::simple::get_option<real_type>(argv, argv+argc, "--brown_des-alpha", detail::default_brown_double_exponential_alpha);
		opt_holt_winters_double_exponential_alpha = dcs::cli::simple::get_option<real_type>(argv, argv+argc, "--holt_winters_des-alpha", detail::default_holt_winters_double_exponential_alpha);
		opt_holt_winters_double_exponential_beta = dcs::cli::simple::get_option<real_type>(argv, argv+argc, "--holt_winters_des-beta", detail::default_holt_winters_double_exponential_beta);
		opt_holt_winters_double_exponential_delta = dcs::cli::simple::get_option<real_type>(argv, argv+argc, "--holt_winters_des-delta", detail::default_holt_winters_double_exponential_delta);
		opt_tc = dcs::cli::simple::get_option<real_type>(argv, argv+argc, "--tc", detail::default_control_time);
		opt_ts = dcs::cli::simple::get_option<real_type>(argv, argv+argc, "--ts", detail::default_sampling_time);
		opt_verbose = dcs::cli::simple::get_option(argv, argv+argc, "--verbose");
		opt_vm_uris = dcs::cli::simple::get_options<std::string>(argv, argv+argc, "--vm-uri");
		opt_wkl = dcs::cli::simple::get_option<testbed::workload_category>(argv, argv+argc, "--wkl", detail::default_workload);
		opt_wkl_driver = dcs::cli::simple::get_option<testbed::workload_generator_category>(argv, argv+argc, "--wkl-driver", detail::default_workload_driver);
		opt_wkl_driver_rain_path = dcs::cli::simple::get_option<std::string>(argv, argv+argc, "--wkl-driver-rain-path", detail::default_workload_driver_rain_path);
		opt_wkl_driver_ycsb_path = dcs::cli::simple::get_option<std::string>(argv, argv+argc, "--wkl-driver-ycsb-path", detail::default_workload_driver_ycsb_path);
		opt_wkl_ycsb_classpath = dcs::cli::simple::get_option<std::string>(argv, argv+argc, "--wkl-ycsb-classpath", detail::default_workload_ycsb_classpath);
		opt_wkl_ycsb_db_class = dcs::cli::simple::get_option<std::string>(argv, argv+argc, "--wkl-ycsb-db-class", detail::default_workload_ycsb_db_class);
		opt_wkl_ycsb_prop_paths = dcs::cli::simple::get_options<std::string>(argv, argv+argc, "--wkl-ycsb-prop-path", detail::default_workload_ycsb_prop_path);
		opt_str = dcs::cli::simple::get_option<std::string>(argv, argv+argc, "--slo-metric", detail::default_slo_metric_str);
		opt_slo_metric = detail::make_slo_metric(opt_str);
		opt_slo_value = dcs::cli::simple::get_option<real_type>(argv, argv+argc, "--slo-value", detail::default_slo_value);
		opt_no_restore_vms = dcs::cli::simple::get_option(argv, argv+argc, "--no-restore-vms");
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

		oss << "Application manager: " << opt_app_manager;
		dcs::log_info(DCS_LOGGING_AT, oss.str());
		oss.str("");

		oss << "Application manager output stats file: " << opt_app_manager_stats_file;
		dcs::log_info(DCS_LOGGING_AT, oss.str());
		oss.str("");

		oss << "Data estimator: " << opt_data_estimator;
		dcs::log_info(DCS_LOGGING_AT, oss.str());
		oss.str("");

//		oss << "Quantile estimator probability: " << opt_quantile_prob;
//		dcs::log_info(DCS_LOGGING_AT, oss.str());
//		oss.str("");

		oss << "(Chen et al.,2000)'s EWMA quantile estimator probability: " << opt_chen2000_ewma_quantile_prob;
		dcs::log_info(DCS_LOGGING_AT, oss.str());
		oss.str("");

		oss << "(Chen et al.,2000)'s EWMA quantile estimator w: " << opt_chen2000_ewma_w;
		dcs::log_info(DCS_LOGGING_AT, oss.str());
		oss.str("");

		oss << "(Chen et al.,2000)'s EWSA quantile estimator probability: " << opt_chen2000_ewsa_quantile_prob;
		dcs::log_info(DCS_LOGGING_AT, oss.str());
		oss.str("");

		oss << "(Chen et al.,2000)'s EWSA quantile estimator w: " << opt_chen2000_ewsa_w;
		dcs::log_info(DCS_LOGGING_AT, oss.str());
		oss.str("");

		oss << "(Chen et al.,2000)'s SA quantile estimator probability: " << opt_chen2000_sa_quantile_prob;
		dcs::log_info(DCS_LOGGING_AT, oss.str());
		oss.str("");

		oss << "(Jain et al.,1985)'s P^2 quantile estimator probability: " << opt_jain1985_p2_quantile_prob;
		dcs::log_info(DCS_LOGGING_AT, oss.str());
		oss.str("");

		oss << "True quantile estimator probability: " << opt_true_quantile_prob;
		dcs::log_info(DCS_LOGGING_AT, oss.str());
		oss.str("");

		oss << "(Welsh et al.,2003)'s EWMA quantile estimator alpha: " << opt_welsh2003_ewma_alpha;
		dcs::log_info(DCS_LOGGING_AT, oss.str());
		oss.str("");

		oss << "(Welsh et al.,2003)'s EWMA quantile estimator probability: " << opt_welsh2003_ewma_quantile_prob;
		dcs::log_info(DCS_LOGGING_AT, oss.str());
		oss.str("");

		oss << "Data smoother: " << opt_data_smoother;
		dcs::log_info(DCS_LOGGING_AT, oss.str());
		oss.str("");

		oss << "Brown's single exponential smoother alpha: " << opt_brown_single_exponential_alpha;
		dcs::log_info(DCS_LOGGING_AT, oss.str());
		oss.str("");

		oss << "Brown's double exponential smoother alpha: " << opt_brown_double_exponential_alpha;
		dcs::log_info(DCS_LOGGING_AT, oss.str());
		oss.str("");

		oss << "Holt-Winters' double exponential smoother alpha: " << opt_holt_winters_double_exponential_alpha;
		dcs::log_info(DCS_LOGGING_AT, oss.str());
		oss.str("");

		oss << "Holt-Winters' double exponential smoother beta: " << opt_holt_winters_double_exponential_beta;
		dcs::log_info(DCS_LOGGING_AT, oss.str());
		oss.str("");

		oss << "Holt-Winters' double exponential smoother delta: " << opt_holt_winters_double_exponential_delta;
		dcs::log_info(DCS_LOGGING_AT, oss.str());
		oss.str("");

		oss << "Control time: " << opt_tc;
		dcs::log_info(DCS_LOGGING_AT, oss.str());
		oss.str("");

		oss << "Sampling time: " << opt_ts;
		dcs::log_info(DCS_LOGGING_AT, oss.str());
		oss.str("");

		oss << "Don't restore VMs resource allocations: " << std::boolalpha << opt_no_restore_vms;
		dcs::log_info(DCS_LOGGING_AT, oss.str());
		oss.str("");

		oss << "SLO metric: " << opt_slo_metric;
		dcs::log_info(DCS_LOGGING_AT, oss.str());
		oss.str("");

		oss << "SLO value: " << opt_slo_value;
		dcs::log_info(DCS_LOGGING_AT, oss.str());
		oss.str("");

		oss << "Verbose output: " << std::boolalpha << opt_verbose;
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

		oss << "Workload driver YCSB path: " << opt_wkl_driver_ycsb_path;
		dcs::log_info(DCS_LOGGING_AT, oss.str());
		oss.str("");

		oss << "Workload YCSB JAVA classpath: " << opt_wkl_ycsb_classpath;
		dcs::log_info(DCS_LOGGING_AT, oss.str());
		oss.str("");

		oss << "Workload YCSB DB JAVA class: " << opt_wkl_ycsb_db_class;
		dcs::log_info(DCS_LOGGING_AT, oss.str());
		oss.str("");

		for (std::size_t i = 0; i < opt_wkl_ycsb_prop_paths.size(); ++i)
		{
			if (i > 0)
			{
				oss << ", ";
			}
			oss << "Workload YCSB property file: " << opt_wkl_ycsb_prop_paths[i];
		}
		dcs::log_info(DCS_LOGGING_AT, oss.str());
		oss.str("");

		oss << "Workload driver YCSB path: " << opt_wkl_driver_ycsb_path;
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
		switch (opt_slo_metric)
		{
			case testbed::response_time_application_performance:
				p_app->slo(testbed::response_time_application_performance, detail::rt_slo_checker<real_type>(opt_slo_value));
				break;
			case testbed::throughput_application_performance:
				p_app->slo(testbed::throughput_application_performance, detail::tput_slo_checker<real_type>(opt_slo_value));
				break;
		}

		// - Setup workload driver
		app_driver_pointer p_drv;
		switch (opt_wkl_driver)
		{
			case testbed::rain_workload_generator:
				{
					boost::shared_ptr< testbed::rain::workload_driver<traits_type> > p_drv_impl = boost::make_shared< testbed::rain::workload_driver<traits_type> >(opt_wkl, opt_wkl_driver_rain_path);
					p_app->register_sensor(opt_slo_metric, p_drv_impl->sensor(opt_slo_metric));
					//p_drv = boost::make_shared< testbed::rain::workload_driver<traits_type> >(drv_impl);
					p_drv = p_drv_impl;
				}
				break;
			case testbed::ycsb_workload_generator:
				{
					boost::shared_ptr< testbed::ycsb::workload_driver<traits_type> > p_drv_impl = boost::make_shared< testbed::ycsb::workload_driver<traits_type> >(opt_wkl,
																																									opt_wkl_ycsb_prop_paths.begin(),
																																									opt_wkl_ycsb_prop_paths.end(),
																																									opt_wkl_driver_ycsb_path,
																																									opt_wkl_ycsb_db_class,
																																									opt_wkl_ycsb_classpath);
					p_app->register_sensor(opt_slo_metric, p_drv_impl->sensor(opt_slo_metric));
					//p_drv = boost::make_shared< testbed::ycsb::workload_driver<traits_type> >(drv_impl);
					p_drv = p_drv_impl;
				}
				break;
		}
		p_drv->app(p_app);

		// - Setup data estimator
		boost::shared_ptr< testbed::base_estimator<real_type> > p_estimator;
		switch (opt_data_estimator)
		{
			case detail::chen2000_ewma_quantile_estimator:
					p_estimator = boost::make_shared< testbed::chen2000_ewma_quantile_estimator<real_type> >(opt_chen2000_ewma_quantile_prob, opt_chen2000_ewma_w);
					break;
			case detail::chen2000_ewsa_quantile_estimator:
					p_estimator = boost::make_shared< testbed::chen2000_ewsa_quantile_estimator<real_type> >(opt_chen2000_ewsa_quantile_prob, opt_chen2000_ewsa_w);
					break;
			case detail::chen2000_sa_quantile_estimator:
					p_estimator = boost::make_shared< testbed::chen2000_sa_quantile_estimator<real_type> >(opt_chen2000_sa_quantile_prob);
					break;
			case detail::jain1985_p2_algorithm_quantile_estimator:
					p_estimator = boost::make_shared< testbed::jain1985_p2_algorithm_quantile_estimator<real_type> >(opt_jain1985_p2_quantile_prob);
					break;
			case detail::mean_estimator:
				p_estimator = boost::make_shared< testbed::mean_estimator<real_type> >();
				break;
			case detail::most_recently_observed_estimator:
				p_estimator = boost::make_shared< testbed::most_recently_observed_estimator<real_type> >();
				break;
			case detail::true_quantile_estimator:
					p_estimator = boost::make_shared< testbed::true_quantile_estimator<real_type> >(opt_true_quantile_prob);
					break;
			case detail::welsh2003_ewma_quantile_estimator:
					p_estimator = boost::make_shared< testbed::welsh2003_ewma_quantile_estimator<real_type> >(opt_welsh2003_ewma_quantile_prob, opt_welsh2003_ewma_alpha, false);
					break;
			case detail::welsh2003_ewma_ext_quantile_estimator:
					p_estimator = boost::make_shared< testbed::welsh2003_ewma_quantile_estimator<real_type> >(opt_welsh2003_ewma_quantile_prob, opt_welsh2003_ewma_alpha, true);
					break;
			default:
				DCS_EXCEPTION_THROW(std::runtime_error, "Unknown data estimator");
		}

		// - Setup data smoother
		boost::shared_ptr< testbed::base_smoother<real_type> > p_smoother;
		switch (opt_data_smoother)
		{
			case detail::brown_single_exponential_smoother:
				p_smoother = boost::make_shared< testbed::brown_single_exponential_smoother<real_type> >(opt_brown_single_exponential_alpha);
				break;
			case detail::brown_double_exponential_smoother:
				p_smoother = boost::make_shared< testbed::brown_double_exponential_smoother<real_type> >(opt_brown_double_exponential_alpha);
				break;
			case detail::dummy_smoother:
				p_smoother = boost::make_shared< testbed::dummy_smoother<real_type> >();
				break;
			case detail::holt_winters_double_exponential_smoother:
				if (opt_holt_winters_double_exponential_delta > 0)
				{
					p_smoother = boost::make_shared< testbed::holt_winters_double_exponential_smoother<real_type> >(opt_holt_winters_double_exponential_delta);
				}
				else
				{
					p_smoother = boost::make_shared< testbed::holt_winters_double_exponential_smoother<real_type> >(opt_holt_winters_double_exponential_alpha, opt_holt_winters_double_exponential_beta);
				}
				break;
			default:
				DCS_EXCEPTION_THROW(std::runtime_error, "Unknown data smoother");
		}

		// - Setup application manager
		app_manager_pointer p_mgr;
		//p_mgr = boost::make_shared< testbed::lqry_application_manager<traits_type> >();
		switch (opt_app_manager)
		{
			case detail::albano2013_fuzzyqe_app_manager:
				{
					const real_type beta = 0.9;

					testbed::albano2013_fuzzyqe_application_manager<traits_type> albano2013_fuzzyqe_mgr;
					albano2013_fuzzyqe_mgr.smoothing_factor(beta);
					if (!opt_app_manager_stats_file.empty())
					{
						albano2013_fuzzyqe_mgr.export_data_to(opt_app_manager_stats_file);
					}

					p_mgr = boost::make_shared< testbed::albano2013_fuzzyqe_application_manager<traits_type> >(albano2013_fuzzyqe_mgr);
				}
				break;
			case detail::anglano2014_fc2q_app_manager:
				{
					const real_type beta = 0.9;

					testbed::anglano2014_fc2q_application_manager<traits_type> anglano2014_fc2q_mgr;
					anglano2014_fc2q_mgr.smoothing_factor(beta);
					if (!opt_app_manager_stats_file.empty())
					{
						anglano2014_fc2q_mgr.export_data_to(opt_app_manager_stats_file);
					}

					p_mgr = boost::make_shared< testbed::anglano2014_fc2q_application_manager<traits_type> >(anglano2014_fc2q_mgr);
				}
				break;
//TODO
#if 0
			case detail::guazzone2012_app_manager:
				{
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
					//lqry_mgr.target_value(testbed::response_time_application_performance, opt_slo_value);

					p_mgr = boost::make_shared< testbed::lqry_application_manager<traits_type> >(lqry_mgr);
				}
				break;
#endif // 0
			case detail::dummy_app_manager:
				{
					testbed::dummy_application_manager<traits_type> dummy_mgr;
					if (!opt_app_manager_stats_file.empty())
					{
						dummy_mgr.export_data_to(opt_app_manager_stats_file);
					}

					p_mgr = boost::make_shared< testbed::dummy_application_manager<traits_type> >(dummy_mgr);
				}
				break;
			case detail::padala2009_autocontrol_app_manager:
				{
					const std::size_t na(2);
					const std::size_t nb(2);
					const std::size_t nk(1);
					const std::size_t ny(1);
					const std::size_t nu(nt);
					const real_type ff(0.98);
					const real_type q(2);

					sysid_strategy_pointer p_sysid_alg = boost::make_shared< testbed::rls_ff_arx_miso_proxy<traits_type> >(na, nb, nk, ny, nu, ff);
					testbed::padala2009_autocontrol_application_manager<traits_type> padala2009_autocontrol_mgr;
					padala2009_autocontrol_mgr.sysid_strategy(p_sysid_alg);
					//padala2009_autocontrol_mgr.target_value(testbed::response_time_application_performance, rt_q99*(1-0.20));
					//padala2009_autocontrol_mgr.target_value(testbed::response_time_application_performance, opt_slo_value);
					padala2009_autocontrol_mgr.stability_factor(q);
					if (!opt_app_manager_stats_file.empty())
					{
						padala2009_autocontrol_mgr.export_data_to(opt_app_manager_stats_file);
					}

					p_mgr = boost::make_shared< testbed::padala2009_autocontrol_application_manager<traits_type> >(padala2009_autocontrol_mgr);
				}
				break;
			case detail::rao2013_dynaqos_app_manager:
				{
					const real_type gamma = 0.8;

					testbed::rao2013_dynaqos_application_manager<traits_type> rao2013_dynaqos_mgr;
					rao2013_dynaqos_mgr.discount_factor(gamma);
					if (!opt_app_manager_stats_file.empty())
					{
						rao2013_dynaqos_mgr.export_data_to(opt_app_manager_stats_file);
					}

					p_mgr = boost::make_shared< testbed::rao2013_dynaqos_application_manager<traits_type> >(rao2013_dynaqos_mgr);
				}
				break;
			default:
				DCS_EXCEPTION_THROW(std::runtime_error, "Unknown application manager");
		}
		p_mgr->target_value(opt_slo_metric, opt_slo_value);
		p_mgr->data_estimator(opt_slo_metric, p_estimator);
		p_mgr->data_smoother(opt_slo_metric, p_smoother);
		p_mgr->sampling_time(opt_ts);
		p_mgr->control_time(opt_tc);
		p_mgr->app(p_app);

		// Add to main experiment
		boost::shared_ptr< testbed::application_experiment<traits_type> > p_app_exp;
		p_app_exp = boost::make_shared< testbed::application_experiment<traits_type> >(p_app, p_drv, p_mgr);
		p_app_exp->restore_state(!opt_no_restore_vms);
		sys_exp.add_app_experiment(p_app_exp);

		// Set experiment trackers
		testbed::utility::experiment_stats_gatherer<traits_type> exp_stats;
		exp_stats.track(sys_exp);


		//sys_exp.logger(...);
		//sys_exp.output_data_file(out_dat_file);

		// Run!
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
