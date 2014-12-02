/**
 * \file dcs/testbed/conf/commons.hpp
 *
 * \brief Common definitions
 *
 * \author Marco Guazzone (marco.guazzone@gmail.com)
 *
 * <hr/>
 *
 * Copyright 2014 Marco Guazzone (marco.guazzone@gmail.com)
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

#ifndef DCS_TESTBED_CONF_COMMONS_HPP
#define DCS_TESTBED_CONF_COMMONS_HPP


#include <boost/algorithm/string.hpp>
#include <boost/smart_ptr.hpp>
#include <dcs/exception.hpp>
#include <dcs/math/traits/float.hpp>
#include <dcs/testbed/application_performance_category.hpp>
#include <dcs/testbed/virtual_machine_performance_category.hpp>
#include <dcs/testbed/workload_category.hpp>
#include <dcs/testbed/workload_generator_category.hpp>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>


namespace dcs { namespace testbed { namespace conf {

enum app_manager_category
{
	albano2013_fuzzyqe_app_manager,
	anglano2014_fc2q_app_manager,
	anglano2014_fc2q_mimo_app_manager,
	dummy_app_manager,
	padala2009_autocontrol_app_manager,
	rao2013_dynaqos_app_manager,
	sysid_app_manager
};

template <typename CharT, typename CharTraitsT>
std::basic_istream<CharT,CharTraitsT>& operator>>(std::basic_istream<CharT,CharTraitsT>& is, app_manager_category& cat)
{
	std::string s;
	is >> s;
	boost::to_lower(s);

	if (!s.compare("albano2013-fuzzyqe") || !s.compare("albano2013"))
	{
		cat = albano2013_fuzzyqe_app_manager;
	}
	else if (!s.compare("anglano2014-fc2q-miso") || !s.compare("anglano2014-fc2q"))
	{
		cat = anglano2014_fc2q_app_manager;
	}
	else if (!s.compare("anglano2014-fc2q-mimo"))
	{
		cat = anglano2014_fc2q_mimo_app_manager;
	}
	else if (!s.compare("dummy"))
	{
		cat = dummy_app_manager;
	}
	else if (!s.compare("padala2009-autocontrol"))
	{
		cat = padala2009_autocontrol_app_manager;
	}
	else if (!s.compare("rao2013-dynaqos"))
	{
		cat = rao2013_dynaqos_app_manager;
	}
	else if (!s.compare("sysid"))
	{
		cat = sysid_app_manager;
	}
	else
	{
		DCS_EXCEPTION_THROW(std::runtime_error, "Unknown application manager category");
	}

	return is;
}

template <typename CharT, typename CharTraitsT>
std::basic_ostream<CharT,CharTraitsT>& operator<<(std::basic_ostream<CharT,CharTraitsT>& os, app_manager_category cat)
{
	switch (cat)
	{
		case albano2013_fuzzyqe_app_manager:
			os << "albano2013-fuzzyqe";
			break;
		case anglano2014_fc2q_app_manager:
			os << "anglano2014-fc2q";
			break;
		case anglano2014_fc2q_mimo_app_manager:
			os << "anglano2014-fc2q-mimo";
			break;
		case dummy_app_manager:
			os << "dummy";
			break;
		case padala2009_autocontrol_app_manager:
			os << "padal2009-autocontrol";
			break;
		case rao2013_dynaqos_app_manager:
			os << "rao2013-dynaqos";
			break;
		case sysid_app_manager:
			os << "sysid";
			break;
		default:
			DCS_EXCEPTION_THROW(::std::runtime_error, "Unknown application manager category");
	}

	return os;
}

enum data_estimator_category
{
	mean_estimator,
	chen2000_ewma_quantile_estimator,
	chen2000_ewsa_quantile_estimator,
	chen2000_sa_quantile_estimator,
	jain1985_p2_algorithm_quantile_estimator,
	most_recently_observed_estimator,
	true_quantile_estimator,
	welsh2003_ewma_quantile_estimator/*,
	welsh2003_ewma_ext_quantile_estimator*/
};

template <typename CharT, typename CharTraitsT>
std::basic_istream<CharT,CharTraitsT>& operator>>(std::basic_istream<CharT,CharTraitsT>& is, data_estimator_category& cat)
{
	std::string s;
	is >> s;
	boost::to_lower(s);

	if (!s.compare("mean"))
	{
		cat = mean_estimator;
	}
	else if (!s.compare("mro"))
	{
		cat = most_recently_observed_estimator;
	}
	else if (!s.compare("chen2000-ewma-quantile"))
	{
		cat = chen2000_ewma_quantile_estimator;
	}
	else if (!s.compare("chen2000-ewsa-quantile"))
	{
		cat = chen2000_ewsa_quantile_estimator;
	}
	else if (!s.compare("chen2000-sa-quantile"))
	{
		cat = chen2000_sa_quantile_estimator;
	}
	else if (!s.compare("jain1985-p2_algorithm-quantile"))
	{
		cat = jain1985_p2_algorithm_quantile_estimator;
	}
	else if (!s.compare("welsh2003-ewma-quantile"))
	{
		cat = welsh2003_ewma_quantile_estimator;
	}
//	else if (!s.compare("welsh2003-ewma-ext-quantile"))
//	{
//		cat = welsh2003_ewma_ext_quantile_estimator;
//	}
	else
	{
		DCS_EXCEPTION_THROW(std::runtime_error, "Unknown data estimator category");
	}

	return is;
}

template <typename CharT, typename CharTraitsT>
std::basic_ostream<CharT,CharTraitsT>& operator<<(std::basic_ostream<CharT,CharTraitsT>& os, data_estimator_category cat)
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
			os << "chen2000-ewma-quantile";
			break;
		case chen2000_ewsa_quantile_estimator:
			os << "chen2000-ewsa-quantile";
			break;
		case chen2000_sa_quantile_estimator:
			os << "chen2000-sa-quantile";
			break;
		case jain1985_p2_algorithm_quantile_estimator:
			os << "jain1985-p2-algorithm-quantile";
			break;
		case welsh2003_ewma_quantile_estimator:
			os << "welsh2003-ewma-quantile";
			break;
//		case welsh2003_ewma_ext_quantile_estimator:
//			os << "welsh2003-ewma-ext-quantile";
//			break;
		default:
			DCS_EXCEPTION_THROW(::std::runtime_error, "Unknown data estimator category");
	}

	return os;
}

enum data_smoother_category
{
    dummy_smoother,
    brown_single_exponential_smoother,
    brown_double_exponential_smoother,
    holt_winters_double_exponential_smoother
};

template <typename CharT, typename CharTraitsT>
std::basic_istream<CharT,CharTraitsT>& operator>>(std::basic_istream<CharT,CharTraitsT>& is, data_smoother_category& cat)
{
	std::string s;
	is >> s;
	boost::to_lower(s);

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
		DCS_EXCEPTION_THROW(std::runtime_error, "Unknown data smoother category");
	}

	return is;
}

template <typename CharT, typename CharTraitsT>
std::basic_ostream<CharT,CharTraitsT>& operator<<(std::basic_ostream<CharT,CharTraitsT>& os, data_smoother_category cat)
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
		default:
			DCS_EXCEPTION_THROW(std::runtime_error, "Unknown data smoother category");
	}

	return os;
}

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

template <typename CharT, typename CharTraitsT>
std::basic_istream<CharT,CharTraitsT>& operator>>(std::basic_istream<CharT,CharTraitsT>& is, signal_category& cat)
{
	std::string s;
	is >> s;
	boost::to_lower(s);

	if (!s.compare("constant"))
	{
		cat = constant_signal;
	}
	else if (!s.compare("half-sine"))
	{
		cat = half_sinusoidal_signal;
	}
	else if (!s.compare("half-sine-mesh"))
	{
		cat = half_sinusoidal_mesh_signal;
	}
	else if (!s.compare("gaussian"))
	{
		cat = gaussian_signal;
	}
	else if (!s.compare("sawtooth"))
	{
		cat = sawtooth_signal;
	}
	else if (!s.compare("sine"))
	{
		cat = sinusoidal_signal;
	}
	else if (!s.compare("sine-mesh"))
	{
		cat = sinusoidal_mesh_signal;
	}
	else if (!s.compare("square"))
	{
		cat = square_signal;
	}
	else if (!s.compare("uniform"))
	{
		cat = uniform_signal;
	}
	else
	{
		DCS_EXCEPTION_THROW(std::runtime_error, "Cannot find a valid signal category");
	}

	return is;
}

template <typename CharT, typename CharTraitsT>
std::basic_ostream<CharT,CharTraitsT>& operator<<(std::basic_ostream<CharT,CharTraitsT>& os, signal_category sig)
{
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
			break;
		default:
			DCS_EXCEPTION_THROW(std::runtime_error, "Cannot find a valid signal category");
	}

	return os;
}

namespace defaults
{
	const bool app_experiment_restore_vms = true;
	const app_manager_category app_manager = dummy_app_manager;
	const bool app_manager_sysid_report_extended = false;
	const std::string app_manager_sysid_report_path("./sysid-out.dat");
	const double app_manager_albano2013_fuzzyqe_beta = 0.9;
	const std::string app_manager_albano2013_fuzzyqe_report_path("./albano2013_fuzzyqe-out.dat");
	const double sampling_time = 10;
	const double control_time = 3*sampling_time;
	const double exciting_time = 3*sampling_time;
	const unsigned long rng_seed = 5489UL;
	const data_estimator_category data_estimator = mean_estimator;
	const double data_estimator_quantile_probability = 0.99;
	//const double data_estimator_jain1985_p2_quantile_probability = data_estimator_quantile_probability;
	const double data_estimator_chen2000_ewma_w = 0.05;
	//const double data_estimator_chen2000_ewma_quantile_probability = data_estimator_quantile_probability;
	const double data_estimator_chen2000_ewsa_w = 0.05;
	//const double data_estimator_chen2000_ewsa_quantile_probability = data_estimator_quantile_probability;
	//const double data_estimator_chen2000_sa_quantile_probability = data_estimator_quantile_probability;
	const double data_estimator_welsh2003_ewma_alpha = 0.7;
	//const double data_estimator_welsh2003_ewma_quantile_probability = data_estimator_quantile_probability;
	const bool data_estimator_welsh2003_ewma_extended = false;
	const data_smoother_category data_smoother = dummy_smoother;
	const double data_smoother_brown_single_exponential_alpha = 0.7;
	const double data_smoother_brown_double_exponential_alpha = 0.7;
	const double data_smoother_holt_winters_double_exponential_alpha = 0.8;
	const double data_smoother_holt_winters_double_exponential_beta = 0.3;
	const double data_smoother_holt_winters_double_exponential_delta = 0.7;
	const signal_category signal = constant_signal;
	const double signal_upper_bound = std::numeric_limits<double>::infinity();
	const double signal_lower_bound = -std::numeric_limits<double>::infinity();
	const double signal_constant_value = 1;
	const double signal_sawtooth_low = 0;
	const double signal_sawtooth_high = 1;
	const double signal_sawtooth_increment = 0.1;
	const double signal_sine_amplitude = 0.5;
	const unsigned int signal_sine_frequency = 8;
	const unsigned int signal_sine_phase = 0;
	const double signal_sine_bias = 0.5;
	const double signal_sine_mesh_amplitude = 0.5;
	const unsigned int signal_sine_mesh_frequency = 8;
	const unsigned int signal_sine_mesh_phase = 0;
	const double signal_sine_mesh_bias = 0.5;
	const double signal_half_sine_amplitude = 0.5;
	const unsigned int signal_half_sine_frequency = 8;
	const unsigned int signal_half_sine_phase = 0;
	const double signal_half_sine_bias = 0.5;
	const double signal_half_sine_mesh_amplitude = 0.5;
	const unsigned int signal_half_sine_mesh_frequency = 8;
	const unsigned int signal_half_sine_mesh_phase = 0;
	const double signal_half_sine_mesh_bias = 0.5;
	const double signal_square_low = 0;
	const double signal_square_high = 1;
	const double signal_uniform_min = 0;
	const double signal_uniform_max = 1;
	const double signal_gaussian_mean = 0;
	const double signal_gaussian_sd = 1;
	const dcs::testbed::application_performance_category slo_metric = dcs::testbed::response_time_application_performance;
	const double slo_value = std::numeric_limits<double>::quiet_NaN();
	const dcs::testbed::virtual_machine_performance_category vm_performance = dcs::testbed::cpu_util_virtual_machine_performance;
	const dcs::testbed::workload_category workload = dcs::testbed::olio_workload;
	const dcs::testbed::workload_generator_category workload_generator = dcs::testbed::rain_workload_generator;
	const std::string workload_generator_rain_path("/usr/local/opt/rain-workload-toolkit");
	const std::string workload_generator_ycsb_path("/usr/local/opt/YCSB");
	const std::vector<std::string> workload_generator_ycsb_property_files(1, "/usr/local/opt/YCSB/workloads/workloada");
	const std::string workload_generator_ycsb_classpath;
	const std::string workload_generator_ycsb_db_class("basic");
} // Namespace defaults

namespace detail {

template <typename RealT>
struct response_time_slo_checker
{
	response_time_slo_checker(RealT max_val, RealT rel_tol=0.05)
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
struct throughput_slo_checker
{
	throughput_slo_checker(RealT min_val, RealT rel_tol=0.05)
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

} // Namespace detail

}}} // Namespace dcs::testbed::conf


#endif // DCS_TESTBED_CONF_COMMONS_HPP
