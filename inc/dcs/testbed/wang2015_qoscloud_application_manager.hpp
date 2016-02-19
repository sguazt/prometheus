/**
 * \file dcs/testbed/wang2015_qoscloud_application_manager.hpp
 *
 * \brief Application manager based on the QoSCloud solution proposed in
 * (Wang et al.,2012) and (Wang et al.,2015).
 *
 * References
 * -# Lixi Wang, Jing Xu, and Ming Zhao,
 *    "Modeling VM Performance Interference with Fuzzy MIMO Model,"
 *    Proc. of the 7th International Workshop on Feedback Computing (FeedbackComputing, co-held with ICAC2012), 2012.
 * -# Lixi Wang, Jing Xu, and Ming Zhao,
 *    "QoS-driven Cloud Resource Management through Fuzzy Model Predictive Control,"
 *    Proc. of the 12th International Conference on Autonomic Computing (ICAC), 2015.
 *
 * \author Marco Guazzone (marco.guazzone@gmail.com)
 *
 * <hr/>
 *
 * Copyright 2016   Marco Guazzone (marco.guazzone@gmail.com)
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

#ifndef DCS_TESTBED_WANG2015_QOSCLOUD_APPLICATION_MANAGER_HPP
#define DCS_TESTBED_WANG2015_QOSCLOUD_APPLICATION_MANAGER_HPP


#include <boost/algorithm/string/replace.hpp>
#include <boost/numeric/ublas/io.hpp>
#include <boost/numeric/ublas/matrix.hpp>
#include <boost/numeric/ublas/matrix_expression.hpp>
#include <boost/numeric/ublas/matrix_proxy.hpp>
#include <boost/numeric/ublas/vector.hpp>
#include <boost/numeric/ublas/vector_expression.hpp>
#include <boost/smart_ptr.hpp>
#include <boost/timer/timer.hpp>
#include <cmath>
#include <cstddef>
#include <dcs/assert.hpp>
#include <dcs/debug.hpp>
#include <dcs/exception.hpp>
#include <dcs/logging.hpp>
#include <dcs/math/function/round.hpp>
#include <dcs/math/traits/float.hpp>
#include <dcs/testbed/application_performance_category.hpp>
#include <dcs/testbed/base_application_manager.hpp>
#include <dcs/testbed/detail/matlab_utility.hpp>
//#include <dcs/testbed/data_estimators.hpp>
#include <dcs/testbed/virtual_machine_performance_category.hpp>
#include <deque>
#include <fl/anfis.h>
#include <fl/dataset.h>
#include <fl/Headers.h>
#include <fstream>
#include <limits>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>


namespace dcs { namespace testbed {

namespace wang2015_qoscloud_detail {

template <typename RealT>
class matlab_output_consumer
{
    public: matlab_output_consumer()
    : ok_(true)
    {
    }

    public: void operator()(dcs::system::posix_process& matlab_process)
    {
        ok_ = true;

        if (!matlab_process.alive())
        {
            ok_ = false;
            errmsg_ = "MATLAB is not running";
            return;
        }

        std::istream& is = matlab_process.output_stream();

//DCS_DEBUG_TRACE("BEGIN parsing MATLAB output");//XXX

        bool parse_line = false;
        while (matlab_process.alive() && is.good())
        {
//DCS_DEBUG_TRACE("Going to read another line: GOOD: " << is.good() << " - EOF: " << is.eof() << " - FAIL: " << is.fail() << " - BAD: " << is.bad() << " - !(): " << !static_cast<bool>(is) << "");//XXX
            std::string line;

            std::getline(is, line);

//DCS_DEBUG_TRACE("Read from MATLAB --> " << line);//XXX

            if (line.find("???") != std::string::npos
                || line.find("Error:") != std::string::npos)
            {
                DCS_DEBUG_TRACE("An error is occurred while executing MATLAB: " << line);//XXX
                ok_ = false;
                errmsg_ = line;
                break;
            }

            if (parse_line)
            {
                if (line.find("[/dcs::testbed::wang2015_qoscloud_application_manager]") != std::string::npos)
                {
                    // The end of parsable lines
                    parse_line = false;
                }
                else
                {
                    typename std::string::size_type pos;
                    
                    if ((pos = line.find("x=")) != std::string::npos)
                    {
                        detail::parse_matlab_str(line.substr(pos+2), x_);
//DCS_DEBUG_TRACE("Parsed as uopt=" << x_);//XXX
                    }
                    else if ((pos = line.find("fval=")) != std::string::npos)
                    {
                        detail::parse_matlab_str(line.substr(pos+5), fval_);
//DCS_DEBUG_TRACE("Parsed as fxopt=" << fval_);//XXX
                    }
                    else if ((pos = line.find("exitflag=")) != std::string::npos)
                    {
                        detail::parse_matlab_str(line.substr(pos+9), exitflag_);
//DCS_DEBUG_TRACE("Parsed as Uopt=" << exitflag_);//XXX
                    }
                }
            }
            else
            {
                if (line.find("[dcs::testbed::wang2015_qoscloud_application_manager]") != std::string::npos)
                {
                    // The beginning of parsable lines
                    parse_line = true;
                }
            }
//DCS_DEBUG_TRACE("Looping: GOOD: " << is.good() << " - EOF: " << is.eof() << " - FAIL: " << is.fail() << " - BAD: " << is.bad() << " - !(): " << !static_cast<bool>(is) << "");//XXX
        }

//DCS_DEBUG_TRACE("DONE WITH LOOPING");//XXX
//DCS_DEBUG_TRACE("END parsing MATLAB output");//XXX
//DCS_DEBUG_TRACE("IS state: " << is.good() << " - " << is.eof() << " - " << is.fail() << " - " << is.bad());//XXX
    }

    public: bool ok_;
    public: std::string errmsg_;
    public: boost::numeric::ublas::vector<RealT> x_;
    public: RealT fval_;
    public: int exitflag_;
}; // matlab_output_consumer

} // Namespace wang2015_qoscloud_detail


/**
 * \brief Application manager based on the QoS Cloud solution proposed in
 *  (Wang et al.,2012) and (Wang et al.,2015).
 *
 * Parameters:
 *  --- TODO ---
 *
 * \author Marco Guazzone (marco.guazzone@gmail.com)
 */
template <typename TraitsT>
class wang2015_qoscloud_application_manager: public base_application_manager<TraitsT>
{
    private: typedef base_application_manager<TraitsT> base_type;
    public: typedef typename base_type::traits_type traits_type;
    public: typedef typename traits_type::real_type real_type;
    public: typedef typename traits_type::uint_type uint_type;
    private: typedef typename base_type::app_type app_type;
    private: typedef typename base_type::app_pointer app_pointer;
    private: typedef typename base_type::vm_identifier_type vm_identifier_type;
    private: typedef typename app_type::sensor_type sensor_type;
    private: typedef typename app_type::sensor_pointer sensor_pointer;
    private: typedef std::map<application_performance_category,sensor_pointer> out_sensor_map;
    private: typedef std::map<virtual_machine_performance_category,std::map<vm_identifier_type,sensor_pointer> > in_sensor_map;


	private: static const std::size_t control_warmup_size;
	private: static const float resource_share_tol;


    public: wang2015_qoscloud_application_manager()
    : output_order_(1),
//      prediction_horizon_(1),
//      control_horizon_(1),
      forget_factor_(0.9),
      mpc_tracking_weight_(1),
      mpc_control_weight_(1),
      num_inputs_(0),
      num_outputs_(0),
      use_prebuilt_fis_(false),
      p_anfis_eng_(new fl::anfis::Engine()),
      p_anfis_builder_(new fl::SubtractiveClusteringFisBuilder<fl::anfis::Engine>()),
      p_anfis_trainer_(new fl::anfis::LeastSquaresLearningAlgorithm()),
	  beta_(0.9),
      ctrl_count_(0),
      ctrl_skip_count_(0),
      ctrl_fail_count_(0),
      anfis_initialized_(false)
    {
        init();
    }

    public: void output_order(std::size_t value)
    {
        output_order_ = value;
    }

    public: std::size_t output_order() const
    {
        return output_order_;
    }

//    public: void prediction_horizon(std::size_t value)
//    {
//        prediction_horizon_ = value;
//    }

//    public: std::size_t prediction_horizon() const
//    {
//        return prediction_horizon_;
//    }

//    public: void control_horizon(std::size_t value)
//    {
//        control_horizon_ = value;
//    }

//    public: std::size_t control_horizon() const
//    {
//        return control_horizon_;
//    }

    public: void forgetting_factor(real_type value)
    {
        forget_factor_ = value;
    }

    public: real_type forgetting_factor() const
    {
        return forget_factor_;
    }

    public: void mpc_tracking_weight(real_type value)
    {
        mpc_tracking_weight_ = value;
    }

    public: real_type mpc_tracking_weight() const
    {
        return mpc_tracking_weight_;
    }

    public: void mpc_control_weight(real_type value)
    {
        mpc_control_weight_ = value;
    }

    public: real_type mpc_control_weight() const
    {
        return mpc_control_weight_;
    }

    public: void export_data_to(std::string const& fname)
    {
        dat_fname_ = fname;
    }

    public: void use_prebuilt_anfis(bool value)
    {
        use_prebuilt_fis_ = value;
    }

    public: void prebuilt_anfis_file(std::string const& fname)
    {
        prebuilt_fis_fname_ = fname;
    }

    private: void do_reset()
    {
        typedef typename base_type::target_value_map::const_iterator target_iterator;
        typedef typename app_type::vm_pointer vm_pointer;

        vm_perf_cats_.clear();
        vm_perf_cats_.push_back(cpu_util_virtual_machine_performance);
        vm_perf_cats_.push_back(memory_util_virtual_machine_performance);

        const std::vector<vm_pointer> vms = this->app().vms();
        const std::size_t nvms = this->app().num_vms();
        const std::size_t num_vm_perf_cats = vm_perf_cats_.size();

        // Reset output sensors
        out_sensors_.clear();
        for (target_iterator tgt_it = this->target_values().begin(),
                             tgt_end_it = this->target_values().end();
             tgt_it != tgt_end_it;
             ++tgt_it)
        {
            const application_performance_category cat = tgt_it->first;

            out_sensors_[cat] = this->app().sensor(cat);
        }

        // Reset input history
        in_shares_.clear();
        in_shares_.resize(nvms);
        in_utils_.clear();
        in_utils_.resize(nvms);
        out_perf_history_.clear();

        // Reset counters
        ctrl_count_ = ctrl_skip_count_
                    = ctrl_fail_count_
                    = 0;

        // Computes number of system inputs/outputs
        num_inputs_ = nvms*num_vm_perf_cats;
        num_outputs_ = std::distance(this->target_values().begin(), this->target_values().end());

        // Reset fuzzy and MPC controller
        this->init_anfis();
        this->init_mpc();

        // Reset output data file
        if (p_dat_ofs_ && p_dat_ofs_->is_open())
        {
            p_dat_ofs_->close();
        }
        p_dat_ofs_.reset();
        if (!dat_fname_.empty())
        {
            p_dat_ofs_ = ::boost::make_shared< std::ofstream >(dat_fname_.c_str());
            if (!p_dat_ofs_->good())
            {
                std::ostringstream oss;
                oss << "Cannot open output data file '" << dat_fname_ << "'";

                DCS_EXCEPTION_THROW(std::runtime_error, oss.str());
            }

            *p_dat_ofs_ << "\"ts\"";

            for (std::size_t i = 0; i < nvms; ++i)
            {
				*p_dat_ofs_ << ",\"CPUCap_{" << vms[i]->id() << "}(k)\",\"CPUShare_{" << vms[i]->id() << "}(k)\""
							<< ",\"MemCap_{" << vms[i]->id() << "}(k)\",\"MemShare_{" << vms[i]->id() << "}(k)\"";
            }
			for (std::size_t i = 0; i < nvms; ++i)
			{
				*p_dat_ofs_ << ",\"CPUShare_{" << vms[i]->id() << "}(k-1)\""
							<< ",\"MemShare_{" << vms[i]->id() << "}(k-1)\"";
			}
			for (std::size_t i = 0; i < nvms; ++i)
			{
				*p_dat_ofs_ << ",\"CPUUtil_{" << vms[i]->id() << "}(k-1)\""
							<< ",\"MemUtil_{" << vms[i]->id() << "}(k-1)\"";
			}
            for (target_iterator tgt_it = this->target_values().begin(),
                                 tgt_end_it = this->target_values().end();
                 tgt_it != tgt_end_it;
                 ++tgt_it)
            {
                const application_performance_category cat = tgt_it->first;

                *p_dat_ofs_ << ",\"ReferenceOutput_{" << cat << "}(k-1)\",\"MeasuredOutput_{" << cat << "}(k-1)\",\"RelativeOutputError_{" << cat << "}(k-1)\"";
            }
            for (std::size_t i = 0,
                             ni = num_outputs_*output_order_ + num_inputs_;
                 i < ni;
                 ++i)
            {
                *p_dat_ofs_ << ",\"ANFIS Input_{" << i << "}\"";
            }
            for (std::size_t i = 0; i < num_outputs_; ++i)
            {
                *p_dat_ofs_ << ",\"ANFIS Output_{" << i << "}\"";
            }
            for (std::size_t i = 0; i < num_inputs_; ++i)
            {
                *p_dat_ofs_ << ",\"MPC Output_{" << i << "}\"";
            }
            *p_dat_ofs_ << ",\"# Controls\",\"# Skip Controls\",\"# Fail Controls\"";
            *p_dat_ofs_ << ",\"Elapsed Time\"";
            *p_dat_ofs_ << std::endl;
        }

        if (p_dat_ofs_)
        {
            // Reset input sensors
            in_sensors_.clear();
            for (std::size_t i = 0; i < nvms; ++i)
            {
                vm_pointer p_vm = vms[i];

                for (std::size_t j = 0; j < num_vm_perf_cats; ++j)
                {
                    const virtual_machine_performance_category cat = vm_perf_cats_[j];

                    in_sensors_[cat][p_vm->id()] = p_vm->sensor(cat);
                }
            }

            // Reset VM smoother
            for (::std::size_t i = 0; i < nvms; ++i)
            {
                const vm_pointer p_vm = vms[i];

                for (std::size_t j = 0; j < num_vm_perf_cats; ++j)
                {
                    const virtual_machine_performance_category cat = vm_perf_cats_[j];

                    this->data_smoother(cat, p_vm->id(), ::boost::make_shared< testbed::brown_single_exponential_smoother<real_type> >(beta_));
                }
            }
        }
    }

    private: void do_sample()
    {
        typedef typename in_sensor_map::const_iterator in_sensor_iterator;
        typedef typename out_sensor_map::const_iterator out_sensor_iterator;
        typedef std::vector<typename sensor_type::observation_type> obs_container;
        typedef typename obs_container::const_iterator obs_iterator;

        DCS_DEBUG_TRACE("(" << this << ") BEGIN Do SAMPLE - Count: " << ctrl_count_ << "/" << ctrl_skip_count_ << "/" << ctrl_fail_count_);

        if (p_dat_ofs_)
        {
            // Collect input values
            const in_sensor_iterator in_sens_end_it = in_sensors_.end();
            for (in_sensor_iterator in_sens_it = in_sensors_.begin();
                 in_sens_it != in_sens_end_it;
                 ++in_sens_it)
            {
                const virtual_machine_performance_category cat = in_sens_it->first;

                const typename in_sensor_map::mapped_type::const_iterator vm_end_it = in_sens_it->second.end();
                for (typename in_sensor_map::mapped_type::const_iterator vm_it = in_sens_it->second.begin();
                     vm_it != vm_end_it;
                     ++vm_it)
                {
                    const vm_identifier_type vm_id = vm_it->first;
                    sensor_pointer p_sens = vm_it->second;

                    // check: p_sens != null
                    DCS_DEBUG_ASSERT( p_sens );

                    p_sens->sense();
                    if (p_sens->has_observations())
                    {
                        const obs_container obs = p_sens->observations();
                        const obs_iterator end_it = obs.end();
                        for (obs_iterator it = obs.begin();
                             it != end_it;
                             ++it)
                        {
                            this->data_smoother(cat, vm_id).smooth(it->value());
                        }
                    }
                }
            }
        }

        // Collect output values
        const out_sensor_iterator out_sens_end_it = out_sensors_.end();
        for (out_sensor_iterator out_sens_it = out_sensors_.begin();
             out_sens_it != out_sens_end_it;
             ++out_sens_it)
        {
            const application_performance_category cat = out_sens_it->first;

            sensor_pointer p_sens = out_sens_it->second;

            // check: p_sens != null
            DCS_DEBUG_ASSERT( p_sens );

            p_sens->sense();
            if (p_sens->has_observations())
            {
                const obs_container obs = p_sens->observations();
                const obs_iterator end_it = obs.end();
                for (obs_iterator it = obs.begin();
                     it != end_it;
                     ++it)
                {
                    this->data_estimator(cat).collect(it->value());
                }
            }
        }

        DCS_DEBUG_TRACE("(" << this << ") END Do SAMPLE - Count: " << ctrl_count_ << "/" << ctrl_skip_count_ << "/" << ctrl_fail_count_);
    }

    private: void do_control()
    {
        namespace ublas = boost::numeric::ublas;
        //namespace ublasx = boost::numeric::ublasx;

        typedef typename base_type::target_value_map::const_iterator target_iterator;
        typedef typename app_type::vm_pointer vm_pointer;

        DCS_DEBUG_TRACE("(" << this << ") BEGIN Do CONTROL - Count: " << ctrl_count_ << "/" << ctrl_skip_count_ << "/" << ctrl_fail_count_);

        boost::timer::cpu_timer cpu_timer;

        const std::size_t num_vm_perf_cats = vm_perf_cats_.size();

        ++ctrl_count_;

        bool skip_ctrl = false;
        bool skip_collect = false;

        std::vector<real_type> new_xshares;
		std::map<virtual_machine_performance_category,std::vector<real_type> > old_xshares;

        std::vector<vm_pointer> vms = this->app().vms();
        const std::size_t nvms = vms.size();

        // Makes sure there is some data to collect for control
        for (target_iterator tgt_it = this->target_values().begin(),
                             tgt_end_it = this->target_values().end();
             tgt_it != tgt_end_it;
             ++tgt_it)
        {
            const application_performance_category cat = tgt_it->first;

            // Compute a summary statistics of collected observation
            if (this->data_estimator(cat).count() == 0)
            {
                    skip_ctrl = true;
                    skip_collect = true;
                    DCS_DEBUG_TRACE("No output observation collected during the last control interval -> Skip control");
                    break;
            }
        }

        // Collects data for control
        if (!skip_collect)
        {
            for (std::size_t i = 0; i < nvms; ++i)
            {
                for (std::size_t j = 0; j < num_vm_perf_cats; ++j)
                {
                    const virtual_machine_performance_category cat = vm_perf_cats_[j];
                    const vm_pointer p_vm = vms[i];

                    real_type c = 0;
                    switch (cat)
                    {
                        case cpu_util_virtual_machine_performance:
                            c = p_vm->cpu_share();
                            break;
                        case memory_util_virtual_machine_performance:
                            c = p_vm->memory_share();
                            break;
                    }
                    in_shares_[i][cat] = c;
                    in_utils_[i][cat] = this->data_smoother(cat, p_vm->id()).forecast(0);
DCS_DEBUG_TRACE("VM " << p_vm->id() << " - Performance Category: " << cat << " - Uhat(k): " << in_utils_[i].at(cat) << " - C(k): " << c);//XXX
                }
            }

            for (target_iterator tgt_it = this->target_values().begin(),
                                 tgt_end_it = this->target_values().end();
                 tgt_it != tgt_end_it;
                 ++tgt_it)
            {
                const application_performance_category cat = tgt_it->first;

                // Compute a summary statistics of collected observation
                const real_type yh = this->data_estimator(cat).estimate();
                const real_type yr = this->target_value(cat);

#ifdef DCS_DEBUG
                real_type err = 0;
                switch (cat)
                {
                    case response_time_application_performance:
                        err = (yr-yh)/yr;
                        break;
                    case throughput_application_performance:
                        err = (yh-yr)/yr;
                        break;
                }
DCS_DEBUG_TRACE("APP Performance Category: " << cat << " - Yhat(k): " << yh << " - R: " << yr << " -> E(k+1): " << err);//XXX
#endif // DCS_DEBUG

                // NOTE: we need to collect (output_order_+1) number of output
                //       observations in order to form input/output data needed
                //       to train the ANFIS model.

//[XXX]
{
std::cerr << "OUT_PERF_HISTORY - BEFORE: [";
for (std::size_t k = 0; k < out_perf_history_[cat].size(); ++k)
{
    std::cerr << ", " << out_perf_history_[cat].at(k);
}
std::cerr << "]" << std::endl;
}
//[/XXX]

                if (out_perf_history_[cat].size() >= (output_order_+1))
                {
                    out_perf_history_[cat].pop_front();
                }
                out_perf_history_[cat].push_back(yh);

//[XXX]
{
std::cerr << "OUT_PERF_HISTORY - AFTER: [";
for (std::size_t k = 0; k < out_perf_history_[cat].size(); ++k)
{
    std::cerr << ", " << out_perf_history_[cat].at(k);
}
std::cerr << "]" << std::endl;
}
//[/XXX]

                if (out_perf_history_[cat].size() < (output_order_+1))
                {
                    // Not enough outputs have been collected -> not ready for control yet
                    skip_ctrl = true;
                }

#ifdef DCSXX_TESTBED_EXP_APP_MGR_RESET_ESTIMATION_EVERY_INTERVAL
                this->data_estimator(cat).reset();
#endif // DCSXX_TESTBED_EXP_APP_MGR_RESET_ESTIMATION_EVERY_INTERVAL
            }
        }

		// Skip control until we see enough observations.
		// This should give enough time to let the estimated performance metric
		// (e.g., 95th percentile of response time) stabilize
		if (ctrl_count_ <= control_warmup_size)
		{
			skip_ctrl = true;
		}

        if (!skip_ctrl)
        {
            // Update ANFIS model

            this->update_anfis_model();

            if (!anfis_initialized_)
            {
                skip_ctrl = true;
            }
        }

        if (!skip_ctrl)
        {
            // Perform MPC control

            bool ok = false;

            try
            {
                new_xshares = this->perform_mpc_control();

                ok = true;
            }
            catch (std::exception const& e)
            {
                DCS_DEBUG_TRACE( "Caught exception: " << e.what() );

                std::ostringstream oss;
                oss << "Unable to compute optimal control: " << e.what();
                ::dcs::log_warn(DCS_LOGGING_AT, oss.str());
            }

            // Apply control results
            if (ok)
            {
                std::size_t k = 0;
                for (std::size_t i = 0; i < nvms; ++i)
                {
                    vm_pointer p_vm = vms[i];

                    for (std::size_t j = 0; j < num_vm_perf_cats; ++j)
                    {
                        const virtual_machine_performance_category cat = vm_perf_cats_[j];

                        real_type old_share = 0;
                        switch (cat)
                        {
                            case cpu_util_virtual_machine_performance:
                                old_share = p_vm->cpu_share();
                                break;
                            case memory_util_virtual_machine_performance:
                                old_share = p_vm->memory_share();
                                break;
                        }
                        old_xshares[cat].push_back(old_share);

                        real_type new_share = std::max(std::min(new_xshares[k], 1.0), 0.0);
					    new_share = dcs::math::round(new_share/resource_share_tol)*resource_share_tol;

                        DCS_DEBUG_TRACE("VM '" << p_vm->id() << "' - Performance Category: " << cat << " - old-share: " << old_share << " - new-share: " << new_share);

                        if (std::isfinite(new_share) && !dcs::math::float_traits<real_type>::essentially_equal(old_share, new_share, resource_share_tol))
                        {
                            switch (cat)
                            {
                                case cpu_util_virtual_machine_performance:
                                    p_vm->cpu_share(new_share);
                                    break;
                                case memory_util_virtual_machine_performance:
                                    p_vm->memory_share(new_share);
                                    break;
                            }
                            new_xshares[k] = new_share;
DCS_DEBUG_TRACE("VM " << vms[i]->id() << ", Performance Category: " << cat << " -> C(k+1): " << new_share);//XXX
                        }
                        else
                        {
                            new_xshares[k] = old_share;
DCS_DEBUG_TRACE("VM " << vms[i]->id() << ", Performance Category: " << cat << " -> C(k+1): not set!");//XXX
                        }

                        ++k;
                    }
                }
DCS_DEBUG_TRACE("Control applied");//XXX
            }
            else
            {
                ++ctrl_fail_count_;

                std::ostringstream oss;
                oss << "Control not applied: failed to solve the control problem";
                ::dcs::log_warn(DCS_LOGGING_AT, oss.str());
            }
        }
        else
        {
            ++ctrl_skip_count_;
        }

        cpu_timer.stop();

        // Export to file
        if (p_dat_ofs_)
        {
			// Initialize data structures if needed

			if (new_xshares.size() == 0)
			{
				for (::std::size_t i = 0; i < nvms; ++i)
				{
					const vm_pointer p_vm = vms[i];

					// check: p_vm != null
					DCS_DEBUG_ASSERT( p_vm );

					new_xshares.push_back(p_vm->cpu_share());
					new_xshares.push_back(p_vm->memory_share());
				}
			}
			if (old_xshares.size() == 0)
			{
				for (::std::size_t i = 0; i < nvms; ++i)
				{
					const vm_pointer p_vm = vms[i];

					// check: p_vm != null
					DCS_DEBUG_ASSERT( p_vm );

					old_xshares[cpu_util_virtual_machine_performance].push_back(p_vm->cpu_share());
					old_xshares[memory_util_virtual_machine_performance].push_back(p_vm->memory_share());
				}
			}

            // Write to data file

            *p_dat_ofs_ << std::time(0) << ",";
            for (std::size_t i = 0; i < nvms; ++i)
            {
                const vm_pointer p_vm = vms[i];

                // check: p_vm != null
                DCS_DEBUG_ASSERT( p_vm );

                if (i != 0)
                {
                    *p_dat_ofs_ << ",";
                }
				*p_dat_ofs_ << p_vm->cpu_cap() << "," << p_vm->cpu_share()
							<< "," << p_vm->memory_cap() << "," << p_vm->memory_share();
            }
            *p_dat_ofs_ << ",";
            for (std::size_t i = 0; i < nvms; ++i)
            {
                if (i != 0)
                {
                    *p_dat_ofs_ << ",";
                }
                *p_dat_ofs_ << old_xshares.at(cpu_util_virtual_machine_performance)[i]
                            << "," << old_xshares.at(memory_util_virtual_machine_performance)[i];
            }
            *p_dat_ofs_ << ",";
            for (std::size_t i = 0; i < nvms; ++i)
            {
                const vm_pointer p_vm = vms[i];

                // check: p_vm != null
                DCS_DEBUG_ASSERT( p_vm );

                if (i != 0)
                {
                    *p_dat_ofs_ << ",";
                }
				for (std::size_t j = 0; j < num_vm_perf_cats; ++j)
				{
					const virtual_machine_performance_category vm_cat = vm_perf_cats_[j];
					const real_type uh = (in_utils_.size() > 0 && in_utils_[i].count(vm_cat) > 0)
                                         ? in_utils_[i].at(vm_cat)
                                         : std::numeric_limits<real_type>::quiet_NaN();

                    if (j != 0)
                    {
                        *p_dat_ofs_ << ",";
                    }
					//*p_dat_ofs_ << this->data_smoother(vm_cat, p_vm->id()).forecast(0);
					*p_dat_ofs_ << uh;
				}
                //*p_dat_ofs_ << this->data_smoother(cpu_util_virtual_machine_performance, p_vm->id()).forecast(0)
                //            << "," << this->data_smoother(memory_util_virtual_machine_performance, p_vm->id()).forecast(0);
            }
            *p_dat_ofs_ << ",";
            for (target_iterator tgt_it = this->target_values().begin(),
                                 tgt_end_it = this->target_values().end();
            tgt_it != tgt_end_it;
            ++tgt_it)
            {
                const application_performance_category cat = tgt_it->first;

                if (tgt_it != this->target_values().begin())
                {
                    *p_dat_ofs_ << ",";
                }
                const real_type yh = this->data_estimator(cat).estimate();
                const real_type yr = tgt_it->second;
                real_type err = 0;
                switch (cat)
                {
                    case response_time_application_performance:
                        err = (yr-yh)/yr;
                        break;
                    case throughput_application_performance:
                        err = (yh-yr)/yr;
                        break;
                }
                *p_dat_ofs_ << yr << "," << yh << "," << err;
            }
            for (std::size_t i = 0,
                             ni = p_anfis_eng_->numberOfInputVariables();
                 i < ni;
                 ++i)
            {
                *p_dat_ofs_ << "," << p_anfis_eng_->getInputVariable(i)->getValue();
            }
            for (std::size_t i = 0,
                             ni = p_anfis_eng_->numberOfOutputVariables();
                 i < ni;
                 ++i)
            {
                *p_dat_ofs_ << "," << p_anfis_eng_->getOutputVariable(i)->getValue();
            }
            if (new_xshares.size() > 0)
            {
                for (std::size_t i = 0; i < num_inputs_; ++i)
                {
                    *p_dat_ofs_ << "," << new_xshares[i];
                }
            }
            else
            {
                for (std::size_t i = 0; i < num_inputs_; ++i)
                {
                    *p_dat_ofs_ << ","; // << std::numeric_limits<real_type>::quiet_NaN();
                }
            }
            *p_dat_ofs_ << "," << ctrl_count_ << "," << ctrl_skip_count_ << "," << ctrl_fail_count_;
            *p_dat_ofs_ << "," << (cpu_timer.elapsed().user+cpu_timer.elapsed().system);
            *p_dat_ofs_ << std::endl;
        }

        DCS_DEBUG_TRACE("(" << this << ") END Do CONTROL - Count: " << ctrl_count_ << "/" << ctrl_skip_count_ << "/" << ctrl_fail_count_);
    }

    private: void init()
    {
        //vm_perf_cats_.clear();
        //vm_perf_cats_.push_back(cpu_util_virtual_machine_performance);
        //vm_perf_cats_.push_back(memory_util_virtual_machine_performance);

        // Reset fuzzy and MPC controller
        this->init_anfis();
        this->init_mpc();
    }

    private: void init_anfis()
    {
        DCS_DEBUG_ASSERT( p_anfis_builder_ );
        DCS_DEBUG_ASSERT( p_anfis_eng_ );
        DCS_DEBUG_ASSERT( p_anfis_trainer_ );

        fl::fuzzylite::setDecimals(std::numeric_limits<fl::scalar>::digits10+1);
        fl::fuzzylite::setMachEps(std::numeric_limits<fl::scalar>::epsilon());

        p_anfis_eng_->clear();

        if (use_prebuilt_fis_)
        {
            //std::ostringstream oss;
            //oss << "experiments/data/rubis-wang2015_qoscloud-order"
            //    << "_out_" << output_order_
            //    //<< "-users_30_45_15-sig_mesh_a05_f12_p3_b05_lb02_ub1"
            //    //<< "-users_45-sig_mesh_a05_f12_p3_b05_lb02_ub1"
            //    << "-users_45-sig_constant_v1_lb02_ub1"
            //    << "-anfis_trained.fll";
            //const std::string prebuilt_fis_fname(oss.str());

            fl::FllImporter fllImp;
            fl::Engine* p_eng = fllImp.fromFile(prebuilt_fis_fname_);

            DCS_ASSERT(p_eng,
                       DCS_EXCEPTION_THROW(std::runtime_error, "Unable to initialize ANFIS fuzzy controller"));

            *p_anfis_eng_ = fl::anfis::Engine(*p_eng);

            delete p_eng;

            anfis_initialized_ = true;
        }
        else
        {
            anfis_initialized_ = false;
        }

        p_anfis_trainer_->setIsOnline(true);
        p_anfis_trainer_->setForgettingFactor(forget_factor_);
        p_anfis_trainer_->setEngine(p_anfis_eng_.get());
        p_anfis_trainer_->reset();

        const std::size_t nxi = output_order_*num_outputs_;
        anfis_trainset_ = fl::DataSet<real_type>(nxi+num_inputs_, num_outputs_);
    }

    private: void init_mpc()
    {
        // Empty
    }

    private: void update_anfis_model()
    {
        //
        // Builds the input vector for the ANFIS model
        // The input vector is formed by the concatenation of two vectors $\xi(k)$ and $u(k)$ where:
        // - $\xi(k)$ is the regressor vector at time $k$, with
        //   \[
        //    \xi(k) = [y_1(k), ..., y_1(k-n_a+1), ..., y_{n_y}(k), ...,  y_{n_y}(k-n_a+1)]^T
        //   \]
        //   where:
        //   * $U_{i,j}(k)$ and $C_{i,j}(k)$ are the utilization and capacity of resource $j$ of tier $i$ at time $k$, respectively,
        //   * $n_a$ and $n_b$ are the input and output order of the model, respectively.
        // - $u(k)$ is the system input vector at time $k$, with:
        //   \[
        //    u(k) = [C_{1,1}(k), ..., C_{1,R}(k), ..., C_{T,1}(k), ..., C_{T,R}(k)]^T
        //   \]
        // This is equivalent to see the ANFIS model as a NARX(n_a,n_b) model
        // \[
        //  y(k+1) = R(\xi(k),u(k))
        // \]
        // where $y(k)$ is the output of the ANFIS model at time $k$.
        //

        namespace ublas = boost::numeric::ublas;

        typedef typename base_type::target_value_map::const_iterator target_iterator;

        // Forms the $\xi(k-1)$, $u(k-1)$, and $y(k)$ vectors
        const std::size_t num_vm_perf_cats = vm_perf_cats_.size();
        const std::size_t nvms = this->app().num_vms();
        const std::size_t nxi = output_order_*num_outputs_;
        ublas::vector<real_type> u(num_inputs_, 0);
        ublas::vector<real_type> xi(nxi, 0);
        //ublas::vector<real_type> y(num_outputs_, 0);
        std::size_t u_ix = 0;
        std::size_t xi_ix = 0;
        //std::size_t y_ix = 0;
        ublas::vector<real_type> u_train(num_inputs_, 0);
        ublas::vector<real_type> xi_train(nxi, 0);
        ublas::vector<real_type> y_train(num_outputs_, 0);
        std::size_t u_ix_train = 0;
        std::size_t xi_ix_train = 0;
        std::size_t y_ix_train = 0;
DCS_DEBUG_TRACE("BUILDING XIs...");//XXX
        for (target_iterator tgt_it = this->target_values().begin(),
                             tgt_end_it = this->target_values().end();
             tgt_it != tgt_end_it;
             ++tgt_it)
        {
            const application_performance_category cat = tgt_it->first;

            for (typename std::deque<real_type>::const_reverse_iterator val_it = out_perf_history_.at(cat).rbegin(),
                                                                        val_end_it = out_perf_history_.at(cat).rend();
                 val_it != val_end_it;
                 ++val_it)
            {
                if (val_it == out_perf_history_.at(cat).rbegin())
                {
                    y_train(y_ix_train++) = *val_it;
DCS_DEBUG_TRACE("Y_TRAIN[" << (y_ix_train-1) << "]: " << y_train(y_ix_train-1));//XXX
                }
                else
                {
                    xi_train(xi_ix_train++) = *val_it;
DCS_DEBUG_TRACE("XI_TRAIN[" << (xi_ix_train-1) << "]: " << xi_train(xi_ix_train-1));//XXX
                }
                if (xi_ix < nxi)
                {
                    xi(xi_ix++) = *val_it;
DCS_DEBUG_TRACE("XI[" << (xi_ix-1) << "]: " << xi(xi_ix-1));//XXX
                }
            }
        }
DCS_DEBUG_TRACE("BUILDING Us...");//XXX
        for (std::size_t i = 0; i < nvms; ++i)
        {
            for (std::size_t j = 0; j < num_vm_perf_cats; ++j)
            {
                const virtual_machine_performance_category cat = vm_perf_cats_[j];

#if 1 // Use shares as resource usage. This seems the solution chosen by Lama
                u_train(u_ix_train++) = in_shares_[i].at(cat);
                u(u_ix++) = in_shares_[i].at(cat);
#else // Use utilizations as resource usage
                //u_train(u_ix_train++) = in_utils_[i].at(cat);
                //u(u_ix++) = in_utils_[i].at(cat);
#endif
DCS_DEBUG_TRACE("U_TRAIN[" << (u_ix_train-1) << "]: " << u_train(u_ix_train-1));//XXX
DCS_DEBUG_TRACE("U[" << (u_ix-1) << "]: " << u(u_ix-1));//XXX
            }
        }

        // post conditions
        DCS_DEBUG_ASSERT( xi_ix_train == nxi );
        DCS_DEBUG_ASSERT( u_ix_train == num_inputs_ );
        DCS_DEBUG_ASSERT( y_ix_train == num_outputs_ );

        {
            ublas::vector<real_type> inputs(nxi+num_inputs_, 0);
            ublas::subrange(inputs, 0, nxi) = xi_train;
            ublas::subrange(inputs, nxi, inputs.size()) = u_train;
            fl::DataSetEntry<real_type> entry(inputs.begin(), inputs.end(), y_train.begin(), y_train.end());
            anfis_trainset_.add(entry);
//[XXX]
{
std::cerr << "ANFIS - TRAINING INSTANCE: <";
std::cerr << "IN: [";
for (std::size_t i = 0; i < inputs.size(); ++i)
{
    std::cerr << ", " << inputs[i];
}
std::cerr << "], OUT: [";
for (std::size_t i = 0; i < num_outputs_; ++i)
{
    std::cerr << ", " << y_train[i];
}
std::cerr << "]>" << std::endl;
}
//[/XXX]
        }

        if (anfis_initialized_)
        {
            // Train the ANFIS model
            real_type rmse = 0;
//[FIXME]
#if 1
            const std::size_t min_trainset_size_online = 1;
            const std::size_t min_trainset_size_offline = 10;
#else
            const std::size_t min_trainset_size_online = 10;
            const std::size_t min_trainset_size_offline = 10;
#endif
//[/FIXME]
            if ((p_anfis_trainer_->isOnline() && anfis_trainset_.size() >= min_trainset_size_online)
                || anfis_trainset_.size() >= min_trainset_size_offline)
            {
                //rmse = p_anfis_trainer_->train(anfis_trainset_);
                rmse = p_anfis_trainer_->trainSingleEpoch(anfis_trainset_);
//[XXX]
{
std::ostringstream oss;
oss << "wang2015_qoscloud_trainset_n" << ctrl_count_ << ".dat";
std::ofstream ofs(oss.str().c_str());
fl::detail::MatrixOutput(ofs, anfis_trainset_.data());
ofs.close();
}
//[/XXX]
                anfis_trainset_.clear();
DCS_DEBUG_TRACE("ANFIS TRAINED -> RMSE: " << rmse);//XXX
            }

            DCS_DEBUG_ASSERT( p_anfis_eng_->numberOfInputVariables() == (nxi+num_inputs_) );
            DCS_DEBUG_ASSERT( p_anfis_eng_->numberOfOutputVariables() == num_outputs_ );

            // Load the $\xi$ and $u$ vector into the ANFIS model
            for (std::size_t i = 0; i < nxi; ++i)
            {
                //p_anfis_eng_->getInputVariable(i)->setValue(xi_train(i));
                p_anfis_eng_->getInputVariable(i)->setValue(xi(i));
            }
            for (std::size_t i = 0; i < num_inputs_; ++i)
            {
                //p_anfis_eng_->getInputVariable(i+nxi)->setValue(u_train(i));
                p_anfis_eng_->getInputVariable(i+nxi)->setValue(u(i));
            }

            // Apply the inputs to the ANFIS model
            p_anfis_eng_->process();
//[XXX]
{
std::cerr << "ANFIS - PROCESS: <";
std::cerr << "IN: [";
for (std::size_t i = 0; i < p_anfis_eng_->numberOfInputVariables(); ++i)
{
    std::cerr << ", " << p_anfis_eng_->getInputVariable(i)->getValue();
}
std::cerr << "], OUT: [";
for (std::size_t i = 0; i < p_anfis_eng_->numberOfOutputVariables(); ++i)
{
    std::cerr << ", " << p_anfis_eng_->getOutputVariable(i)->getValue();
}
std::cerr << "]>" << std::endl;
}
//[/XXX]
        }
        else
        {
            // Build the ANFIS model
            if (anfis_trainset_.size() >= 200)
            {
                FL_unique_ptr<fl::anfis::Engine> p_eng = p_anfis_builder_->build(anfis_trainset_);
                *p_anfis_eng_ = *p_eng;
                p_anfis_eng_->build();
                anfis_initialized_ = true;
                real_type rmse = 0;
                rmse = p_anfis_trainer_->train(anfis_trainset_, 20);
DCS_DEBUG_TRACE("ANFIS TRAINED FIRST TIME -> RMSE: " << rmse);//XXX
                anfis_trainset_.clear();
            }
        }
    }

    private: std::vector<real_type> perform_mpc_control()
    {
        namespace ublas = boost::numeric::ublas;

        typedef typename base_type::target_value_map::const_iterator target_iterator;

        const std::size_t nxi = output_order_*num_outputs_;
        const std::size_t nu = num_inputs_;
        const std::size_t ny = p_anfis_eng_->numberOfOutputVariables();

        // Retrieves the $\xi$ and $u$ vector from the ANFIS model
        ublas::vector<real_type> xi(nxi, 0);
        ublas::vector<real_type> u(nu, 0);
        for (std::size_t i = 0,
                         ni = nxi+nu;
             i < ni;
             ++i)
        {
            if (i < nxi)
            {
                xi(i) = p_anfis_eng_->getInputVariable(i)->getValue();
            }
            else
            {
                u(i-nxi) = p_anfis_eng_->getInputVariable(i)->getValue();
            }
        }

        // Builds the vector of output reference values
        ublas::vector<real_type> yref(num_outputs_, 0);
        {
            std::size_t i = 0;
            for (target_iterator tgt_it = this->target_values().begin(),
                                 tgt_end_it = this->target_values().end();
                 tgt_it != tgt_end_it;
                 ++tgt_it)
            {
                const application_performance_category cat = tgt_it->first;

                // Compute a summary statistics of collected observation
                yref(i++) = this->target_value(cat);
            }
        }

        fl::FisExporter fisExp;
        ublas::vector<real_type> ydefaults(ny);
        // Cancel default output values since MATLAB don't use them (and raise an error if it finds them in the FIS file)
        for (std::size_t i = 0; i < ny; ++i)
        {
            ydefaults[i] = p_anfis_eng_->getOutputVariable(i)->getDefaultValue();
            p_anfis_eng_->getOutputVariable(i)->setDefaultValue(std::numeric_limits<real_type>::quiet_NaN());
        }
        std::string fis_str = fisExp.toString(p_anfis_eng_.get());
        // Restore default output values
        for (std::size_t i = 0; i < ny; ++i)
        {
            p_anfis_eng_->getOutputVariable(i)->setDefaultValue(ydefaults[i]);
        }
        boost::algorithm::replace_all(fis_str, "\n", "\\n");
        boost::algorithm::replace_all(fis_str, "'", "''");


        std::vector<std::string> matlab_args;

        matlab_args.push_back("-nodisplay");
        //matlab_args.push_back("-nojvm");
        matlab_args.push_back("-nodesktop");

        std::ostringstream oss;

        oss << "-r \""
            << " try"
            << "   xi = " << detail::to_matlab_str(xi) << ";"
            << "   u = " << detail::to_matlab_str(u) << ";"
            << "   yref = " << detail::to_matlab_str(yref) << ";"
            << "   ny = " << ny << ";"
            << "   nu = " << nu << ";"
            << "   nxi = " << nxi << ";"
            << "   Q = " << mpc_tracking_weight_ << "*eye(ny);"
            << "   R = " << mpc_control_weight_ << "*eye(nu);"
            << "   fisstr = sprintf('" << fis_str << "');"
            << "   fisfile = [tempname, '.fis'];"
            //<< "   save(fisfile, 'fisstr', '-ascii');"
            << "   fd = fopen(fisfile, 'w');"
            << "   fwrite(fd, fisstr);"
            << "   fclose(fd);"
            << "   fis = readfis(fisfile);"
            << "   nvar = nu+nxi;"
            << "   wsqr = @(x,w) x'*w*x;"
            << "   objfun = @(x) (wsqr((evalfis(x',fis)-yref),Q) + wsqr((x(nxi+1:end)'-u),R));" // We need to transpose x since it is represented as a row vector
            << "   x0 = [xi; u];"
            << "   gaopts = gaoptimset('InitialPopulation',[x0'], 'TimeLimit', " << (this->control_time()*0.5) << ");" // We need to transpose x0 since it is represented as a column vector
            << "   LB = zeros(size(x0));"
            << "   UB = ones(size(x0));"
            << "   rng(1, 'twister');" // For reproducibility
            << "   [x,fval,exitflag] = ga(objfun, nvar, [], [], [], [], LB, UB, [], gaopts);"
            << "   format long;"
            << "   disp('--- [dcs::testbed::wang2015_qoscloud_application_manager] ---');"
            << "   disp(['x =', mat2str(x)]);"
            << "   disp(['fval =', num2str(fval)]);"
            << "   disp(['exitflag =', num2str(exitflag)]);"
            << "   disp('--- [/dcs::testbed::wang2015_qoscloud_application_manager] ---');"
            << " catch me,"
            << "  disp(['??? Error: ', me.message]);"
            << " end;"
            << " quit force;"
            << "\"";

        matlab_args.push_back(oss.str());

        wang2015_qoscloud_detail::matlab_output_consumer<real_type> matlab_consumer;
        detail::run_matlab("matlab", matlab_args.begin(), matlab_args.end(), matlab_consumer);

        if (!matlab_consumer.ok_)
        {
            DCS_EXCEPTION_THROW(std::runtime_error, matlab_consumer.errmsg_);
        }

        ublas::vector<real_type> u_opt;
        int exit_status;
        u_opt = matlab_consumer.x_;
        exit_status = matlab_consumer.exitflag_;
DCS_DEBUG_TRACE("Optimal control from GA: " << u_opt);///XXX

        // Check exit status
        // From MATLAB documentation:
        //  1   Without nonlinear constraints — Average cumulative change in value of the fitness function over StallGenLimit generations is less than TolFun, and the constraint violation is less than TolCon.
        //      With nonlinear constraints — Magnitude of the complementarity measure (see Complementarity Measure) is less than sqrt(TolCon), the subproblem is solved using a tolerance less than TolFun, and the constraint violation is less than TolCon.
        //  2   Fitness limit reached and the constraint violation is less than TolCon.
        //  3   Value of the fitness function did not change in StallGenLimit generations and the constraint violation is less than TolCon.
        //  4   Magnitude of step smaller than machine precision and the constraint violation is less than TolCon.
        //  5   Minimum fitness limit FitnessLimit reached and the constraint violation is less than TolCon.
        //  0   Maximum number of generations Generations exceeded.
        // -1   Optimization terminated by an output function or plot function.
        // -2   No feasible point found.
        // -4   Stall time limit StallTimeLimit exceeded.
        // -5   Time limit TimeLimit exceeded.
        if (exit_status == -2)
        {
            DCS_EXCEPTION_THROW(std::runtime_error, "(GA optimization) No feasible point found");
        }

        return std::vector<real_type>(u_opt.begin(), u_opt.end());
    }


    private: std::size_t output_order_; ///< The number of past outputs to consider that can influence the current output
    //private: std::size_t prediction_horizon_; ///< The prediction horizon used by MPC
    //private: std::size_t control_horizon_; ///< The prediction horizon used by MPC
    private: real_type forget_factor_; ///< The forgetting factor used by the wRLS algorithm
    private: real_type mpc_tracking_weight_; ///< The weight to apply to the tracking part of the MPC objective function
    private: real_type mpc_control_weight_; ///< The weight to apply to the tracking part of the MPC objective function
    private: std::size_t num_inputs_; ///< The number of system inputs
    private: std::size_t num_outputs_; ///< The number of system outputs
    private: bool use_prebuilt_fis_; ///< `true` if ANFIS is initialized from a previously built FIS
    private: ::boost::shared_ptr<fl::anfis::Engine> p_anfis_eng_; ///< The fuzzy modeling engine based on ANFIS
    private: boost::shared_ptr<fl::SubtractiveClusteringFisBuilder<fl::anfis::Engine> > p_anfis_builder_; ///< Builder for the ANFIS model
    private: boost::shared_ptr<fl::anfis::LeastSquaresLearningAlgorithm> p_anfis_trainer_; ///< Training algorithm for the ANFIS model
    private: real_type beta_; ///< Smoothing factor for VM CPU and Memory utilization
    private: std::size_t ctrl_count_; ///< Number of times control function has been invoked
    private: std::size_t ctrl_skip_count_; ///< Number of times control has been skipped
    private: std::size_t ctrl_fail_count_; ///< Number of times control has failed
    private: in_sensor_map in_sensors_;
    private: out_sensor_map out_sensors_;
    private: std::string dat_fname_;
    private: std::string prebuilt_fis_fname_;
    private: ::boost::shared_ptr< std::ofstream > p_dat_ofs_;
    private: std::vector<virtual_machine_performance_category> vm_perf_cats_;
    //private: std::vector<application_performance_category> app_perf_cats_;
    private: std::vector< std::map<virtual_machine_performance_category, real_type> > in_shares_;
    private: std::vector< std::map<virtual_machine_performance_category, real_type> > in_utils_;
    private: std::map< application_performance_category, std::deque<real_type> > out_perf_history_;
    private: bool anfis_initialized_;
    private: fl::DataSet<real_type> anfis_trainset_;
}; // wang2015_qoscloud_application_manager

template <typename T>
const std::size_t wang2015_qoscloud_application_manager<T>::control_warmup_size = 5;

template <typename T>
const float wang2015_qoscloud_application_manager<T>::resource_share_tol = 1e-2;

}} // Namespace dcs::testbed

#endif // DCS_TESTBED_WANG2015_QOSCLOUD_APPLICATION_MANAGER_HPP

/* vim: set tabstop=4 expandtab shiftwidth=4 softtabstop=4: */
