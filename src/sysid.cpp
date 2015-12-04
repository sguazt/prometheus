/**
 * \file src/sysid.hpp
 *
 * \brief Driver for performing system identification against an Apache Olio
 *  instance.
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
#include <dcs/logging.hpp>
#include <dcs/testbed/application.hpp>
#include <dcs/testbed/application_experiment.hpp>
#include <dcs/testbed/application_performance_category.hpp>
#include <dcs/testbed/base_application.hpp>
#include <dcs/testbed/base_application_manager.hpp>
#include <dcs/testbed/data_estimators.hpp>
#include <dcs/testbed/data_smoothers.hpp>
#include <dcs/testbed/experiment_stats_gatherer.hpp>
#include <dcs/testbed/io.hpp>
#include <dcs/testbed/signal_generators.hpp>
#include <dcs/testbed/system_experiment.hpp>
#include <dcs/testbed/sysid_application_manager.hpp>
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

enum data_estimator_category
{
	mean_estimator,
	chen2000_ewma_quantile_estimator,
	chen2000_ewsa_quantile_estimator,
	chen2000_sa_quantile_estimator,
	jain1985_p2_algorithm_quantile_estimator,
	most_recently_observed_estimator,
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

const std::string default_cfg_file("config.yaml");
const dcs::testbed::workload_category default_workload = dcs::testbed::olio_workload;
const dcs::testbed::workload_generator_category default_workload_driver = dcs::testbed::rain_workload_generator;
const ::std::string default_workload_driver_rain_path("/usr/local/opt/rain-workload-toolkit");
const ::std::string default_workload_rain_java_xargs("");
const ::std::string default_workload_driver_ycsb_path("/usr/local/opt/YCSB");
const ::std::string default_workload_ycsb_prop_path("workloads/workloada");
const ::std::string default_workload_ycsb_classpath;
const ::std::string default_workload_ycsb_db_class;
const ::std::string default_out_dat_file("./sysid-out.dat");
const unsigned long default_rng_seed = 5489UL;
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
const double default_welsh2003_ewma_alpha = 0.7;
const double default_welsh2003_ewma_quantile_prob = default_quantile_prob;
const data_smoother_category default_data_smoother = dummy_smoother;
const double default_brown_single_exponential_alpha = 0.7;
const double default_brown_double_exponential_alpha = 0.7;
const double default_holt_winters_double_exponential_alpha = 0.8;
const double default_holt_winters_double_exponential_beta = 0.3;
const double default_holt_winters_double_exponential_delta = 0.7;
const signal_category default_signal_category = constant_signal;
const double default_signal_common_upper_bound = std::numeric_limits<double>::infinity();
const double default_signal_common_lower_bound = -std::numeric_limits<double>::infinity();
const double default_signal_const_val = 1;
const double default_signal_sawtooth_low = 0;
const double default_signal_sawtooth_high = 1;
const double default_signal_sawtooth_incr = 0.1;
const double default_signal_sine_amplitude = 0.5;
const unsigned int default_signal_sine_frequency = 8;
const unsigned int default_signal_sine_phase = 0;
const double default_signal_sine_bias = 0.5;
const double default_signal_sine_mesh_amplitude = 0.5;
const unsigned int default_signal_sine_mesh_frequency = 8;
const unsigned int default_signal_sine_mesh_phase = 0;
const double default_signal_sine_mesh_bias = 0.5;
const double default_signal_half_sine_amplitude = 0.5;
const unsigned int default_signal_half_sine_frequency = 8;
const unsigned int default_signal_half_sine_phase = 0;
const double default_signal_half_sine_bias = 0.5;
const double default_signal_half_sine_mesh_amplitude = 0.5;
const unsigned int default_signal_half_sine_mesh_frequency = 8;
const unsigned int default_signal_half_sine_mesh_phase = 0;
const double default_signal_half_sine_mesh_bias = 0.5;
const double default_signal_square_low = 0;
const double default_signal_square_high = 1;
const double default_signal_uniform_min = 0;
const double default_signal_uniform_max = 1;
const double default_signal_gaussian_mean = 0;
const double default_signal_gaussian_sd = 1;
const std::string default_slo_metric_str("rt");
const dcs::testbed::virtual_machine_performance_category default_vm_performance = dcs::testbed::cpu_util_virtual_machine_performance;


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

void usage(char const* progname)
{
	::std::cerr << "Usage: " << progname << " [options]" << ::std::endl
				<< " --help" << ::std::endl
				<< "   Show this message." << ::std::endl
				<< " --data-estimator <name>" << ::std::endl
				<< "   The name of the estimator to use to estimate summary statistics from observed data." << ::std::endl
				<< "   Possible values are:" << ::std::endl
				<< "   - 'chen2000_ewma_quantile': quantile estimation according to the EWMA method by (Chen et al., 2000)" << ::std::endl
				<< "   - 'chen2000_ewsa_quantile': quantile estimation according to the EWSA method by (Chen et al., 2000)" << ::std::endl
				<< "   - 'chen2000_sa_quantile': quantile estimation according to the SA method by (Chen et al., 2000)" << ::std::endl
				<< "   - 'jain1985_p2_algorithm_quantile': quantile estimation according to the P^2 algorithm by (Jain et al., 1985)" << ::std::endl
				<< "   - 'mean': sample mean" << ::std::endl
				<< "   - 'mro': most recently observed data" << ::std::endl
				<< "   - 'welsh2003_ewma_quantile': quantile estimation according to the EWMA method by (Welsh et al., 2003)" << ::std::endl
				<< "   - 'welsh2003_ewma_ext_quantile': quantile estimation according to the extended EWMA method by (Welsh et al., 2003)" << ::std::endl
				<< "   [default: '" << default_data_estimator << "']." << ::std::endl
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
				<< " --welsh2003_ewma-alpha <value>" << ::std::endl
				<< "   The alpha parameter for the (Welsh el al.,2003) EWMA quantile estimator." << ::std::endl
				<< "   [default: '" << default_welsh2003_ewma_alpha << "']." << ::std::endl
				<< " --welsh2003_ewma-quantile <value>" << ::std::endl
				<< "   The probability value for the (Welsh el al.,2003) P^2 quantile estimator." << ::std::endl
				<< "   [default: '" << default_jain1985_p2_quantile_prob << "']." << ::std::endl
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
				<< " --out-dat-file <file path>" << ::std::endl
				<< "   The path to the output data file." << ::std::endl
				<< "   [default: '" << default_out_dat_file << "']" << ::std::endl
				<< " --rng-seed <value>" << ::std::endl
				<< "   The seed for the random number generator." << ::std::endl
				<< "   [default: '" << default_rng_seed << "']" << ::std::endl
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
				<< " --sig-upper-bound <value>" << std::endl
				<< "   The signal upper bound." << std::endl
				<< "   [default: " << default_signal_common_upper_bound << "]." << std::endl
				<< " --sig-lower-bound <value>" << std::endl
				<< "   The signal lower bound." << std::endl
				<< "   [default: " << default_signal_common_lower_bound << "]." << std::endl
				<< " --sig-constant-val <value>" << std::endl
				<< "   The value for the constant signal." << std::endl
				<< "   [default: " << default_signal_const_val << "]." << std::endl
				<< " --sig-sawtooth-low <value>" << std::endl
				<< "   The lower value for the sawtooth signal." << std::endl
				<< "   [default: " << default_signal_sawtooth_low << "]." << std::endl
				<< " --sig-sawtooth-high <value>" << std::endl
				<< "   The higher value for the sawtooth signal." << std::endl
				<< "   [default: " << default_signal_sawtooth_high << "]." << std::endl
				<< " --sig-sawtooth-incr <value>" << std::endl
				<< "   The increment value for the sawtooth signal." << std::endl
				<< "   [default: " << default_signal_sawtooth_incr << "]." << std::endl
				<< " --sig-sine-ampl <value>" << std::endl
				<< "   The amplitude value for the sinusoidal signal." << std::endl
				<< "   [default: " << default_signal_sine_amplitude << "]." << std::endl
				<< " --sig-sine-freq <value>" << std::endl
				<< "   The frequency value for the sinusoidal signal." << std::endl
				<< "   [default: " << default_signal_sine_frequency << "]." << std::endl
				<< " --sig-sine-phase <value>" << std::endl
				<< "   The phase value for the sinusoidal signal." << std::endl
				<< "   [default: " << default_signal_sine_phase << "]." << std::endl
				<< " --sig-sine-bias <value>" << std::endl
				<< "   The bias (offset) value for the sinusoidal signal." << std::endl
				<< "   [default: " << default_signal_sine_bias << "]." << std::endl
				<< " --sig-sine-mesh-ampl <value>" << std::endl
				<< "   The amplitude value for the mesh sinusoidal signal." << std::endl
				<< "   [default: " << default_signal_sine_mesh_amplitude << "]." << std::endl
				<< " --sig-sine-mesh-freq <value>" << std::endl
				<< "   The frequency value for the mesh sinusoidal signal." << std::endl
				<< "   [default: " << default_signal_sine_mesh_frequency << "]." << std::endl
				<< " --sig-sine-mesh-phase <value>" << std::endl
				<< "   The phase value for the mesh sinusoidal signal." << std::endl
				<< "   [default: " << default_signal_sine_mesh_phase << "]." << std::endl
				<< " --sig-sine-mesh-bias <value>" << std::endl
				<< "   The bias (offset) value for the mesh sinusoidal signal." << std::endl
				<< "   [default: " << default_signal_sine_mesh_bias << "]." << std::endl
				<< " --sig-half-sine-ampl <value>" << std::endl
				<< "   The amplitude value for the half-sinusoidal signal." << std::endl
				<< "   [default: " << default_signal_half_sine_amplitude << "]." << std::endl
				<< " --sig-half-sine-freq <value>" << std::endl
				<< "   The frequency value for the half-sinusoidal signal." << std::endl
				<< "   [default: " << default_signal_half_sine_frequency << "]." << std::endl
				<< " --sig-half-sine-phase <value>" << std::endl
				<< "   The phase value for the half-sinusoidal signal." << std::endl
				<< "   [default: " << default_signal_half_sine_phase << "]." << std::endl
				<< " --sig-half-sine-bias <value>" << std::endl
				<< "   The bias (offset) value for the half-sinusoidal signal." << std::endl
				<< "   [default: " << default_signal_half_sine_bias << "]." << std::endl
				<< " --sig-half-sine-mesh-ampl <value>" << std::endl
				<< "   The amplitude value for the mesh half-sinusoidal signal." << std::endl
				<< "   [default: " << default_signal_half_sine_mesh_amplitude << "]." << std::endl
				<< " --sig-half-sine-mesh-freq <value>" << std::endl
				<< "   The frequency value for the mesh half-sinusoidal signal." << std::endl
				<< "   [default: " << default_signal_half_sine_mesh_frequency << "]." << std::endl
				<< " --sig-half-sine-mesh-phase <value>" << std::endl
				<< "   The phase value for the mesh half-sinusoidal signal." << std::endl
				<< "   [default: " << default_signal_half_sine_mesh_phase << "]." << std::endl
				<< " --sig-half-sine-mesh-bias <value>" << std::endl
				<< "   The bias (offset) value for the mesh half-sinusoidal signal." << std::endl
				<< "   [default: " << default_signal_half_sine_mesh_bias << "]." << std::endl
				<< " --sig-square-low <value>" << std::endl
				<< "   The lower value for the square signal." << std::endl
				<< "   [default: " << default_signal_square_low << "]." << std::endl
				<< " --sig-square-high <value>" << std::endl
				<< "   The higher value for the square signal." << std::endl
				<< "   [default: " << default_signal_square_high << "]." << std::endl
				<< " --sig-uniform-min <value>" << std::endl
				<< "   The minimum value for the uniform signal." << std::endl
				<< "   [default: " << default_signal_uniform_min << "]." << std::endl
				<< " --sig-uniform-max <value>" << std::endl
				<< "   The maximum value for the uniform signal." << std::endl
				<< "   [default: " << default_signal_uniform_max << "]." << std::endl
				<< " --sig-gaussian-mean <value>" << std::endl
				<< "   The mean value for the Gaussian signal." << std::endl
				<< "   [default: " << default_signal_gaussian_mean << "]." << std::endl
				<< " --sig-gaussian-sd <value>" << std::endl
				<< "   The standard deviation value for the Gaussian signal." << std::endl
				<< "   [default: " << default_signal_gaussian_sd << "]." << std::endl
				<< " --slo-metric <name>" << ::std::endl
				<< "   The SLO metric. Possible values are:" << std::endl
				<< "   - 'rt': response time," << ::std::endl
				<< "   - 'tput': throughput" << ::std::endl
				<< "   [default: '" << default_slo_metric_str << "']." << ::std::endl
				<< " --ts <time in secs>" << ::std::endl
				<< "   Sampling time (in seconds)." << ::std::endl
				<< "   [default: " << default_sampling_time << "]." << ::std::endl
				<< " --verbose" << ::std::endl
				<< "   Show verbose messages." << ::std::endl
				<< "   [default: disabled]." << ::std::endl
				<< " --vm-perf <category>" << ::std::endl
				<< "   The performance category to monitor from VMs." << ::std::endl
				<< "   Possible values:" << std::endl
				<< "   - 'cpu-util': CPU utilization," << std::endl
				<< "   - 'mem-util': memory utilization." << ::std::endl
				<< "   To specifiy more than one category, repeat the option multiple times." << std::endl
				<< "   [default: '" << default_vm_performance << "']." << ::std::endl
				<< " --vm-uri <URI>" << ::std::endl
				<< "   The URI used to connect to a VM." << ::std::endl
				<< "   To specifiy more than one category, repeat the option multiple times." << std::endl
				<< " --wkl <name>" << ::std::endl
				<< "   The workload to generate." << std::endl
				<< "   Possible values are:" << std::endl
				<< "   -'cassandra'," << std::endl
				<< "   - 'olio'," << std::endl
				<< "   - 'redis'." << ::std::endl
				<< "   - 'rubbos'." << ::std::endl
				<< "   - 'rubis'." << ::std::endl
				<< "   [default: '" << default_workload << "']." << ::std::endl
				<< " --wkl-driver <name>" << ::std::endl
				<< "   The workload driver to use. Possible values are: 'rain'." << ::std::endl
				<< "   [default: '" << default_workload_driver << "']." << ::std::endl
				<< " --wkl-driver-rain-path <name>" << ::std::endl
				<< "   The full path to the RAIN workload driver." << ::std::endl
				<< "   [default: '" << default_workload_driver_rain_path << "']." << ::std::endl
				<< " --wkl-driver-ycsb-path <name>" << ::std::endl
				<< "   The full path to the YCSB workload driver." << ::std::endl
				<< "   [default: '" << default_workload_driver_ycsb_path << "']." << ::std::endl
				<< " --wkl-rain-java-xargs <argument>" << ::std::endl
				<< "   The argument to pass to the java command." << ::std::endl
				<< "   Repeat this option as many times as is the number of argument you want to specify." << ::std::endl
				<< "   [default: '" << default_workload_rain_java_xargs << "']." << ::std::endl
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

}} // Namespace detail::<unnamed>

#if 1
int main(int argc, char *argv[])
{
	namespace testbed = ::dcs::testbed;

	typedef double real_type;
	typedef unsigned int uint_type;
	typedef testbed::traits<real_type,uint_type> traits_type;

	bool opt_help = false;
	std::vector<std::string> opt_vm_uris;
	detail::data_estimator_category opt_data_estimator;
	real_type opt_brown_single_exponential_alpha;
	real_type opt_brown_double_exponential_alpha;
	real_type opt_chen2000_ewma_quantile_prob;
	real_type opt_chen2000_ewma_w;
	real_type opt_chen2000_ewsa_quantile_prob;
	real_type opt_chen2000_ewsa_w;
	real_type opt_chen2000_sa_quantile_prob;
	real_type opt_jain1985_p2_quantile_prob;
	real_type opt_welsh2003_ewma_alpha;
	real_type opt_welsh2003_ewma_quantile_prob;
	detail::data_smoother_category opt_data_smoother;
	real_type opt_holt_winters_double_exponential_alpha;
	real_type opt_holt_winters_double_exponential_beta;
	real_type opt_holt_winters_double_exponential_delta;
	std::string opt_out_dat_file;
	uint_type opt_rng_seed;
	detail::signal_category opt_sig;
	real_type opt_sig_common_up_bound;
	real_type opt_sig_common_lo_bound;
	real_type opt_sig_const_val;
	real_type opt_sig_gauss_mean;
	real_type opt_sig_gauss_sd;
	real_type opt_sig_half_sine_ampl;
	uint_type opt_sig_half_sine_freq;
	uint_type opt_sig_half_sine_phase;
	real_type opt_sig_half_sine_bias;
	real_type opt_sig_half_sine_mesh_ampl;
	uint_type opt_sig_half_sine_mesh_freq;
	uint_type opt_sig_half_sine_mesh_phase;
	real_type opt_sig_half_sine_mesh_bias;
	real_type opt_sig_sawtooth_low;
	real_type opt_sig_sawtooth_high;
	real_type opt_sig_sawtooth_incr;
	real_type opt_sig_sine_ampl;
	uint_type opt_sig_sine_freq;
	uint_type opt_sig_sine_phase;
	real_type opt_sig_sine_bias;
	real_type opt_sig_sine_mesh_ampl;
	uint_type opt_sig_sine_mesh_freq;
	uint_type opt_sig_sine_mesh_phase;
	real_type opt_sig_sine_mesh_bias;
	real_type opt_sig_square_low;
	real_type opt_sig_square_high;
	real_type opt_sig_unif_min;
	real_type opt_sig_unif_max;
	std::vector<testbed::virtual_machine_performance_category> opt_vm_perfs;
	testbed::application_performance_category opt_slo_metric;
	std::string opt_str;
	real_type opt_tc;
	real_type opt_ts;
	bool opt_verbose = false;
	testbed::workload_category opt_wkl;
	testbed::workload_generator_category opt_wkl_driver;
	std::string opt_wkl_driver_rain_path;
	std::vector<std::string> opt_wkl_rain_java_xargs;
	std::string opt_wkl_driver_ycsb_path;
	std::vector<std::string> opt_wkl_ycsb_prop_paths;
	std::string opt_wkl_ycsb_classpath;
	std::string opt_wkl_ycsb_db_class;


	// Parse command line options
	try
	{
		opt_help = dcs::cli::simple::get_option(argv, argv+argc, "--help");
		opt_data_estimator = dcs::cli::simple::get_option<detail::data_estimator_category>(argv, argv+argc, "--data-estimator", detail::default_data_estimator);
		opt_chen2000_ewma_quantile_prob = dcs::cli::simple::get_option<real_type>(argv, argv+argc, "--chen2000_ewma-quantile", detail::default_chen2000_ewma_quantile_prob);
		opt_chen2000_ewma_w = dcs::cli::simple::get_option<real_type>(argv, argv+argc, "--chen2000_ewma-w", detail::default_chen2000_ewma_w);
		opt_chen2000_ewsa_quantile_prob = dcs::cli::simple::get_option<real_type>(argv, argv+argc, "--chen2000_ewsa-quantile", detail::default_chen2000_ewsa_quantile_prob);
		opt_chen2000_ewsa_w = dcs::cli::simple::get_option<real_type>(argv, argv+argc, "--chen2000_ewsa-w", detail::default_chen2000_ewsa_w);
		opt_chen2000_sa_quantile_prob = dcs::cli::simple::get_option<real_type>(argv, argv+argc, "--chen2000_sa-quantile", detail::default_chen2000_sa_quantile_prob);
		opt_jain1985_p2_quantile_prob = dcs::cli::simple::get_option<real_type>(argv, argv+argc, "--jain1985_p2-quantile", detail::default_jain1985_p2_quantile_prob);
		opt_welsh2003_ewma_alpha = dcs::cli::simple::get_option<real_type>(argv, argv+argc, "--welsh2003_ewma-alpha", detail::default_welsh2003_ewma_alpha);
		opt_welsh2003_ewma_quantile_prob = dcs::cli::simple::get_option<real_type>(argv, argv+argc, "--welsh2003_ewma-quantile", detail::default_welsh2003_ewma_quantile_prob);
		opt_data_smoother = dcs::cli::simple::get_option<detail::data_smoother_category>(argv, argv+argc, "--data-smoother", detail::default_data_smoother);
		opt_brown_single_exponential_alpha = dcs::cli::simple::get_option<real_type>(argv, argv+argc, "--brown_ses-alpha", detail::default_brown_single_exponential_alpha);
		opt_brown_double_exponential_alpha = dcs::cli::simple::get_option<real_type>(argv, argv+argc, "--brown_des-alpha", detail::default_brown_double_exponential_alpha);
		opt_holt_winters_double_exponential_alpha = dcs::cli::simple::get_option<real_type>(argv, argv+argc, "--holt_winters_des-alpha", detail::default_holt_winters_double_exponential_alpha);
		opt_holt_winters_double_exponential_beta = dcs::cli::simple::get_option<real_type>(argv, argv+argc, "--holt_winters_des-beta", detail::default_holt_winters_double_exponential_beta);
		opt_holt_winters_double_exponential_delta = dcs::cli::simple::get_option<real_type>(argv, argv+argc, "--holt_winters_des-delta", detail::default_holt_winters_double_exponential_delta);
		opt_out_dat_file = dcs::cli::simple::get_option<std::string>(argv, argv+argc, "--out-dat-file", detail::default_out_dat_file);
		opt_rng_seed = dcs::cli::simple::get_option<uint_type>(argv, argv+argc, "--rng-seed", detail::default_rng_seed);
		opt_sig = dcs::cli::simple::get_option<detail::signal_category>(argv, argv+argc, "--sig", detail::default_signal_category);
		opt_sig_common_up_bound = dcs::cli::simple::get_option<real_type>(argv, argv+argc, "--sig-upper-bound", detail::default_signal_common_upper_bound);
		opt_sig_common_lo_bound = dcs::cli::simple::get_option<real_type>(argv, argv+argc, "--sig-lower-bound", detail::default_signal_common_lower_bound);
		opt_sig_const_val = dcs::cli::simple::get_option<real_type>(argv, argv+argc, "--sig-constant-val", detail::default_signal_const_val);
		opt_sig_sawtooth_low = dcs::cli::simple::get_option<real_type>(argv, argv+argc, "--sig-sawtooth-low", detail::default_signal_sawtooth_low);
		opt_sig_sawtooth_high = dcs::cli::simple::get_option<real_type>(argv, argv+argc, "--sig-sawtooth-high", detail::default_signal_sawtooth_high);
		opt_sig_sawtooth_incr = dcs::cli::simple::get_option<real_type>(argv, argv+argc, "--sig-sawtooth-incr", detail::default_signal_sawtooth_incr);
		opt_sig_sine_ampl = dcs::cli::simple::get_option<real_type>(argv, argv+argc, "--sig-sine-ampl", detail::default_signal_sine_amplitude);
		opt_sig_sine_freq = dcs::cli::simple::get_option<uint_type>(argv, argv+argc, "--sig-sine-freq", detail::default_signal_sine_frequency);
		opt_sig_sine_phase = dcs::cli::simple::get_option<uint_type>(argv, argv+argc, "--sig-sine-phase", detail::default_signal_sine_phase);
		opt_sig_sine_bias = dcs::cli::simple::get_option<real_type>(argv, argv+argc, "--sig-sine-bias", detail::default_signal_sine_bias);
		opt_sig_sine_mesh_ampl = dcs::cli::simple::get_option<real_type>(argv, argv+argc, "--sig-sine-mesh-ampl", detail::default_signal_sine_mesh_amplitude);
		opt_sig_sine_mesh_freq = dcs::cli::simple::get_option<uint_type>(argv, argv+argc, "--sig-sine-mesh-freq", detail::default_signal_sine_mesh_frequency);
		opt_sig_sine_mesh_phase = dcs::cli::simple::get_option<uint_type>(argv, argv+argc, "--sig-sine-mesh-phase", detail::default_signal_sine_mesh_phase);
		opt_sig_sine_mesh_bias = dcs::cli::simple::get_option<real_type>(argv, argv+argc, "--sig-sine-mesh-bias", detail::default_signal_sine_mesh_bias);
		opt_sig_half_sine_ampl = dcs::cli::simple::get_option<real_type>(argv, argv+argc, "--sig-half-sine-ampl", detail::default_signal_half_sine_amplitude);
		opt_sig_half_sine_freq = dcs::cli::simple::get_option<uint_type>(argv, argv+argc, "--sig-half-sine-freq", detail::default_signal_half_sine_frequency);
		opt_sig_half_sine_phase = dcs::cli::simple::get_option<uint_type>(argv, argv+argc, "--sig-half-sine-phase", detail::default_signal_half_sine_phase);
		opt_sig_half_sine_bias = dcs::cli::simple::get_option<real_type>(argv, argv+argc, "--sig-half-sine-bias", detail::default_signal_half_sine_bias);
		opt_sig_half_sine_mesh_ampl = dcs::cli::simple::get_option<real_type>(argv, argv+argc, "--sig-half-sine-mesh-ampl", detail::default_signal_half_sine_mesh_amplitude);
		opt_sig_half_sine_mesh_freq = dcs::cli::simple::get_option<uint_type>(argv, argv+argc, "--sig-half-sine-mesh-freq", detail::default_signal_half_sine_mesh_frequency);
		opt_sig_half_sine_mesh_phase = dcs::cli::simple::get_option<uint_type>(argv, argv+argc, "--sig-half-sine-mesh-phase", detail::default_signal_half_sine_mesh_phase);
		opt_sig_half_sine_mesh_bias = dcs::cli::simple::get_option<real_type>(argv, argv+argc, "--sig-half-sine-mesh-bias", detail::default_signal_half_sine_mesh_bias);
		opt_sig_square_low = dcs::cli::simple::get_option<real_type>(argv, argv+argc, "--sig-square-low", detail::default_signal_square_low);
		opt_sig_square_high = dcs::cli::simple::get_option<real_type>(argv, argv+argc, "--sig-square-high", detail::default_signal_square_high);
		opt_sig_unif_min = dcs::cli::simple::get_option<real_type>(argv, argv+argc, "--sig-uniform-min", detail::default_signal_uniform_min);
		opt_sig_unif_max = dcs::cli::simple::get_option<real_type>(argv, argv+argc, "--sig-uniform-max", detail::default_signal_uniform_max);
		opt_sig_gauss_mean = dcs::cli::simple::get_option<real_type>(argv, argv+argc, "--sig-gaussian-mean", detail::default_signal_gaussian_mean);
		opt_sig_gauss_sd = dcs::cli::simple::get_option<real_type>(argv, argv+argc, "--sig-gaussian-sd", detail::default_signal_gaussian_sd);
		opt_tc = dcs::cli::simple::get_option<real_type>(argv, argv+argc, "--tc", detail::default_control_time);
		opt_ts = dcs::cli::simple::get_option<real_type>(argv, argv+argc, "--ts", detail::default_sampling_time);
		opt_verbose = dcs::cli::simple::get_option(argv, argv+argc, "--verbose");
		opt_vm_perfs = dcs::cli::simple::get_options<testbed::virtual_machine_performance_category>(argv, argv+argc, "--vm-perf");
		if (opt_vm_perfs.size() == 0)
		{
			opt_vm_perfs.push_back(detail::default_vm_performance);
		}
		opt_vm_uris = dcs::cli::simple::get_options<std::string>(argv, argv+argc, "--vm-uri");
		opt_wkl = dcs::cli::simple::get_option<testbed::workload_category>(argv, argv+argc, "--wkl", detail::default_workload);
		opt_wkl_driver = dcs::cli::simple::get_option<testbed::workload_generator_category>(argv, argv+argc, "--wkl-driver", detail::default_workload_driver);
		opt_wkl_driver_rain_path = dcs::cli::simple::get_option<std::string>(argv, argv+argc, "--wkl-driver-rain-path", detail::default_workload_driver_rain_path);
		opt_wkl_rain_java_xargs = dcs::cli::simple::get_options<std::string>(argv, argv+argc, "--wkl-rain-java-xargs", detail::default_workload_rain_java_xargs);
		opt_wkl_driver_ycsb_path = dcs::cli::simple::get_option<std::string>(argv, argv+argc, "--wkl-driver-ycsb-path", detail::default_workload_driver_ycsb_path);
		opt_wkl_ycsb_classpath = dcs::cli::simple::get_option<std::string>(argv, argv+argc, "--wkl-ycsb-classpath", detail::default_workload_ycsb_classpath);
		opt_wkl_ycsb_db_class = dcs::cli::simple::get_option<std::string>(argv, argv+argc, "--wkl-ycsb-db-class", detail::default_workload_ycsb_db_class);
		opt_wkl_ycsb_prop_paths = dcs::cli::simple::get_options<std::string>(argv, argv+argc, "--wkl-ycsb-prop-path", detail::default_workload_ycsb_prop_path);
		opt_str = dcs::cli::simple::get_option<std::string>(argv, argv+argc, "--slo-metric", detail::default_slo_metric_str);
		opt_slo_metric = detail::make_slo_metric(opt_str);
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

	if (opt_help)
	{
		detail::usage(argv[0]);
		return EXIT_SUCCESS;
	}

	int ret = 0;

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

		oss << "Data estimator: " << opt_data_estimator;
		dcs::log_info(DCS_LOGGING_AT, oss.str());
		oss.str("");

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

		oss << "Output data file: " << opt_out_dat_file;
		dcs::log_info(DCS_LOGGING_AT, oss.str());
		oss.str("");

		oss << "Random Number Generator Seed: " << opt_rng_seed;
		dcs::log_info(DCS_LOGGING_AT, oss.str());
		oss.str("");

		oss << "Signal category: " << opt_sig;
		dcs::log_info(DCS_LOGGING_AT, oss.str());
		oss.str("");

		oss << "Signal lower bound: " << opt_sig_common_lo_bound;
		dcs::log_info(DCS_LOGGING_AT, oss.str());
		oss.str("");

		oss << "Signal upper bound: " << opt_sig_common_up_bound;
		dcs::log_info(DCS_LOGGING_AT, oss.str());
		oss.str("");

		switch (opt_sig)
		{
			case detail::constant_signal:
				oss << "Constant signal - value: " << opt_sig_const_val;
				dcs::log_info(DCS_LOGGING_AT, oss.str());
				oss.str("");
				break;
			case detail::gaussian_signal:
				oss << "Gaussian signal -"
					<< "  mean: " << opt_sig_gauss_mean
					<< ", standard deviation: " << opt_sig_gauss_sd;
				dcs::log_info(DCS_LOGGING_AT, oss.str());
				oss.str("");
				break;
			case detail::half_sinusoidal_signal:
				oss << "Half-sinusoidal signal -"
					<< "  amplitude: " << opt_sig_half_sine_ampl
					<< ", frequency: " << opt_sig_half_sine_freq
					<< ", phase: " << opt_sig_half_sine_phase
					<< ", bias: " << opt_sig_half_sine_bias;
				dcs::log_info(DCS_LOGGING_AT, oss.str());
				oss.str("");
				break;
			case detail::half_sinusoidal_mesh_signal:
				oss << "Half-sinusoidal mesh signal -"
					<< "  amplitude: " << opt_sig_half_sine_mesh_ampl
					<< ", frequency: " << opt_sig_half_sine_mesh_freq
					<< ", phase: " << opt_sig_half_sine_mesh_phase
					<< ", bias: " << opt_sig_half_sine_mesh_bias;
				dcs::log_info(DCS_LOGGING_AT, oss.str());
				oss.str("");
				break;
			case detail::sawtooth_signal:
				oss << "Sawtooth signal -"
					<< "  lower value: " << opt_sig_sawtooth_low
					<< ", higher value: " << opt_sig_sawtooth_high
					<< ", increment: " << opt_sig_sawtooth_incr;
				dcs::log_info(DCS_LOGGING_AT, oss.str());
				oss.str("");
				break;
			case detail::sinusoidal_signal:
				oss << "Sinusoidal signal -"
					<< "  amplitude: " << opt_sig_sine_ampl
					<< ", frequency: " << opt_sig_sine_freq
					<< ", phase: " << opt_sig_sine_phase
					<< ", bias: " << opt_sig_sine_bias;
				dcs::log_info(DCS_LOGGING_AT, oss.str());
				oss.str("");
				break;
			case detail::sinusoidal_mesh_signal:
				oss << "Sinusoidal mesh signal -"
					<< "  amplitude: " << opt_sig_sine_mesh_ampl
					<< ", frequency: " << opt_sig_sine_mesh_freq
					<< ", phase: " << opt_sig_sine_mesh_phase
					<< ", bias: " << opt_sig_sine_mesh_bias;
				dcs::log_info(DCS_LOGGING_AT, oss.str());
				oss.str("");
				break;
			case detail::square_signal:
				oss << "Square signal -"
					<< "  lower value: " << opt_sig_square_low
					<< ", higher value: " << opt_sig_square_high;
				dcs::log_info(DCS_LOGGING_AT, oss.str());
				oss.str("");
				break;
			case detail::uniform_signal:
				oss << "Uniform signal -"
					<< "  minimum value: " << opt_sig_unif_min
					<< ", maximum value: " << opt_sig_unif_max;
				dcs::log_info(DCS_LOGGING_AT, oss.str());
				oss.str("");
				break;
			default:
				break;
		}

		oss << "Control time: " << opt_tc;
		dcs::log_info(DCS_LOGGING_AT, oss.str());
		oss.str("");

		oss << "Sampling time: " << opt_ts;
		dcs::log_info(DCS_LOGGING_AT, oss.str());
		oss.str("");

		oss << "SLO metric: " << opt_slo_metric;
		dcs::log_info(DCS_LOGGING_AT, oss.str());
		oss.str("");

		oss << "Verbose output: " << std::boolalpha << opt_verbose;
		dcs::log_info(DCS_LOGGING_AT, oss.str());
		oss.str("");

		for (std::size_t i = 0; i < opt_vm_perfs.size(); ++i)
		{
			if (i > 0)
			{
				oss << ", ";
			}
			oss << "VM performance category: " << opt_vm_perfs[i];
		}
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

		for (std::size_t i = 0; i < opt_wkl_rain_java_xargs.size(); ++i)
		{
			if (i > 0)
			{
				oss << ", ";
			}
			oss << "Workload RAIN Java extra arguments: " << opt_wkl_rain_java_xargs[i];
		}
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
	typedef testbed::base_virtual_machine_manager<traits_type> vmm_type;
	typedef boost::shared_ptr<vmm_type> vmm_pointer;
	typedef vmm_type::identifier_type vmm_identifier_type;
    typedef testbed::base_application<traits_type> app_type;
    typedef boost::shared_ptr<app_type> app_pointer;
	typedef testbed::base_application_manager<traits_type> app_manager_type;
	typedef boost::shared_ptr<app_manager_type> app_manager_pointer;
	typedef testbed::base_workload_driver<traits_type> app_driver_type;
	typedef boost::shared_ptr<app_driver_type> app_driver_pointer;
	typedef boost::random::mt19937 random_generator_type;

	try
	{
		const std::size_t nt(opt_vm_uris.size()); // Number of tiers

		testbed::system_experiment<traits_type> sys_exp;

		// Setup application experiment
		//  - Setup application (and VMs)
		std::map<vmm_identifier_type,vmm_pointer> vmm_map;
		std::vector<vm_pointer> vms;
		const std::vector<std::string>::const_iterator uri_end_it = opt_vm_uris.end();
		for (std::vector<std::string>::const_iterator it = opt_vm_uris.begin();
			 it != uri_end_it;
			 ++it)
		{
			std::string const& uri = *it;

			vmm_pointer p_vmm;
			if (vmm_map.count(uri) > 0)
			{
				p_vmm = vmm_map.at(uri);
			}
			else
			{
				p_vmm = boost::make_shared< testbed::libvirt::virtual_machine_manager<traits_type> >(uri);
				vmm_map[uri] = p_vmm;
			}

			vm_pointer p_vm(p_vmm->vm(uri));

			// check: p_vm != null
			DCS_DEBUG_ASSERT( p_vm );

			vms.push_back(p_vm);
		}
		app_pointer p_app = boost::make_shared< testbed::application<traits_type> >(vms.begin(), vms.end());

		// - Setup workload driver
		app_driver_pointer p_drv;
		switch (opt_wkl_driver)
		{
			case testbed::rain_workload_generator:
				{
					boost::shared_ptr< testbed::rain::workload_driver<traits_type> > p_drv_impl;
					p_drv_impl = boost::make_shared< testbed::rain::workload_driver<traits_type> >(opt_wkl,
																								   opt_wkl_driver_rain_path);
					if (opt_wkl_rain_java_xargs.size() > 0 && !opt_wkl_rain_java_xargs[0].empty())
					{
						p_drv_impl->java_arguments(opt_wkl_rain_java_xargs.begin(), opt_wkl_rain_java_xargs.end());
					}

					p_app->register_sensor(opt_slo_metric, p_drv_impl->sensor(opt_slo_metric));
					p_drv = p_drv_impl;
				}
				break;
			case testbed::ycsb_workload_generator:
				{
					boost::shared_ptr< testbed::ycsb::workload_driver<traits_type> > p_drv_impl;
					p_drv_impl = boost::make_shared< testbed::ycsb::workload_driver<traits_type> >(opt_wkl,
																								   opt_wkl_ycsb_prop_paths.begin(),
																								   opt_wkl_ycsb_prop_paths.end(),
																								   opt_wkl_driver_ycsb_path,
																								   opt_wkl_ycsb_db_class,
																								   opt_wkl_ycsb_classpath);
					p_app->register_sensor(opt_slo_metric, p_drv_impl->sensor(opt_slo_metric));
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

		// - Setup signal generator
		random_generator_type rng(opt_rng_seed);
		std::map< testbed::virtual_machine_performance_category, boost::shared_ptr< testbed::base_signal_generator<real_type> > > sig_gens;
		for (std::size_t k = 0; k < opt_vm_perfs.size(); ++k)
		{
			const testbed::virtual_machine_performance_category cat = opt_vm_perfs[k];

			boost::shared_ptr< testbed::base_signal_generator<real_type> > p_sig_gen;

			// Specialized params
			switch (opt_sig)
			{
				case detail::constant_signal:
					{
						std::vector<real_type> u0(nt, opt_sig_const_val);
						p_sig_gen = boost::make_shared< testbed::constant_signal_generator<real_type> >(u0);
					}
					break;
				case detail::gaussian_signal:
					{
						std::vector<real_type> mean(nt, opt_sig_gauss_mean);
						std::vector<real_type> sd(nt, opt_sig_gauss_sd);
						//p_sig_gen = boost::make_shared< testbed::gaussian_signal_generator<real_type,random_generator_type> >(mean, sd, rng);
						p_sig_gen = boost::shared_ptr< testbed::base_signal_generator<real_type> >(new testbed::gaussian_signal_generator<real_type,random_generator_type>(mean, sd, rng));
					}
					break;
				case detail::half_sinusoidal_signal:
					{
						std::vector<real_type> ampl(nt, opt_sig_half_sine_ampl);
						std::vector<uint_type> freq(nt, opt_sig_half_sine_freq);
						std::vector<uint_type> phase(nt, opt_sig_half_sine_phase);
						std::vector<real_type> bias(nt, opt_sig_half_sine_bias);
						p_sig_gen = boost::make_shared< testbed::half_sinusoidal_signal_generator<real_type,uint_type> >(ampl, freq, phase, bias);
					}
					break;
				case detail::half_sinusoidal_mesh_signal:
					{
						std::vector<real_type> ampl(nt, opt_sig_half_sine_mesh_ampl);
						std::vector<uint_type> freq(nt, opt_sig_half_sine_mesh_freq);
						std::vector<uint_type> phase(nt, opt_sig_half_sine_mesh_phase);
						std::vector<real_type> bias(nt, opt_sig_half_sine_mesh_bias);
						p_sig_gen = boost::make_shared< testbed::half_sinusoidal_mesh_signal_generator<real_type,uint_type> >(ampl, freq, phase, bias);
					}
					break;
				case detail::sawtooth_signal:
					{
						std::vector<real_type> low(nt, opt_sig_sawtooth_low);
						std::vector<real_type> high(nt, opt_sig_sawtooth_high);
						std::vector<real_type> incr(nt, opt_sig_sawtooth_incr);
						p_sig_gen = boost::make_shared< testbed::sawtooth_signal_generator<real_type> >(low, high, incr);
					}
					break;
				case detail::sinusoidal_signal:
					{
						std::vector<real_type> ampl(nt, opt_sig_sine_ampl);
						std::vector<uint_type> freq(nt, opt_sig_sine_freq);
						std::vector<uint_type> phase(nt, opt_sig_sine_phase);
						std::vector<real_type> bias(nt, opt_sig_sine_bias);
						p_sig_gen = boost::make_shared< testbed::sinusoidal_signal_generator<real_type,uint_type> >(ampl, freq, phase, bias);
					}
					break;
				case detail::sinusoidal_mesh_signal:
					{
						std::vector<real_type> ampl(nt, opt_sig_sine_mesh_ampl);
						std::vector<uint_type> freq(nt, opt_sig_sine_mesh_freq);
						std::vector<uint_type> phase(nt, opt_sig_sine_mesh_phase);
						std::vector<real_type> bias(nt, opt_sig_sine_mesh_bias);
						p_sig_gen = boost::make_shared< testbed::sinusoidal_mesh_signal_generator<real_type,uint_type> >(ampl, freq, phase, bias);
					}
					break;
				case detail::square_signal:
					{
						std::vector<real_type> low(nt, opt_sig_square_low);
						std::vector<real_type> high(nt, opt_sig_square_high);
						p_sig_gen = boost::make_shared< testbed::square_signal_generator<real_type> >(low, high);
					}
					break;
				case detail::uniform_signal:
					{
						std::vector<real_type> min(nt, opt_sig_unif_min);
						std::vector<real_type> max(nt, opt_sig_unif_max);
						//p_sig_gen = boost::make_shared< testbed::uniform_signal_generator<real_type,random_generator_type> >(min, max, rng);
						p_sig_gen = boost::shared_ptr< testbed::base_signal_generator<real_type> >(new testbed::uniform_signal_generator<real_type,random_generator_type>(min, max, rng));
					}
					break;
				default:
					DCS_EXCEPTION_THROW(::std::runtime_error, "Unknown signal generator");
					break;
			}
			// Common params
			p_sig_gen->upper_bound(opt_sig_common_up_bound);
			p_sig_gen->lower_bound(opt_sig_common_lo_bound);

			sig_gens[cat] = p_sig_gen;
		}

		// - Setup system identificator
		app_manager_pointer p_mgr;
		testbed::sysid_application_manager<traits_type> sysid_mgr;
		for (typename std::map< testbed::virtual_machine_performance_category,boost::shared_ptr< testbed::base_signal_generator<real_type> > >::const_iterator it = sig_gens.begin(),
																																							   end_it = sig_gens.end();
			 it != end_it;
			 ++it)
		{
			sysid_mgr.signal_generator(it->first, it->second);
		}
		sysid_mgr.export_data_to(opt_out_dat_file);
		//sysid_mgr.output_extended_format(true);
		p_mgr = boost::make_shared< testbed::sysid_application_manager<traits_type> >(sysid_mgr);
		p_mgr->target_value(opt_slo_metric, std::numeric_limits<real_type>::quiet_NaN()); // SLO value not used
		p_mgr->data_estimator(opt_slo_metric, p_estimator);
		p_mgr->data_smoother(opt_slo_metric, p_smoother);
		for (::std::size_t i = 0; i < vms.size(); ++i)
		{
			const vm_pointer p_vm = vms[i];

			for (std::size_t k = 0; k < opt_vm_perfs.size(); ++k)
			{
				const testbed::virtual_machine_performance_category cat = opt_vm_perfs[k];

				p_mgr->data_estimator(cat, p_vm->id(), boost::make_shared< testbed::mean_estimator<real_type> >());
				p_mgr->data_smoother(cat, p_vm->id(), boost::make_shared< testbed::dummy_smoother<real_type> >());
			}
		}
		p_mgr->sampling_time(opt_ts);
		p_mgr->control_time(opt_tc);
		p_mgr->app(p_app);

		// Add to main experiment
		boost::shared_ptr< testbed::application_experiment<traits_type> > p_app_exp;
		p_app_exp = boost::make_shared< testbed::application_experiment<traits_type> >(p_app, p_drv, p_mgr);
		//p_app_exp->restore_state(!opt_no_restore_vms);
		sys_exp.add_app_experiment(p_app_exp);

		// Set experiment trackers
		testbed::utility::experiment_stats_gatherer<traits_type> exp_stats;
		exp_stats.track(sys_exp);

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

#else // if 0

int main(int argc, char *argv[])
{
	namespace testbed = ::dcs::testbed;

	typedef double real_type;
	typedef unsigned int uint_type;
	typedef testbed::traits<real_type,uint_type> traits_type;

	bool opt_help = false;
	std::vector<std::string> opt_vm_uris;
	std::string opt_cfg_file;
	std::string opt_out_dat_file;
	std::string opt_str;
	bool opt_verbose = false;


	// Parse command line options
	try
	{
		opt_help = dcs::cli::simple::get_option(argv, argv+argc, "--help");
//		opt_data_estimator = dcs::cli::simple::get_option<detail::data_estimator_category>(argv, argv+argc, "--data-estimator", detail::default_data_estimator);
//		opt_chen2000_ewma_quantile_prob = dcs::cli::simple::get_option<real_type>(argv, argv+argc, "--chen2000_ewma-quantile", detail::default_chen2000_ewma_quantile_prob);
//		opt_chen2000_ewma_w = dcs::cli::simple::get_option<real_type>(argv, argv+argc, "--chen2000_ewma-w", detail::default_chen2000_ewma_w);
//		opt_chen2000_ewsa_quantile_prob = dcs::cli::simple::get_option<real_type>(argv, argv+argc, "--chen2000_ewsa-quantile", detail::default_chen2000_ewsa_quantile_prob);
//		opt_chen2000_ewsa_w = dcs::cli::simple::get_option<real_type>(argv, argv+argc, "--chen2000_ewsa-w", detail::default_chen2000_ewsa_w);
//		opt_chen2000_sa_quantile_prob = dcs::cli::simple::get_option<real_type>(argv, argv+argc, "--chen2000_sa-quantile", detail::default_chen2000_sa_quantile_prob);
//		opt_jain1985_p2_quantile_prob = dcs::cli::simple::get_option<real_type>(argv, argv+argc, "--jain1985_p2-quantile", detail::default_jain1985_p2_quantile_prob);
//		opt_welsh2003_ewma_alpha = dcs::cli::simple::get_option<real_type>(argv, argv+argc, "--welsh2003_ewma-alpha", detail::default_welsh2003_ewma_alpha);
//		opt_welsh2003_ewma_quantile_prob = dcs::cli::simple::get_option<real_type>(argv, argv+argc, "--welsh2003_ewma-quantile", detail::default_welsh2003_ewma_quantile_prob);
//		opt_data_smoother = dcs::cli::simple::get_option<detail::data_smoother_category>(argv, argv+argc, "--data-smoother", detail::default_data_smoother);
//		opt_brown_single_exponential_alpha = dcs::cli::simple::get_option<real_type>(argv, argv+argc, "--brown_ses-alpha", detail::default_brown_single_exponential_alpha);
//		opt_brown_double_exponential_alpha = dcs::cli::simple::get_option<real_type>(argv, argv+argc, "--brown_des-alpha", detail::default_brown_double_exponential_alpha);
//		opt_holt_winters_double_exponential_alpha = dcs::cli::simple::get_option<real_type>(argv, argv+argc, "--holt_winters_des-alpha", detail::default_holt_winters_double_exponential_alpha);
//		opt_holt_winters_double_exponential_beta = dcs::cli::simple::get_option<real_type>(argv, argv+argc, "--holt_winters_des-beta", detail::default_holt_winters_double_exponential_beta);
//		opt_holt_winters_double_exponential_delta = dcs::cli::simple::get_option<real_type>(argv, argv+argc, "--holt_winters_des-delta", detail::default_holt_winters_double_exponential_delta);
		opt_cfg_file = dcs::cli::simple::get_option<std::string>(argv, argv+argc, "--config", detail::default_cfg_file);
		opt_out_dat_file = dcs::cli::simple::get_option<std::string>(argv, argv+argc, "--out-dat-file", detail::default_out_dat_file);
//		opt_rng_seed = dcs::cli::simple::get_option<uint_type>(argv, argv+argc, "--rng-seed", detail::default_rng_seed);
//		opt_sig = dcs::cli::simple::get_option<detail::signal_category>(argv, argv+argc, "--sig", detail::default_signal_category);
//		opt_sig_common_up_bound = dcs::cli::simple::get_option<real_type>(argv, argv+argc, "--sig-upper-bound", detail::default_signal_common_upper_bound);
//		opt_sig_common_lo_bound = dcs::cli::simple::get_option<real_type>(argv, argv+argc, "--sig-lower-bound", detail::default_signal_common_lower_bound);
//		opt_sig_const_val = dcs::cli::simple::get_option<real_type>(argv, argv+argc, "--sig-constant-val", detail::default_signal_const_val);
//		opt_sig_sawtooth_low = dcs::cli::simple::get_option<real_type>(argv, argv+argc, "--sig-sawtooth-low", detail::default_signal_sawtooth_low);
//		opt_sig_sawtooth_high = dcs::cli::simple::get_option<real_type>(argv, argv+argc, "--sig-sawtooth-high", detail::default_signal_sawtooth_high);
//		opt_sig_sawtooth_incr = dcs::cli::simple::get_option<real_type>(argv, argv+argc, "--sig-sawtooth-incr", detail::default_signal_sawtooth_incr);
//		opt_sig_sine_ampl = dcs::cli::simple::get_option<real_type>(argv, argv+argc, "--sig-sine-ampl", detail::default_signal_sine_amplitude);
//		opt_sig_sine_freq = dcs::cli::simple::get_option<uint_type>(argv, argv+argc, "--sig-sine-freq", detail::default_signal_sine_frequency);
//		opt_sig_sine_phase = dcs::cli::simple::get_option<uint_type>(argv, argv+argc, "--sig-sine-phase", detail::default_signal_sine_phase);
//		opt_sig_sine_bias = dcs::cli::simple::get_option<real_type>(argv, argv+argc, "--sig-sine-bias", detail::default_signal_sine_bias);
//		opt_sig_sine_mesh_ampl = dcs::cli::simple::get_option<real_type>(argv, argv+argc, "--sig-sine-mesh-ampl", detail::default_signal_sine_mesh_amplitude);
//		opt_sig_sine_mesh_freq = dcs::cli::simple::get_option<uint_type>(argv, argv+argc, "--sig-sine-mesh-freq", detail::default_signal_sine_mesh_frequency);
//		opt_sig_sine_mesh_phase = dcs::cli::simple::get_option<uint_type>(argv, argv+argc, "--sig-sine-mesh-phase", detail::default_signal_sine_mesh_phase);
//		opt_sig_sine_mesh_bias = dcs::cli::simple::get_option<real_type>(argv, argv+argc, "--sig-sine-mesh-bias", detail::default_signal_sine_mesh_bias);
//		opt_sig_half_sine_ampl = dcs::cli::simple::get_option<real_type>(argv, argv+argc, "--sig-half-sine-ampl", detail::default_signal_half_sine_amplitude);
//		opt_sig_half_sine_freq = dcs::cli::simple::get_option<uint_type>(argv, argv+argc, "--sig-half-sine-freq", detail::default_signal_half_sine_frequency);
//		opt_sig_half_sine_phase = dcs::cli::simple::get_option<uint_type>(argv, argv+argc, "--sig-half-sine-phase", detail::default_signal_half_sine_phase);
//		opt_sig_half_sine_bias = dcs::cli::simple::get_option<real_type>(argv, argv+argc, "--sig-half-sine-bias", detail::default_signal_half_sine_bias);
//		opt_sig_half_sine_mesh_ampl = dcs::cli::simple::get_option<real_type>(argv, argv+argc, "--sig-half-sine-mesh-ampl", detail::default_signal_half_sine_mesh_amplitude);
//		opt_sig_half_sine_mesh_freq = dcs::cli::simple::get_option<uint_type>(argv, argv+argc, "--sig-half-sine-mesh-freq", detail::default_signal_half_sine_mesh_frequency);
//		opt_sig_half_sine_mesh_phase = dcs::cli::simple::get_option<uint_type>(argv, argv+argc, "--sig-half-sine-mesh-phase", detail::default_signal_half_sine_mesh_phase);
//		opt_sig_half_sine_mesh_bias = dcs::cli::simple::get_option<real_type>(argv, argv+argc, "--sig-half-sine-mesh-bias", detail::default_signal_half_sine_mesh_bias);
//		opt_sig_square_low = dcs::cli::simple::get_option<real_type>(argv, argv+argc, "--sig-square-low", detail::default_signal_square_low);
//		opt_sig_square_high = dcs::cli::simple::get_option<real_type>(argv, argv+argc, "--sig-square-high", detail::default_signal_square_high);
//		opt_sig_unif_min = dcs::cli::simple::get_option<real_type>(argv, argv+argc, "--sig-uniform-min", detail::default_signal_uniform_min);
//		opt_sig_unif_max = dcs::cli::simple::get_option<real_type>(argv, argv+argc, "--sig-uniform-max", detail::default_signal_uniform_max);
//		opt_sig_gauss_mean = dcs::cli::simple::get_option<real_type>(argv, argv+argc, "--sig-gaussian-mean", detail::default_signal_gaussian_mean);
//		opt_sig_gauss_sd = dcs::cli::simple::get_option<real_type>(argv, argv+argc, "--sig-gaussian-sd", detail::default_signal_gaussian_sd);
//		opt_tc = dcs::cli::simple::get_option<real_type>(argv, argv+argc, "--tc", detail::default_control_time);
//		opt_ts = dcs::cli::simple::get_option<real_type>(argv, argv+argc, "--ts", detail::default_sampling_time);
		opt_verbose = dcs::cli::simple::get_option(argv, argv+argc, "--verbose");
//		opt_vm_perfs = dcs::cli::simple::get_options<testbed::virtual_machine_performance_category>(argv, argv+argc, "--vm-perf");
//		if (opt_vm_perfs.size() == 0)
//		{
//			opt_vm_perfs.push_back(detail::default_vm_performance);
//		}
		opt_vm_uris = dcs::cli::simple::get_options<std::string>(argv, argv+argc, "--vm-uri");
//		opt_wkl = dcs::cli::simple::get_option<testbed::workload_category>(argv, argv+argc, "--wkl", detail::default_workload);
//		opt_wkl_driver = dcs::cli::simple::get_option<testbed::workload_generator_category>(argv, argv+argc, "--wkl-driver", detail::default_workload_driver);
//		opt_wkl_driver_rain_path = dcs::cli::simple::get_option<std::string>(argv, argv+argc, "--wkl-driver-rain-path", detail::default_workload_driver_rain_path);
//		opt_wkl_driver_ycsb_path = dcs::cli::simple::get_option<std::string>(argv, argv+argc, "--wkl-driver-ycsb-path", detail::default_workload_driver_ycsb_path);
//		opt_wkl_ycsb_classpath = dcs::cli::simple::get_option<std::string>(argv, argv+argc, "--wkl-ycsb-classpath", detail::default_workload_ycsb_classpath);
//		opt_wkl_ycsb_db_class = dcs::cli::simple::get_option<std::string>(argv, argv+argc, "--wkl-ycsb-db-class", detail::default_workload_ycsb_db_class);
//		opt_wkl_ycsb_prop_paths = dcs::cli::simple::get_options<std::string>(argv, argv+argc, "--wkl-ycsb-prop-path", detail::default_workload_ycsb_prop_path);
//		opt_str = dcs::cli::simple::get_option<std::string>(argv, argv+argc, "--slo-metric", detail::default_slo_metric_str);
//		opt_slo_metric = detail::make_slo_metric(opt_str);
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

	if (opt_help)
	{
		detail::usage(argv[0]);
		return EXIT_SUCCESS;
	}

	int ret = 0;

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

		oss << "Config file: " << opt_cfg_file;
		dcs::log_info(DCS_LOGGING_AT, oss.str());
		oss.str("");

		oss << "Output data file: " << opt_out_dat_file;
		dcs::log_info(DCS_LOGGING_AT, oss.str());
		oss.str("");

		oss << "Verbose output: " << std::boolalpha << opt_verbose;
		dcs::log_info(DCS_LOGGING_AT, oss.str());
		oss.str("");
	}

	typedef testbed::base_virtual_machine<traits_type> vm_type;
	typedef boost::shared_ptr<vm_type> vm_pointer;
	typedef testbed::base_virtual_machine_manager<traits_type> vmm_type;
	typedef boost::shared_ptr<vmm_type> vmm_pointer;
	typedef vmm_type::identifier_type vmm_identifier_type;
    typedef testbed::base_application<traits_type> app_type;
    typedef boost::shared_ptr<app_type> app_pointer;
	typedef testbed::base_application_manager<traits_type> app_manager_type;
	typedef boost::shared_ptr<app_manager_type> app_manager_pointer;
	typedef testbed::base_workload_driver<traits_type> app_driver_type;
	typedef boost::shared_ptr<app_driver_type> app_driver_pointer;
	typedef boost::random::mt19937 random_generator_type;

	try
	{
		const std::size_t nt = opt_vm_uris.size(); // Number of tiers

		sysid_configuration<traits_type> config(opt_cfg_file, nt);

		testbed::system_experiment<traits_type> sys_exp;

		// Setup application experiment
		//  - Setup application (and VMs)
		std::map<vmm_identifier_type,vmm_pointer> vmm_map;
		std::vector<vm_pointer> vms;
		const std::vector<std::string>::const_iterator uri_end_it = opt_vm_uris.end();
		for (std::vector<std::string>::const_iterator it = opt_vm_uris.begin();
			 it != uri_end_it;
			 ++it)
		{
			std::string const& uri = *it;

			vmm_pointer p_vmm;
			if (vmm_map.count(uri) > 0)
			{
				p_vmm = vmm_map.at(uri);
			}
			else
			{
				p_vmm = boost::make_shared< testbed::libvirt::virtual_machine_manager<traits_type> >(uri);
				vmm_map[uri] = p_vmm;
			}

			vm_pointer p_vm = p_vmm->vm(uri);

			// check: p_vm != null
			DCS_DEBUG_ASSERT( p_vm );

			vms.push_back(p_vm);
		}
		app_pointer p_app = boost::make_shared< testbed::application<traits_type> >(vms.begin(), vms.end());

		const std::vector< testbed::conf::sys_output<traits_type> > sys_outputs = config.outputs();
		if (sys_outputs.size() > 1)
		{
			DCS_EXCEPTION_THROW(std::runtime_error, "Multiple output not yet managed");
		}
		for (std::size_t i = 0; i < sys_outputs.size(); ++i)
		{
			const testbed::application_performance_category app_perf_cat = sys_outputs[i].application_performance;

			// - Setup workload driver
			p_app->register_sensor(app_perf_cat, sys_outputs[i].p_wkl_gen->sensor(app_perf_cat));
			sys_outputs[i].p_wkl_gen->app(p_app);

			// - Setup system identifier
			boost::shared_ptr< testbed::sysid_application_manager<traits_type> > p_sysid_mgr(new testbed::sysid_application_manager<traits_type>());
			const std::vector< testbed::conf::sys_input<traits_type> > sys_inputs = config.inputs();
			for (std::size_t i = 0; i < sys_inputs.size(); ++i)
			{
				p_sysid_mgr->signal_generator(sys_inputs[i].vm_performance, sys_inputs[i].p_sig_gen);
			}
			p_sysid_mgr->export_data_to(config.output_data_file);
			//p_sysid_mgr->output_extended_format(true);
			p_sysid_mgr->target_value(opt_slo_metric, std::numeric_limits<real_type>::quiet_NaN()); // SLO value not used
			p_sysid_mgr->data_estimator(opt_slo_metric, p_estimator);
			p_sysid_mgr->data_smoother(opt_slo_metric, p_smoother);
			for (::std::size_t i = 0; i < vms.size(); ++i)
			{
				const vm_pointer p_vm = vms[i];

				for (std::size_t k = 0; k < opt_vm_perfs.size(); ++k)
				{
					const testbed::virtual_machine_performance_category cat = opt_vm_perfs[k];

					p_mgr->data_estimator(cat, p_vm->id(), boost::make_shared< testbed::mean_estimator<real_type> >());
					p_mgr->data_smoother(cat, p_vm->id(), boost::make_shared< testbed::dummy_smoother<real_type> >());
				}
			}
			p_mgr->sampling_time(opt_ts);
			p_mgr->control_time(opt_tc);
			p_mgr->app(p_app);

			// Add to main experiment
			boost::shared_ptr< testbed::application_experiment<traits_type> > p_app_exp;
			p_app_exp = boost::make_shared< testbed::application_experiment<traits_type> >(p_app, p_drv, p_mgr);
			//p_app_exp->restore_state(!opt_no_restore_vms);
			sys_exp.add_app_experiment(p_app_exp);
		}

		// Set experiment trackers
		testbed::utility::experiment_stats_gatherer<traits_type> exp_stats;
		exp_stats.track(sys_exp);

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

#endif // if 0
