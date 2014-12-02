/**
 * \file dcs/testbed/conf/yaml_configurator.hpp
 *
 * \brief Classes to configuration an experiment.
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

#ifndef DCS_TESTBED_CONF_YAML_CONFIGURATOR_HPP
#define DCS_TESTBED_CONF_YAML_CONFIGURATOR_HPP


#include <boost/algorithm/string.hpp>
#include <boost/random.hpp>
#include <boost/smart_ptr.hpp>
#include <cstddef>
#include <dcs/debug.hpp>
#include <dcs/exception.hpp>
#include <dcs/math/traits/float.hpp>
#include <dcs/testbed/application.hpp>
#include <dcs/testbed/application_experiment.hpp>
#include <dcs/testbed/application_managers.hpp>
#include <dcs/testbed/application_performance_category.hpp>
#include <dcs/testbed/base_application.hpp>
#include <dcs/testbed/conf/commons.hpp>
#include <dcs/testbed/data_estimators.hpp>
#include <dcs/testbed/data_smoothers.hpp>
//#include <dcs/testbed/experiment_stats_gatherer.hpp>
#include <dcs/testbed/io.hpp>
#include <dcs/testbed/signal_generators.hpp>
#include <dcs/testbed/system_experiment.hpp>
#include <dcs/testbed/traits.hpp>
#include <dcs/testbed/virtual_machine_managers.hpp>
#include <dcs/testbed/virtual_machine_performance_category.hpp>
#include <dcs/testbed/virtual_machines.hpp>
#include <dcs/testbed/workload_category.hpp>
#include <dcs/testbed/workload_drivers.hpp>
#include <dcs/testbed/workload_generator_category.hpp>
#include <iostream>
#include <limits>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <yaml-cpp/yaml.h>


#define DCS_TESTBED_CONF_MAKE_YAML_CONVERTER(T) \
	template <> \
	struct convert<T> \
	{ \
		static Node encode(T cat) \
		{ \
			std::ostringstream oss; \
			oss << cat; \
			return convert<std::string>::encode(oss.str()); \
		} \
		static bool decode(Node const& node, T& cat) \
		{ \
			std::string s; \
			const bool ret = convert<std::string>::decode(node, s); \
			if (ret) \
			{ \
				std::istringstream iss(s); \
				iss >> cat; \
			} \
			return ret; \
		} \
	}; // convert


namespace YAML {

DCS_TESTBED_CONF_MAKE_YAML_CONVERTER(dcs::testbed::conf::app_manager_category)
DCS_TESTBED_CONF_MAKE_YAML_CONVERTER(dcs::testbed::application_performance_category)
DCS_TESTBED_CONF_MAKE_YAML_CONVERTER(dcs::testbed::conf::data_estimator_category)
DCS_TESTBED_CONF_MAKE_YAML_CONVERTER(dcs::testbed::conf::data_smoother_category)
DCS_TESTBED_CONF_MAKE_YAML_CONVERTER(dcs::testbed::conf::signal_category)
DCS_TESTBED_CONF_MAKE_YAML_CONVERTER(dcs::testbed::virtual_machine_performance_category)
DCS_TESTBED_CONF_MAKE_YAML_CONVERTER(dcs::testbed::workload_category)
DCS_TESTBED_CONF_MAKE_YAML_CONVERTER(dcs::testbed::workload_generator_category)

} // Namespace YAML

namespace dcs { namespace testbed { namespace conf {

namespace detail {

template <typename T>
T yaml_value(YAML::Node const& node)
{
	return node.as<T>();
}

template <typename T>
T yaml_value(YAML::Node const& node, T default_val)
{
	if (node)
	{
		return node.as<T>();
	}

	return default_val;
}

template <typename RealT>
boost::shared_ptr< dcs::testbed::base_estimator<RealT> > parse_data_estimator(YAML::Node const& node)
{
	boost::shared_ptr< dcs::testbed::base_estimator<RealT> > p_estimator;

	if (node["estimator"])
	{
		const YAML::Node& est_node = node["estimator"];
		const data_estimator_category est_cat = yaml_value(est_node["method"], dcs::testbed::conf::defaults::data_estimator);
		switch (est_cat)
		{
			case chen2000_ewma_quantile_estimator:
				{
					const RealT prob = yaml_value(est_node["probability"], static_cast<RealT>(dcs::testbed::conf::defaults::data_estimator_quantile_probability));
					const RealT w = yaml_value(est_node["w"], static_cast<RealT>(dcs::testbed::conf::defaults::data_estimator_chen2000_ewma_w));

					p_estimator = boost::make_shared< dcs::testbed::chen2000_ewma_quantile_estimator<RealT> >(prob, w);
				}
				break;
			case chen2000_ewsa_quantile_estimator:
				{
					const RealT prob = yaml_value(est_node["probability"], static_cast<RealT>(dcs::testbed::conf::defaults::data_estimator_quantile_probability));
					const RealT w = yaml_value(est_node["w"], static_cast<RealT>(dcs::testbed::conf::defaults::data_estimator_chen2000_ewsa_w));

					p_estimator = boost::make_shared< dcs::testbed::chen2000_ewsa_quantile_estimator<RealT> >(prob, w);
				}
				break;
			case chen2000_sa_quantile_estimator:
				{
					const RealT prob = yaml_value(est_node["probability"], static_cast<RealT>(dcs::testbed::conf::defaults::data_estimator_quantile_probability));

					p_estimator = boost::make_shared< dcs::testbed::chen2000_sa_quantile_estimator<RealT> >(prob);
				}
				break;
			case jain1985_p2_algorithm_quantile_estimator:
				{
					const RealT prob = yaml_value(est_node["probability"], static_cast<RealT>(dcs::testbed::conf::defaults::data_estimator_quantile_probability));

					p_estimator = boost::make_shared< dcs::testbed::jain1985_p2_algorithm_quantile_estimator<RealT> >(prob);
				}
				break;
			case mean_estimator:
				{
					p_estimator = boost::make_shared< dcs::testbed::mean_estimator<RealT> >();
				}
				break;
			case most_recently_observed_estimator:
				{
					p_estimator = boost::make_shared< dcs::testbed::most_recently_observed_estimator<RealT> >();
				}
				break;
			case true_quantile_estimator:
				{
					const RealT prob = yaml_value(est_node["probability"], static_cast<RealT>(dcs::testbed::conf::defaults::data_estimator_quantile_probability));
					p_estimator = boost::make_shared< dcs::testbed::true_quantile_estimator<RealT> >(prob);
				}
				break;
			case welsh2003_ewma_quantile_estimator:
				{
					const RealT prob = yaml_value(est_node["probability"], static_cast<RealT>(dcs::testbed::conf::defaults::data_estimator_quantile_probability));
					const RealT alpha = yaml_value(est_node["alpha"], static_cast<RealT>(dcs::testbed::conf::defaults::data_estimator_welsh2003_ewma_alpha));
					const bool ext = yaml_value(est_node["extended"], static_cast<RealT>(dcs::testbed::conf::defaults::data_estimator_welsh2003_ewma_extended));

					p_estimator = boost::make_shared< dcs::testbed::welsh2003_ewma_quantile_estimator<RealT> >(prob, alpha, ext);
				}
				break;
			default:
				DCS_EXCEPTION_THROW(std::runtime_error, "Unknown data estimator");
		}
	}

	return p_estimator;
}

template <typename RealT>
boost::shared_ptr< dcs::testbed::base_smoother<RealT> > parse_data_smoother(YAML::Node const& node)
{
	boost::shared_ptr< dcs::testbed::base_smoother<RealT> > p_smoother;

	if (node["smoother"])
	{
		const YAML::Node& smo_node = node["smoother"];
		const data_smoother_category smo_cat = yaml_value(smo_node["category"], defaults::data_smoother);
		switch (smo_cat)
		{
			case brown_single_exponential_smoother:
				{
					const RealT alpha = yaml_value(smo_node["alpha"], static_cast<RealT>(defaults::data_smoother_brown_single_exponential_alpha));

					p_smoother = boost::make_shared< dcs::testbed::brown_single_exponential_smoother<RealT> >(alpha);
				}
				break;
			case brown_double_exponential_smoother:
				{
					const RealT alpha = yaml_value(smo_node["alpha"], static_cast<RealT>(defaults::data_smoother_brown_double_exponential_alpha));

					p_smoother = boost::make_shared< dcs::testbed::brown_double_exponential_smoother<RealT> >(alpha);
				}
				break;
			case dummy_smoother:
				{
					p_smoother = boost::make_shared< dcs::testbed::dummy_smoother<RealT> >();
				}
				break;
			case holt_winters_double_exponential_smoother:
				{
					if (smo_node["delta"])
					{
						const RealT delta = yaml_value(smo_node["delta"], static_cast<RealT>(defaults::data_smoother_holt_winters_double_exponential_delta));

						p_smoother = boost::make_shared< dcs::testbed::holt_winters_double_exponential_smoother<RealT> >(delta);
					}
					else
					{
						const RealT alpha = yaml_value(smo_node["alpha"], static_cast<RealT>(defaults::data_smoother_holt_winters_double_exponential_alpha));
						const RealT beta = yaml_value(smo_node["beta"], static_cast<RealT>(defaults::data_smoother_holt_winters_double_exponential_beta));

						p_smoother = boost::make_shared< dcs::testbed::holt_winters_double_exponential_smoother<RealT> >(alpha, beta);
					}
				}
				break;
			default:
				DCS_EXCEPTION_THROW(std::runtime_error, "Unknown data smoother");
		}
	}

	return p_smoother;
}

} // Namespace detail

template <typename TraitsT>
class yaml_configurator
{
	private: typedef typename TraitsT::real_type real_type;
	private: typedef typename TraitsT::uint_type uint_type;
	private: typedef typename TraitsT::rng_type rng_type;


	public: yaml_configurator()
	{
	}

	public: explicit yaml_configurator(std::string const& fname)
	{
		parse(fname);
	}

	public: boost::shared_ptr< dcs::testbed::system_experiment<TraitsT> > const& configure(std::string const& fname)
	{
		parse(fname);

		return p_sys_exp_;
	}

	public: boost::shared_ptr< dcs::testbed::system_experiment<TraitsT> > const& sys_experiment() const
	{
		return p_sys_exp_;
	}

	public: boost::shared_ptr< dcs::testbed::system_experiment<TraitsT> > const& sys_experiment()
	{
		return p_sys_exp_;
	}

	private: void parse(std::string const& fname)
	{
		YAML::Node yaml = YAML::LoadFile(fname);

		p_sys_exp_ = boost::make_shared< dcs::testbed::system_experiment<TraitsT> >();

		// Seup random number generator
	  	const uint_type rng_seed = detail::yaml_value(yaml["rng"]["seed"], static_cast<uint_type>(defaults::rng_seed));
	  	boost::shared_ptr<rng_type> p_rng = boost::make_shared<rng_type>(rng_seed);
		p_sys_exp_->rng(p_rng);

		// Seup experiments
		for (YAML::const_iterator exp_it = yaml["experiments"].begin(),
								  exp_end_it = yaml["experiments"].end();
			 exp_it != exp_end_it;
			 ++exp_it)
		{
			const YAML::Node& exp_node = *exp_it;

			boost::shared_ptr< dcs::testbed::application<TraitsT> > p_app;
			boost::shared_ptr< dcs::testbed::base_workload_driver<TraitsT> > p_wkl_drv;
			boost::shared_ptr< dcs::testbed::base_application_manager<TraitsT> > p_app_mgr;
			std::vector< boost::shared_ptr< base_virtual_machine<TraitsT> > > vms;

			// Setup application tiers
			if (exp_node["application-tiers"])
			{
				//const std::size_t num_tiers = exp_node["application-tiers"].size();

				std::map< typename base_virtual_machine_manager<TraitsT>::identifier_type,
						  boost::shared_ptr< base_virtual_machine_manager<TraitsT> > > vmm_map;

				for (YAML::const_iterator it = exp_node["application-tiers"].begin(),
										  end_it = exp_node["application-tiers"].end();
					 it != end_it;
					 ++it)
				{
					const YAML::Node& tier_node = *it;

					if (!tier_node["uri"])
					{
						DCS_EXCEPTION_THROW(std::runtime_error, "Missing 'application-tiers/uri' element");
					}

					const std::string uri = detail::yaml_value<std::string>(tier_node["uri"]);
					const std::string vmm_uri = dcs::testbed::libvirt::vmm_uri(uri);

					boost::shared_ptr< base_virtual_machine_manager<TraitsT> > p_vmm;
					if (!vmm_map.count(vmm_uri) > 0)
					{
						p_vmm = boost::make_shared< dcs::testbed::libvirt::virtual_machine_manager<TraitsT> >(vmm_uri);
						vmm_map[vmm_uri] = p_vmm;
					}
					else
					{
						p_vmm = vmm_map.at(vmm_uri);
					}

					boost::shared_ptr< base_virtual_machine<TraitsT> > p_vm = p_vmm->vm(uri);

					// check: p_vm != null
					DCS_DEBUG_ASSERT( p_vm );

					vms.push_back(p_vm);
				}

				p_app = boost::make_shared< dcs::testbed::application<TraitsT> >(vms.begin(), vms.end());
			}
			else
			{
				DCS_EXCEPTION_THROW(std::runtime_error, "Missing 'experiments/application-tiers' element");
			}
			// Setup workload
			if (exp_node["workload"])
			{
				typedef std::map< dcs::testbed::application_performance_category, boost::shared_ptr< base_sensor<TraitsT> > > sensor_map;
				typedef typename sensor_map::const_iterator sensor_iterator;

				const YAML::Node& wkl_node = exp_node["workload"];

				if (!wkl_node["generator"])
				{
					DCS_EXCEPTION_THROW(std::runtime_error, "Missing 'experiments/workload/generator' element");
				}

				const workload_category wkl_cat = detail::yaml_value(wkl_node["category"], defaults::workload);
				const workload_generator_category wkl_gen_cat = detail::yaml_value(wkl_node["generator"]["category"], defaults::workload_generator);
				switch (wkl_gen_cat)
				{
					case dcs::testbed::rain_workload_generator:
						{
							const std::string path = detail::yaml_value(wkl_node["generator"]["path"], defaults::workload_generator_rain_path);

							boost::shared_ptr< dcs::testbed::rain::workload_driver<TraitsT> > p_wkl_drv_impl;
							p_wkl_drv_impl = boost::make_shared< dcs::testbed::rain::workload_driver<TraitsT> >(wkl_cat, path);

							// Register sensors
							const sensor_map sens_map = p_wkl_drv_impl->sensors();
							for (sensor_iterator sens_it = sens_map.begin(),
												 sens_end_it = sens_map.end();
								 sens_it != sens_end_it;
								 ++sens_it)
							{
								p_app->register_sensor(sens_it->first, sens_it->second);
							}
							p_wkl_drv = p_wkl_drv_impl;
						}
						break;
					case dcs::testbed::ycsb_workload_generator:
						{
							const std::string path = detail::yaml_value(wkl_node["generator"]["path"], defaults::workload_generator_ycsb_path);
							const std::string classpath = detail::yaml_value(wkl_node["generator"]["classpath"], defaults::workload_generator_ycsb_classpath);
							const std::vector<std::string> prop_files = detail::yaml_value(wkl_node["generator"]["property-files"], defaults::workload_generator_ycsb_property_files);
							const std::string db_class = detail::yaml_value(wkl_node["generator"]["db-class"], defaults::workload_generator_ycsb_db_class);

							boost::shared_ptr< dcs::testbed::ycsb::workload_driver<TraitsT> > p_wkl_drv_impl;
							p_wkl_drv_impl = boost::make_shared< dcs::testbed::ycsb::workload_driver<TraitsT> >(wkl_cat,
																												prop_files.begin(),
																												prop_files.end(),
																												path,
																												db_class,
																												classpath);

							// Register sensors
							const sensor_map sens_map = p_wkl_drv_impl->sensors();
							for (sensor_iterator sens_it = sens_map.begin(),
												 sens_end_it = sens_map.end();
								 sens_it != sens_end_it;
								 ++sens_it)
							{
								p_app->register_sensor(sens_it->first, sens_it->second);
							}
							p_wkl_drv = p_wkl_drv_impl;
						}
						break;
					default:
						DCS_EXCEPTION_THROW(std::runtime_error, "Uknown workload generator category");
				}
				p_wkl_drv->app(p_app);
			}
			else
			{
				DCS_EXCEPTION_THROW(std::runtime_error, "Missing 'experiments/workload' element");
			}
			// Setup application controller
			if (exp_node["application-manager"])
			{
				const YAML::Node& mgr_node = exp_node["application-manager"];
				const std::size_t num_tiers = vms.size();

				boost::shared_ptr< dcs::testbed::base_application_manager<TraitsT> > p_mgr;

				const app_manager_category app_mgr_cat = detail::yaml_value(mgr_node["category"], defaults::app_manager);
				switch (app_mgr_cat)
				{
					case albano2013_fuzzyqe_app_manager:
						{
							const real_type beta = detail::yaml_value(mgr_node["beta"], defaults::app_manager_albano2013_fuzzyqe_beta);

							boost::shared_ptr< dcs::testbed::albano2013_fuzzyqe_application_manager<TraitsT> > p_app_mgr_impl;
							p_app_mgr_impl = boost::make_shared< dcs::testbed::albano2013_fuzzyqe_application_manager<TraitsT> >();

							p_app_mgr_impl->smoothing_factor(beta);
							if (mgr_node["report"])
							{
								p_app_mgr_impl->export_data_to(detail::yaml_value(mgr_node["report"]["path"], defaults::app_manager_albano2013_fuzzyqe_report_path));
							}

							p_app_mgr = p_app_mgr_impl;
						}
						break;
/*
					case anglano2014_fc2q_app_manager:
						{
							const real_type beta = 0.9;

							testbed::anglano2014_fc2q_application_manager<traits_type> anglano2014_fc2q_mgr;
							anglano2014_fc2q_mgr.smoothing_factor(beta);
							if (!opt_app_manager_stats_file.empty())
							{
								anglano2014_fc2q_mgr.export_data_to(opt_app_manager_stats_file);
							}

							p_app_mgr = boost::make_shared< testbed::anglano2014_fc2q_application_manager<traits_type> >(anglano2014_fc2q_mgr);
						}
						break;
					case anglano2014_fc2q_mimo_app_manager:
						{
							const real_type beta = 0.9;

							testbed::anglano2014_fc2q_mimo_application_manager<traits_type> anglano2014_fc2q_mimo_mgr;
							anglano2014_fc2q_mimo_mgr.smoothing_factor(beta);
							if (!opt_app_manager_stats_file.empty())
							{
								anglano2014_fc2q_mimo_mgr.export_data_to(opt_app_manager_stats_file);
							}

							p_app_mgr = boost::make_shared< testbed::anglano2014_fc2q_mimo_application_manager<traits_type> >(anglano2014_fc2q_mimo_mgr);
						}
						break;
					case dummy_app_manager:
						{
							testbed::dummy_application_manager<traits_type> dummy_mgr;
							if (!opt_app_manager_stats_file.empty())
							{
								dummy_mgr.export_data_to(opt_app_manager_stats_file);
							}

							p_app_mgr = boost::make_shared< testbed::dummy_application_manager<traits_type> >(dummy_mgr);
						}
						break;
					case padala2009_autocontrol_app_manager:
						{
							const std::size_t na = 2;
							const std::size_t nb = 2;
							const std::size_t nk = 1;
							const std::size_t ny = 1;
							const std::size_t nu = num_tiers;
							const real_type ff = 0.98;
							const real_type q = 2;

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

							p_app_mgr = boost::make_shared< testbed::padala2009_autocontrol_application_manager<traits_type> >(padala2009_autocontrol_mgr);
						}
						break;
					case rao2013_dynaqos_app_manager:
						{
							const real_type gamma = 0.8;

							testbed::rao2013_dynaqos_application_manager<traits_type> rao2013_dynaqos_mgr;
							rao2013_dynaqos_mgr.discount_factor(gamma);
							if (!opt_app_manager_stats_file.empty())
							{
								rao2013_dynaqos_mgr.export_data_to(opt_app_manager_stats_file);
							}

							p_app_mgr = boost::make_shared< testbed::rao2013_dynaqos_application_manager<traits_type> >(rao2013_dynaqos_mgr);
						}
						break;
*/
					case sysid_app_manager:
						{
							boost::shared_ptr< dcs::testbed::sysid_application_manager<TraitsT> > p_app_mgr_impl;
							p_app_mgr_impl = boost::make_shared< dcs::testbed::sysid_application_manager<TraitsT> >();

							if (mgr_node["report"])
							{
								p_app_mgr_impl->output_extended_format(detail::yaml_value(mgr_node["report"]["extended"], defaults::app_manager_sysid_report_extended));
								p_app_mgr_impl->export_data_to(detail::yaml_value(mgr_node["report"]["path"], defaults::app_manager_sysid_report_path));
							}
							if (yaml["signals"])
							{
								for (YAML::const_iterator sig_it = mgr_node["signals"].begin(),
														  sig_end_it = mgr_node["signals"].end();
									 sig_it != sig_end_it;
									 ++sig_it) 
								{
									const YAML::Node& sig_node = *sig_it;

									const dcs::testbed::virtual_machine_performance_category vm_perf_cat  = detail::yaml_value<dcs::testbed::virtual_machine_performance_category>(sig_node["knob"], defaults::vm_performance);

									boost::shared_ptr< base_signal_generator<real_type> > p_sig_gen;
									const signal_category sig_cat = detail::yaml_value(sig_node["category"], defaults::signal);
									switch (sig_cat)
									{
										case constant_signal:
											{
												const real_type val = detail::yaml_value(sig_node["value"], static_cast<real_type>(defaults::signal_constant_value));
												std::vector<real_type> u0(num_tiers, val);
												p_sig_gen = boost::make_shared< dcs::testbed::constant_signal_generator<real_type> >(u0);
											}
											break;
										case gaussian_signal:
											{
												const real_type mean = detail::yaml_value(sig_node["mean"], static_cast<real_type>(defaults::signal_gaussian_mean));
												const real_type sd = detail::yaml_value(sig_node["sd"], static_cast<real_type>(defaults::signal_gaussian_sd));

												const std::vector<real_type> means(num_tiers, mean);
												const std::vector<real_type> sds(num_tiers, sd);
												p_sig_gen = boost::shared_ptr< dcs::testbed::base_signal_generator<real_type> >(new dcs::testbed::gaussian_signal_generator<real_type,rng_type>(means, sds, *p_rng));
											}
											break;
										case half_sinusoidal_signal:
											{
												const real_type ampl = detail::yaml_value(sig_node["amplitude"], static_cast<real_type>(defaults::signal_half_sine_amplitude));
												const uint_type freq = detail::yaml_value(sig_node["frequency"], static_cast<uint_type>(defaults::signal_half_sine_frequency));
												const uint_type phase = detail::yaml_value(sig_node["phase"], static_cast<uint_type>(defaults::signal_half_sine_phase));
												const real_type bias = detail::yaml_value(sig_node["bias"], static_cast<real_type>(defaults::signal_half_sine_bias));

												const std::vector<real_type> ampls(num_tiers, ampl);
												const std::vector<uint_type> freqs(num_tiers, freq);
												const std::vector<uint_type> phases(num_tiers, phase);
												const std::vector<real_type> biases(num_tiers, bias);
												p_sig_gen = boost::make_shared< dcs::testbed::half_sinusoidal_signal_generator<real_type,uint_type> >(ampls, freqs, phases, biases);
											}
											break;
										case half_sinusoidal_mesh_signal:
											{
												const real_type ampl = detail::yaml_value(sig_node["amplitude"], static_cast<real_type>(defaults::signal_half_sine_mesh_amplitude));
												const uint_type freq = detail::yaml_value(sig_node["frequency"], static_cast<uint_type>(defaults::signal_half_sine_mesh_frequency));
												const uint_type phase = detail::yaml_value(sig_node["phase"], static_cast<uint_type>(defaults::signal_half_sine_mesh_phase));
												const real_type bias = detail::yaml_value(sig_node["bias"], static_cast<real_type>(defaults::signal_half_sine_mesh_bias));

												const std::vector<real_type> ampls(num_tiers, ampl);
												const std::vector<uint_type> freqs(num_tiers, freq);
												const std::vector<uint_type> phases(num_tiers, phase);
												const std::vector<real_type> biases(num_tiers, bias);
												p_sig_gen = boost::make_shared< dcs::testbed::half_sinusoidal_mesh_signal_generator<real_type,uint_type> >(ampls, freqs, phases, biases);
											}
											break;
										case sawtooth_signal:
											{
												const real_type low = detail::yaml_value(sig_node["low"], static_cast<real_type>(defaults::signal_sawtooth_low));
												const real_type high = detail::yaml_value(sig_node["high"], static_cast<real_type>(defaults::signal_sawtooth_high));
												const real_type incr = detail::yaml_value(sig_node["increment"], static_cast<real_type>(defaults::signal_sawtooth_increment));

												const std::vector<real_type> lows(num_tiers, low);
												const std::vector<real_type> highs(num_tiers, high);
												const std::vector<real_type> incrs(num_tiers, incr);
												p_sig_gen = boost::make_shared< dcs::testbed::sawtooth_signal_generator<real_type> >(lows, highs, incrs);
											}
											break;
										case sinusoidal_signal:
											{
												const real_type ampl = detail::yaml_value(sig_node["amplitude"], static_cast<real_type>(defaults::signal_sine_amplitude));
												const uint_type freq = detail::yaml_value(sig_node["frequency"], static_cast<uint_type>(defaults::signal_sine_frequency));
												const uint_type phase = detail::yaml_value(sig_node["phase"], static_cast<uint_type>(defaults::signal_sine_phase));
												const real_type bias = detail::yaml_value(sig_node["bias"], static_cast<real_type>(defaults::signal_sine_bias));

												const std::vector<real_type> ampls(num_tiers, ampl);
												const std::vector<uint_type> freqs(num_tiers, freq);
												const std::vector<uint_type> phases(num_tiers, phase);
												const std::vector<real_type> biases(num_tiers, bias);
												p_sig_gen = boost::make_shared< dcs::testbed::sinusoidal_signal_generator<real_type,uint_type> >(ampls, freqs, phases, biases);
											}
											break;
										case sinusoidal_mesh_signal:
											{
												const real_type ampl = detail::yaml_value(sig_node["amplitude"], static_cast<real_type>(defaults::signal_sine_mesh_amplitude));
												const uint_type freq = detail::yaml_value(sig_node["frequency"], static_cast<uint_type>(defaults::signal_sine_mesh_frequency));
												const uint_type phase = detail::yaml_value(sig_node["phase"], static_cast<uint_type>(defaults::signal_sine_mesh_phase));
												const real_type bias = detail::yaml_value(sig_node["bias"], static_cast<real_type>(defaults::signal_sine_mesh_bias));

												const std::vector<real_type> ampls(num_tiers, ampl);
												const std::vector<uint_type> freqs(num_tiers, freq);
												const std::vector<uint_type> phases(num_tiers, phase);
												const std::vector<real_type> biases(num_tiers, bias);
												p_sig_gen = boost::make_shared< dcs::testbed::sinusoidal_mesh_signal_generator<real_type,uint_type> >(ampls, freqs, phases, biases);
											}
											break;
										case square_signal:
											{
												const real_type low = detail::yaml_value(sig_node["low"], static_cast<real_type>(defaults::signal_square_low));
												const real_type high = detail::yaml_value(sig_node["high"], static_cast<real_type>(defaults::signal_square_high));

												const std::vector<real_type> lows(num_tiers, low);
												const std::vector<real_type> highs(num_tiers, high);
												p_sig_gen = boost::make_shared< dcs::testbed::square_signal_generator<real_type> >(lows, highs);
											}
											break;
										case uniform_signal:
											{
												const real_type min = detail::yaml_value(sig_node["min"], static_cast<real_type>(defaults::signal_uniform_min));
												const real_type max = detail::yaml_value(sig_node["max"], static_cast<real_type>(defaults::signal_uniform_max));

												const std::vector<real_type> mins(num_tiers, min);
												const std::vector<real_type> maxs(num_tiers, max);
												p_sig_gen = boost::shared_ptr< dcs::testbed::base_signal_generator<real_type> >(new dcs::testbed::uniform_signal_generator<real_type,rng_type>(mins, maxs, *p_rng));
											}
											break;
										default:
											DCS_EXCEPTION_THROW(std::runtime_error, "Unknown signal generator");
											break;
									}
									p_sig_gen->lower_bound( detail::yaml_value(sig_node["lower-bound"], static_cast<real_type>(defaults::signal_lower_bound)) );
									p_sig_gen->upper_bound( detail::yaml_value(sig_node["upper-bound"], static_cast<real_type>(defaults::signal_upper_bound)) );

									p_app_mgr_impl->signal_generator(vm_perf_cat, p_sig_gen);
								}
							}
							p_app_mgr = p_app_mgr_impl;
						}
						break;
					default:
						DCS_EXCEPTION_THROW(std::runtime_error, "Unknown application manager category");
				}

				// Setup SLO information
				if (mgr_node["slos"])
				{
					for (YAML::const_iterator slo_it = mgr_node["slos"].begin(),
											  slo_end_it = mgr_node["slos"].end();
						 slo_it != slo_end_it;
						 ++slo_it) 
					{
						const YAML::Node& slo_node = *slo_it;

						const dcs::testbed::application_performance_category slo_metric = detail::yaml_value(slo_node["metric"], defaults::slo_metric);
						const real_type slo_value = detail::yaml_value(slo_node["value"], static_cast<real_type>(defaults::slo_value));

						// Setup SLO value
						p_app_mgr->target_value(slo_metric, detail::yaml_value(slo_node["value"], static_cast<real_type>(defaults::slo_value)));

						// Setup SLO estimator
						p_app_mgr->data_estimator(slo_metric, detail::parse_data_estimator<real_type>(slo_node));

						// Setup SLO smoother
						p_app_mgr->data_smoother(slo_metric, detail::parse_data_smoother<real_type>(slo_node));

						// Seup SLO checker
						switch (slo_metric)
						{
							case dcs::testbed::response_time_application_performance:
								p_app->slo(dcs::testbed::response_time_application_performance, detail::response_time_slo_checker<real_type>(slo_value));
								break;
							case dcs::testbed::throughput_application_performance:
								p_app->slo(dcs::testbed::throughput_application_performance, detail::throughput_slo_checker<real_type>(slo_value));
								break;
							default:
								DCS_EXCEPTION_THROW(std::runtime_error, "Unknown SLO metric");
						}

						//// Setup SLO sensor
						//p_app->register_sensor(slo_metric, p_wkl_drv->sensor(slo_metric));
					}
				}
				else
				{
					DCS_EXCEPTION_THROW(std::runtime_error, "Missing 'experiments/application-manager/slos' element");
				}

				// Setup control knobs information
				if (mgr_node["knobs"])
				{
					for (YAML::const_iterator knob_it = mgr_node["knobs"].begin(),
											  knob_end_it = mgr_node["knobs"].end();
						 knob_it != knob_end_it;
						 ++knob_it) 
					{
						const YAML::Node& knob_node = *knob_it;

						const dcs::testbed::virtual_machine_performance_category vm_perf_cat = detail::yaml_value(knob_node["category"], defaults::vm_performance);

						for (std::size_t i = 0; i < vms.size(); ++i)
						{
							const boost::shared_ptr< base_virtual_machine<TraitsT> > p_vm = vms[i];

							// Setup knob estimator
							p_app_mgr->data_estimator(vm_perf_cat, p_vm->id(), detail::parse_data_estimator<real_type>(knob_node));

							// Setup knob smoother
							p_app_mgr->data_smoother(vm_perf_cat, p_vm->id(), detail::parse_data_smoother<real_type>(knob_node));
						}
					}
				}
				else
				{
					DCS_EXCEPTION_THROW(std::runtime_error, "Missing 'experiments/application-manager/knobs' element");
				}

				p_app_mgr->sampling_time(detail::yaml_value(mgr_node["sampling-time"], static_cast<real_type>(defaults::sampling_time)));
				p_app_mgr->control_time(detail::yaml_value(mgr_node["control-time"], static_cast<real_type>(defaults::control_time)));
				p_app_mgr->app(p_app);

			}

			boost::shared_ptr< application_experiment<TraitsT> > p_app_exp = boost::make_shared< testbed::application_experiment<TraitsT> >(p_app, p_wkl_drv, p_app_mgr);

			// Setup experiment's name
			if (exp_node["name"])
			{
				p_app_exp->name(detail::yaml_value<std::string>(exp_node["name"]));
			}
			//p_app_exp->restore_state(!opt_no_restore_vms);

			p_sys_exp_->add_app_experiment(p_app_exp);
		}
	}


	private: boost::shared_ptr< dcs::testbed::system_experiment<TraitsT> > p_sys_exp_;
}; // yaml_configurator

}}} // Namespace dcs::testbed::conf

#endif // DCS_TESTBED_CONF_YAML_CONFIGURATOR_HPP
