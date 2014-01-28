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
#include <dcs/testbed/base_experiment_tracker.hpp>
#include <dcs/testbed/base_virtual_machine.hpp>
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
	private: typedef ::boost::accumulators::accumulator_set<real_type,
															::boost::accumulators::stats< ::boost::accumulators::tag::mean,
																						  ::boost::accumulators::tag::variance,
																						  ::boost::accumulators::tag::min,
																						  ::boost::accumulators::tag::max > > summary_accumulator_type;
	private: typedef ::boost::accumulators::accumulator_set<real_type,
															::boost::accumulators::stats< ::boost::accumulators::tag::p_square_quantile > > quantile_accumulator_type;
//															::boost::accumulators::stats< ::boost::accumulators::tag::extended_p_square_quantile > > quantile_accumulator_type;
	private: typedef ::std::map<vm_identifier_type,summary_accumulator_type> vm_summary_accumulator_map;
	private: typedef ::std::map<vm_identifier_type,quantile_accumulator_type> vm_quantile_accumulator_map;
	public: typedef typename app_experiment_type::identifier_type app_experiment_identifier_type;
	private: typedef ::std::map< app_experiment_identifier_type, ::std::map<application_performance_category,summary_accumulator_type> > app_summary_accumulator_map;
	private: typedef ::std::map< app_experiment_identifier_type, ::std::map<application_performance_category,quantile_accumulator_type> > app_quantile_accumulator_map;


	public: real_type app_mean(app_experiment_identifier_type id, application_performance_category cat)
	{
		return ::boost::accumulators::mean(app_perfs_summary_.at(id).at(cat));
	}

	public: real_type app_variance(app_experiment_identifier_type id, application_performance_category cat)
	{
		return ::boost::accumulators::variance(app_perfs_summary_.at(id).at(cat));
	}

	public: real_type app_min(app_experiment_identifier_type id, application_performance_category cat)
	{
		return ::boost::accumulators::min(app_perfs_summary_.at(id).at(cat));
	}

	public: real_type app_max(app_experiment_identifier_type id, application_performance_category cat)
	{
		return ::boost::accumulators::max(app_perfs_summary_.at(id).at(cat));
	}

	private: void do_reset()
	{
		vm_cpu_shares_summary_.clear();
		vm_cpu_shares_quantile_.clear();
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

		real_type probs[] = {0.25, 0.50, 0.75, 0.90, 0.95, 0.99};

		vm_container vms = exp.app().vms();

		vm_iterator vm_end_it = vms.end();
		for (vm_iterator vm_it = vms.begin();
			 vm_it != vm_end_it;
			 ++vm_it)
		{
			const vm_pointer p_vm(*vm_it);

			//vm_cpu_shares_[p_vm->id()] = accumulator_type(::boost::accumulators::tag::extended_p_square::probabilities = probs);
			vm_cpu_shares_summary_[p_vm->id()] = summary_accumulator_type();
			vm_cpu_shares_quantile_[p_vm->id()] = quantile_accumulator_type(::boost::accumulators::extended_p_square_probabilities = probs);
			//vm_cpu_shares_quantile_[p_vm->id()] = quantile_accumulator_type(::boost::accumulators::quantile_probability = 0.95);
		}

		//FIXME: application performance category is hard-coded
		app_perfs_summary_[exp.id()][response_time_application_performance] = summary_accumulator_type();
		app_perfs_quantile_[exp.id()][response_time_application_performance] = quantile_accumulator_type(::boost::accumulators::tag::extended_p_square::probabilities = probs);
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

			vm_cpu_shares_summary_[p_vm->id()](p_vm->cpu_share());
			vm_cpu_shares_quantile_[p_vm->id()](p_vm->cpu_share());
		}

		//FIXME: application performance category is hard-coded
		app_perfs_summary_[exp.id()][response_time_application_performance](exp.manager().data_estimator(response_time_application_performance).estimate());
		app_perfs_quantile_[exp.id()][response_time_application_performance](exp.manager().data_estimator(response_time_application_performance).estimate());
	}

	private: void do_on_app_stop(app_experiment_type const& exp)
	{
		DCS_MACRO_SUPPRESS_UNUSED_VARIABLE_WARNING( exp );
	}

	private: void do_on_stop(sys_experiment_type const& exp)
	{
		DCS_MACRO_SUPPRESS_UNUSED_VARIABLE_WARNING( exp );
	}


	private: vm_summary_accumulator_map vm_cpu_shares_summary_; 
	private: vm_quantile_accumulator_map vm_cpu_shares_quantile_; 
	private: app_summary_accumulator_map app_perfs_summary_;
	private: app_quantile_accumulator_map app_perfs_quantile_;
};

}}} // Namespace dcs::testbed::utility

#endif // DCS_TESTBED_UTILITY_EXPERIMENT_STATS_GATHERER_HPP
