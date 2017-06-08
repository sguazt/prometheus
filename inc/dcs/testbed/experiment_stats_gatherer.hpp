/**
 * \file dcs/testbed/experiment_stats_gatherer.hpp
 *
 * \brief Gathers summary statistics for a system experiment.
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

#ifndef DCS_TESTBED_UTILITY_EXPERIMENT_STATS_GATHERER_HPP
#define DCS_TESTBED_UTILITY_EXPERIMENT_STATS_GATHERER_HPP


#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics/extended_p_square.hpp>
#include <boost/accumulators/statistics/mean.hpp>
#include <boost/accumulators/statistics/min.hpp>
#include <boost/accumulators/statistics/max.hpp>
#include <boost/accumulators/statistics/stats.hpp>
#include <boost/accumulators/statistics/variance.hpp>
#include <boost/smart_ptr.hpp>
#include <dcs/macro.hpp>
#include <dcs/testbed/application_performance_category.hpp>
#include <dcs/testbed/base_application.hpp>
#include <dcs/testbed/base_experiment_tracker.hpp>
#include <dcs/testbed/base_virtual_machine.hpp>
#include <dcs/testbed/virtual_machine_performance_category.hpp>
#include <limits>
#include <map>
#include <vector>


namespace dcs { namespace testbed { namespace utility {

template <typename TraitsT>
class experiment_stats_gatherer: public base_experiment_tracker<TraitsT>
{
	private: typedef base_experiment_tracker<TraitsT> base_type;
	private: typedef TraitsT traits_type;
	private: typedef typename traits_type::real_type real_type;
	private: typedef typename base_type::sys_experiment_type sys_experiment_type;
	private: typedef typename base_type::app_experiment_type app_experiment_type;
	private: typedef typename base_virtual_machine<traits_type>::identifier_type vm_identifier_type;
	private: typedef typename base_application<traits_type>::identifier_type app_identifier_type;
	private: typedef ::boost::accumulators::accumulator_set<real_type,
															::boost::accumulators::stats< ::boost::accumulators::tag::count,
		 																				  ::boost::accumulators::tag::mean,
																						  ::boost::accumulators::tag::variance,
																						  ::boost::accumulators::tag::min,
																						  ::boost::accumulators::tag::max > > summary_accumulator_type;
	private: typedef ::boost::accumulators::accumulator_set<real_type,
															::boost::accumulators::stats< ::boost::accumulators::tag::p_square_quantile > > quantile_accumulator_type;
//															::boost::accumulators::stats< ::boost::accumulators::tag::extended_p_square_quantile > > quantile_accumulator_type;
	//private: typedef ::std::map<vm_identifier_type,summary_accumulator_type> vm_summary_accumulator_map;
	//private: typedef ::std::map<vm_identifier_type,quantile_accumulator_type> vm_quantile_accumulator_map;
	private: typedef ::std::map<vm_identifier_type, ::std::map<virtual_machine_performance_category,summary_accumulator_type> > vm_summary_accumulator_map;
	private: typedef ::std::map<vm_identifier_type, ::std::map<virtual_machine_performance_category,quantile_accumulator_type> > vm_quantile_accumulator_map;
	//public: typedef typename app_experiment_type::identifier_type app_experiment_identifier_type;
	//private: typedef ::std::map< app_experiment_identifier_type, ::std::map<application_performance_category,summary_accumulator_type> > app_summary_accumulator_map;
	//private: typedef ::std::map< app_experiment_identifier_type, ::std::map<application_performance_category,quantile_accumulator_type> > app_quantile_accumulator_map;
	private: typedef ::std::map< app_identifier_type, ::std::map<application_performance_category,summary_accumulator_type> > app_summary_accumulator_map;
	private: typedef ::std::map< app_identifier_type, ::std::map<application_performance_category,quantile_accumulator_type> > app_quantile_accumulator_map;


	public: real_type app_mean(app_identifier_type id, application_performance_category cat)
	{
		return ::boost::accumulators::mean(app_perfs_summary_.at(id).at(cat));
	}

	public: real_type app_variance(app_identifier_type id, application_performance_category cat)
	{
		//NOTE: Boost.Accumulators's variance returns the biased variance.
		//		Instead we are interested in the unbiased variance

		const std::size_t n = ::boost::accumulators::count(app_perfs_summary_.at(id).at(cat));
		return (n > 1) ? ::boost::accumulators::variance(app_perfs_summary_.at(id).at(cat))*n/static_cast<real_type>(n-1) : 0;
	}

	public: real_type app_min(app_identifier_type id, application_performance_category cat)
	{
		return ::boost::accumulators::min(app_perfs_summary_.at(id).at(cat));
	}

	public: real_type app_max(app_identifier_type id, application_performance_category cat)
	{
		return ::boost::accumulators::max(app_perfs_summary_.at(id).at(cat));
	}

	public: real_type app_quantile(app_identifier_type id, application_performance_category cat, real_type prob)
	{
		return ::boost::accumulators::quantile(app_perfs_quantile_.at(id).at(cat), ::boost::accumulators::quantile_probability = prob);
	}

	public: real_type vm_cpu_mean(vm_identifier_type id, virtual_machine_performance_category cat)
	{
		return ::boost::accumulators::mean(vm_perfs_summary_.at(id).at(cat));
	}

	public: real_type vm_cpu_variance(vm_identifier_type id, virtual_machine_performance_category cat)
	{
		//NOTE: Boost.Accumulators's variance returns the biased variance.
		//		Instead we are interested in the unbiased variance

		const std::size_t n = ::boost::accumulators::count(vm_perfs_summary_.at(id).at(cat));
		return (n > 1) ? ::boost::accumulators::variance(vm_perfs_summary_.at(id).at(cat))*n/static_cast<real_type>(n-1) : 0;
	}

	public: real_type vm_min(app_identifier_type id, virtual_machine_performance_category cat)
	{
		return ::boost::accumulators::min(vm_perfs_summary_.at(id).at(cat));
	}

	public: real_type vm_max(app_identifier_type id, virtual_machine_performance_category cat)
	{
		return ::boost::accumulators::max(vm_perfs_summary_.at(id).at(cat));
	}

	public: real_type vm_quantile(app_identifier_type id, virtual_machine_performance_category cat, real_type prob)
	{
		return ::boost::accumulators::quantile(vm_perfs_quantile_.at(id).at(cat), ::boost::accumulators::quantile_probability = prob);
	}

	private: void do_reset()
	{
		vm_perfs_summary_.clear();
		vm_perfs_quantile_.clear();
		app_perfs_summary_.clear();
		app_perfs_quantile_.clear();
	}

	private: void do_on_start(sys_experiment_type const& exp)
	{
		DCS_MACRO_SUPPRESS_UNUSED_VARIABLE_WARNING( exp );

		this->reset();
	}

	private: void do_on_app_start(app_experiment_type const& exp)
	{
		typedef ::boost::shared_ptr< ::dcs::testbed::base_virtual_machine<traits_type> > vm_pointer;
		typedef ::std::vector<vm_pointer> vm_container;
		typedef typename vm_container::const_iterator vm_iterator;

		const real_type probs[] = {0.25, 0.50, 0.75, 0.90, 0.95, 0.99};

		vm_container vms = exp.app().vms();

		vm_iterator vm_end_it = vms.end();
		for (vm_iterator vm_it = vms.begin();
			 vm_it != vm_end_it;
			 ++vm_it)
		{
			const vm_pointer p_vm = *vm_it;

			const ::std::vector<virtual_machine_performance_category> metrics = virtual_machine_performance_categories();
			for (std::size_t i = 0; i < metrics.size(); ++i)
			{
				virtual_machine_performance_category metric = metrics[i];

				vm_perfs_summary_[p_vm->id()][metric] = summary_accumulator_type();
				vm_perfs_quantile_[p_vm->id()][metric] = quantile_accumulator_type(::boost::accumulators::extended_p_square_probabilities = probs);
			}
		}

		const ::std::vector<application_performance_category> metrics = exp.manager().target_metrics();
		for (::std::size_t i = 0; i < metrics.size(); ++i)
		{
			application_performance_category metric = metrics[i];

			app_perfs_summary_[exp.app().id()][metric] = summary_accumulator_type();
			app_perfs_quantile_[exp.app().id()][metric] = quantile_accumulator_type(::boost::accumulators::tag::extended_p_square::probabilities = probs);
		}
	}

	private: void do_on_app_sample(app_experiment_type const& exp)
	{
		DCS_MACRO_SUPPRESS_UNUSED_VARIABLE_WARNING( exp );
	}

	private: void do_on_app_control(app_experiment_type const& exp)
	{
		typedef ::boost::shared_ptr< ::dcs::testbed::base_virtual_machine<traits_type> > vm_pointer;
		typedef ::std::vector<vm_pointer> vm_container;
		typedef typename vm_container::const_iterator vm_iterator;

		vm_container vms = exp.app().vms();

		vm_iterator vm_end_it = vms.end();
		for (vm_iterator vm_it = vms.begin();
			 vm_it != vm_end_it;
			 ++vm_it)
		{
			const vm_pointer p_vm(*vm_it);

			const ::std::vector<virtual_machine_performance_category> metrics = virtual_machine_performance_categories();
			for (std::size_t i = 0; i < metrics.size(); ++i)
			{
				virtual_machine_performance_category metric = metrics[i];

				switch (metric)
				{
					case cpu_util_virtual_machine_performance:
						vm_perfs_summary_[p_vm->id()][metric](p_vm->cpu_share());
						vm_perfs_quantile_[p_vm->id()][metric](p_vm->cpu_share());
						break;
					case memory_util_virtual_machine_performance:
						vm_perfs_summary_[p_vm->id()][metric](p_vm->memory_share());
						vm_perfs_quantile_[p_vm->id()][metric](p_vm->memory_share());
						break;
				}
			}
		}

		::std::vector<application_performance_category> metrics = exp.manager().target_metrics();
		for (::std::size_t i = 0; i < metrics.size(); ++i)
		{
			application_performance_category metric = metrics[i];

			app_perfs_summary_[exp.app().id()][metric](exp.manager().data_estimator(metric).estimate());
			app_perfs_quantile_[exp.app().id()][metric](exp.manager().data_estimator(metric).estimate());
		}
	}

	private: void do_on_app_stop(app_experiment_type const& exp)
	{
		DCS_MACRO_SUPPRESS_UNUSED_VARIABLE_WARNING( exp );
	}

	private: void do_on_stop(sys_experiment_type const& exp)
	{
		DCS_MACRO_SUPPRESS_UNUSED_VARIABLE_WARNING( exp );
	}


	private: vm_summary_accumulator_map vm_perfs_summary_; 
	private: vm_quantile_accumulator_map vm_perfs_quantile_; 
	private: app_summary_accumulator_map app_perfs_summary_;
	private: app_quantile_accumulator_map app_perfs_quantile_;
};

}}} // Namespace dcs::testbed::utility

#endif // DCS_TESTBED_UTILITY_EXPERIMENT_STATS_GATHERER_HPP
