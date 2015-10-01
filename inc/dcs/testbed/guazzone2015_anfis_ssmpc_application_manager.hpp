/**
 * \file dcs/testbed/guazzone2015_anfis_ssmpc_application_manager.hpp
 *
 * \brief Application manager based on the Adaptive Neural Fuzzy Inference
 *  System (ANFIS) and on the Model Predictive Control (MPC).
 *
 * For MPC, we use the single-step linearization technique.
 *
 * \author Marco Guazzone (marco.guazzone@gmail.com)
 *
 * <hr/>
 *
 * Copyright 2015   Marco Guazzone (marco.guazzone@gmail.com)
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

#ifndef DCS_TESTBED_GUAZZONE2015_ANFIS_SSMPC_APPLICATION_MANAGER_HPP
#define DCS_TESTBED_GUAZZONE2015_ANFIS_SSMPC_APPLICATION_MANAGER_HPP


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
#ifdef DCS_TESTBED_USE_MATLAB_LINEAR_MPC
# include <dcs/control/design/matlab_linear_mpc.hpp>
#else
# include <dcs/control/design/linear_mpc.hpp>
#endif // DCS_TESTBED_USE_MATLAB_LINEAR_MPC
#include <dcs/debug.hpp>
#include <dcs/exception.hpp>
#include <dcs/logging.hpp>
#include <dcs/math/traits/float.hpp>
#include <dcs/testbed/application_performance_category.hpp>
#include <dcs/testbed/base_application_manager.hpp>
//#include <dcs/testbed/data_estimators.hpp>
#include <dcs/testbed/data_smoothers.hpp>
#include <dcs/testbed/virtual_machine_performance_category.hpp>
#include <deque>
#include <fl/anfis.h>
#include <fl/dataset.h>
#include <fl/fis_builders.h>
#include <fl/Headers.h>
#include <fstream>
#include <limits>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>


namespace dcs { namespace testbed {

/**
 * \brief Application manager based on the work by (Guazzone et al., 2015)
 *
 * \author Marco Guazzone (marco.guazzone@gmail.com)
 */
template <typename TraitsT>
class guazzone2015_anfis_ssmpc_application_manager: public base_application_manager<TraitsT>
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


    public: guazzone2015_anfis_ssmpc_application_manager()
    : beta_(0.9),
      input_order_(1),
      aux_input_order_(1),
      output_order_(1),
      prediction_horizon_(20),
      control_horizon_(5),
      forget_factor_(0.9),
      mpc_tracking_weight_(1),
      mpc_control_weight_(1),
      num_inputs_(0),
      num_aux_inputs_(0),
      num_outputs_(0),
      p_anfis_eng_(new fl::anfis::Engine()),
      p_anfis_builder_(new fl::SubtractiveClusteringFisBuilder<fl::anfis::Engine>()),
      p_anfis_trainer_(new fl::anfis::Jang1993HybridLearningAlgorithm()),
#ifdef DCS_TESTBED_USE_MATLAB_LINEAR_MPC
      p_mpc_ctrl_(new dcs::control::matlab_linear_mpc_controller<real_type>()),
#else
      p_mpc_ctrl_(new dcs::control::linear_mpc_controller<real_type>()),
#endif // DCS_TESTBED_USE_MATLAB_LINEAR_MPC
      ctrl_count_(0),
      ctrl_skip_count_(0),
      ctrl_fail_count_(0),
      anfis_initialized_(false)
    {
        init();
    }

    public: void smoothing_factor(real_type value)
    {
        beta_ = value;
    }

    public: real_type smoothing_factor() const
    {
        return beta_;
    }

    public: void output_order(std::size_t value)
    {
        output_order_ = value;
    }

    public: std::size_t output_order() const
    {
        return output_order_;
    }

    public: void prediction_horizon(std::size_t value)
    {
        prediction_horizon_ = value;
    }

    public: std::size_t prediction_horizon() const
    {
        return prediction_horizon_;
    }

    public: void control_horizon(std::size_t value)
    {
        control_horizon_ = value;
    }

    public: std::size_t control_horizon() const
    {
        return control_horizon_;
    }

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

    private: void do_reset()
    {
        typedef typename base_type::target_value_map::const_iterator target_iterator;
        typedef typename app_type::vm_pointer vm_pointer;

        vm_perf_cats_.clear();
        vm_perf_cats_.push_back(cpu_util_virtual_machine_performance);
        vm_perf_cats_.push_back(memory_util_virtual_machine_performance);

        //app_perf_cats_.push_back(cpu_util_virtual_machine_performance);

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

        // Reset input sensors
        in_sensors_.clear();
        for (std::size_t i = 0; i < nvms; ++i)
        {
            for (std::size_t j = 0; j < num_vm_perf_cats; ++j)
            {
                const virtual_machine_performance_category cat = vm_perf_cats_[j];
                vm_pointer p_vm = vms[i];

                in_sensors_[cat][p_vm->id()] = p_vm->sensor(cat);
            }
        }

        // Reset input history
        in_cap_history_.clear();
        in_cap_history_.resize(nvms);
        in_util_history_.clear();
        in_util_history_.resize(nvms);
        out_perf_history_.clear();

        // Reset counters
        ctrl_count_ = ctrl_skip_count_
                    = ctrl_fail_count_
                    = 0;

        // Reset fuzzy controller
        this->init_anfis();
        this->init_mpc();

        // Computes number of system inputs/outputs
        num_inputs_ = num_aux_inputs_
                    = nvms*num_vm_perf_cats;
        num_outputs_ = std::distance(this->target_values().begin(), this->target_values().end());

        //// Reset Cres estimator and smoother
        //this->data_estimator(cpu_util_virtual_machine_performance).reset();
        //this->data_smoother(cpu_util_virtual_machine_performance).reset();
        for (std::size_t i = 0; i < nvms; ++i)
        {
            for (std::size_t j = 0; j < num_vm_perf_cats; ++j)
            {
                const virtual_machine_performance_category cat = vm_perf_cats_[j];
                //this->data_estimator(cat, vms[i]->id(), ::boost::make_shared< testbed::mean_estimator<real_type> >());
                this->data_smoother(cat, vms[i]->id(), ::boost::make_shared< testbed::brown_single_exponential_smoother<real_type> >(beta_));
                //this->data_smoother(cat, vms[i]->id(), ::boost::make_shared< testbed::holt_winters_double_exponential_smoother<real_type> >(beta_));
            }
        }

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
                *p_dat_ofs_ << ",\"CPUCap_{" << vms[i]->id() << "}\",\"CPUShare_{" << vms[i]->id() << "}\""
                            << ",\"MemCap_{" << vms[i]->id() << "}\",\"MemShare_{" << vms[i]->id() << "}\"";
            }
            for (target_iterator tgt_it = this->target_values().begin(),
                                 tgt_end_it = this->target_values().end();
                 tgt_it != tgt_end_it;
                 ++tgt_it)
            {
                const application_performance_category cat = tgt_it->first;

                *p_dat_ofs_ << ",\"ReferenceOutput_{" << cat << "}\",\"MeasureOutput_{" << cat << "}\",\"RelativeOutputError_{" << cat << "}\"";
            }
            for (std::size_t i = 0,
                             ni = num_outputs_*output_order_ + num_inputs_*input_order_ + num_aux_inputs_*aux_input_order_ + num_inputs_;
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
    }

    private: void do_sample()
    {
        typedef typename in_sensor_map::const_iterator in_sensor_iterator;
        typedef typename out_sensor_map::const_iterator out_sensor_iterator;
        typedef std::vector<typename sensor_type::observation_type> obs_container;
        typedef typename obs_container::const_iterator obs_iterator;

        DCS_DEBUG_TRACE("(" << this << ") BEGIN Do SAMPLE - Count: " << ctrl_count_ << "/" << ctrl_skip_count_ << "/" << ctrl_fail_count_);

        // Collect input values
        for (in_sensor_iterator in_sens_it = in_sensors_.begin(),
                                in_sens_end_it = in_sensors_.end();
             in_sens_it != in_sens_end_it;
             ++in_sens_it)
        {
            const virtual_machine_performance_category cat = in_sens_it->first;

            for (typename in_sensor_map::mapped_type::const_iterator vm_it = in_sens_it->second.begin(),
                                                                     vm_end_it = in_sens_it->second.end();
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
                        //this->data_estimator(cat, vm_id).collect(it->value());
                        this->data_smoother(cat, vm_id).smooth(it->value());
                    }
                }
            }
        }

        // Collect output values
        for (out_sensor_iterator out_sens_it = out_sensors_.begin(),
                                 out_sens_end_it = out_sensors_.end();
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

        std::vector<vm_pointer> vms = this->app().vms();
        const std::size_t nvms = vms.size();

        // Makes sure there is some data to collect for control
        for (std::size_t i = 0; i < nvms; ++i)
        {
            for (std::size_t j = 0; j < num_vm_perf_cats; ++j)
            {
                const virtual_machine_performance_category cat = vm_perf_cats_[j];
                const vm_pointer p_vm = vms[i];

                if (!this->data_smoother(cat, vms[i]->id()).ready())
                {
                    // No observation collected during the last control interval
                    skip_ctrl = true;
                    skip_collect = true;
                    DCS_DEBUG_TRACE("No input observation collected during the last control interval -> Skip control");
                    break;
                }
            }
        }
        if (!skip_collect)
        {
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

                    const real_type uh = this->data_smoother(cat, p_vm->id()).forecast(0);
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
DCS_DEBUG_TRACE("VM " << p_vm->id() << " - Performance Category: " << cat << " - Uhat(k): " << uh << " - C(k): " << c);//XXX

                    //if (in_cap_history_[i].at(cat).size() >= (input_order_+1))
                    if (in_cap_history_[i].count(cat) > 0 && in_cap_history_[i].at(cat).size() >= input_order_)
                    {
                        in_cap_history_[i].at(cat).pop_front();
                    }
                    in_cap_history_[i][cat].push_back(c);

                    //if (in_util_history_[i].at(cat).size() >= (aux_input_order_+1))
                    if (in_util_history_[i].count(cat) > 0 && in_util_history_[i].at(cat).size() >= aux_input_order_)
                    {
                        in_util_history_[i].at(cat).pop_front();
                    }
                    in_util_history_[i][cat].push_back(uh);

                    //if (in_cap_history_[i].at(cat).size() < (input_order_+1)
                    //  || in_util_history_[i].at(cat).size() < (input_order_+1))
                    if (in_cap_history_[i].at(cat).size() < input_order_
                        || in_util_history_[i].at(cat).size() < aux_input_order_)
                    {
                        // Not enough inputs have been collected -> not ready for control yet
                        skip_ctrl = true;
                    }
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
DCS_DEBUG_TRACE("APP Performance Category: " << cat << " - Yhat(k): " << yh << " - R: " << yr << " -> E(k): " << err);//XXX
#endif // DCS_DEBUG

                // NOTE: we need to collect (output_order_+1) number of output
                //       observations in order to form input/output data needed
                //       to train the ANFIS model.

                //if (out_perf_history_[cat].size() >= output_order_)
                if (out_perf_history_[cat].size() >= (output_order_+1))
                {
                    out_perf_history_[cat].pop_front();
                }
                out_perf_history_[cat].push_back(yh);

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

        if (!skip_ctrl)
        {
            this->update_anfis_model();

            if (!anfis_initialized_)
            {
                skip_ctrl = true;
            }
        }

        std::vector<real_type> new_shares;

        if (!skip_ctrl)
        {
            bool ok = false;

            try
            {
                new_shares = this->perform_mpc_control();

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

                        const real_type new_share = std::max(std::min(new_shares[k++], 1.0), 0.0);

                        DCS_DEBUG_TRACE("VM '" << p_vm->id() << "' - Performance Category: " << cat << " - old-share: " << old_share << " - new-share: " << new_share);

                        if (std::isfinite(new_share) && !dcs::math::float_traits<real_type>::essentially_equal(old_share, new_share))
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
DCS_DEBUG_TRACE("VM " << vms[i]->id() << ", Performance Category: " << cat << " -> C(k+1): " << new_share);//XXX
                        }
                    }
                }
DCS_DEBUG_TRACE("Optimal control applied");//XXX
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
            if (new_shares.size() > 0)
            {
                for (std::size_t i = 0; i < num_inputs_; ++i)
                {
                    *p_dat_ofs_ << "," << new_shares[i];
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
        vm_perf_cats_.clear();
        vm_perf_cats_.push_back(cpu_util_virtual_machine_performance);
        vm_perf_cats_.push_back(memory_util_virtual_machine_performance);

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

#if 1
        std::ostringstream oss;
        oss << "experiments/data/rubis-guazzone2015_anfis_ssmpc-order"
            << "_in_" << input_order_
            << "_auxin_" << aux_input_order_
            << "_out_" << output_order_
            //<< "-users_30_45_15-sig_mesh_a05_f12_p3_b05_lb02_ub1"
            //<< "-users_45-sig_mesh_a05_f12_p3_b05_lb02_ub1"
            << "-users_45-sig_constant_v1_lb02_ub1"
            << "-anfis_trained.fll";
        const std::string prebuilt_fis_fname(oss.str());

        fl::FllImporter fllImp;
        fl::Engine* p_eng = fllImp.fromFile(prebuilt_fis_fname);

        DCS_ASSERT(p_eng,
                   DCS_EXCEPTION_THROW(std::runtime_error, "Unable to initialize ANFIS fuzzy controller"));

        *p_anfis_eng_ = fl::anfis::Engine(*p_eng);

        delete p_eng;

        anfis_initialized_ = true;
#else
        anfis_initialized_ = false;
#endif

        p_anfis_trainer_->setIsOnline(true);
        p_anfis_trainer_->setForgettingFactor(forget_factor_);
        p_anfis_trainer_->setEngine(p_anfis_eng_.get());
        p_anfis_trainer_->reset();

        const std::size_t nxi = output_order_*num_outputs_ + (input_order_ > 0 ? num_inputs_*(input_order_-1) : 0) + num_aux_inputs_*aux_input_order_;
        anfis_trainset_ = fl::DataSet<real_type>(nxi+num_inputs_, num_outputs_);
    }

    private: void init_mpc()
    {
        DCS_DEBUG_ASSERT( p_mpc_ctrl_ );

        p_mpc_ctrl_->reset();
    }

    private: void update_anfis_model()
    {
        //
        // Builds the input vector for the ANFIS model
        // The input vector is formed by the concatenation of two vectors $\xi(k)$ and $u(k)$ where:
        // - $\xi(k)$ is the regressor vector at time $k$, with
        //   \[
        //    \xi(k) = [y_1(k), ..., y_1(k-n_a+1), ..., y_{n_y}(k), ...,  y_{n_y}(k-n_a+1),
        //              U_{1,1}(k), ..., U_{1,1}(k-n_b+1), ..., U_{1,R}(k), ..., U_{1,R}(k-n_b+1), U_{T,1}(k), ..., U_{T,1}(k-n_b+1), ..., U_{T,R}(k), ..., U_{T,R}(k-n_b+1), 
        //              C_{1,1}(k-1), ..., C_{1,1}(k-n_b+1), ..., C_{1,R}(k-1), ..., C_{1,R}(k-n_b+1), C_{T,1}(k-1), ..., C_{T,1}(k-n_b+1), ..., C_{T,R}(k-1), ..., C_{T,R}(k-n_b+1)]^T
        //   \]
        //   where:
        //   * $U_{i,j}(k)$ and $C_{i,j}(k)$ are the utilization and share of resource $j$ of tier $i$ at time $k$, respectively,
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
        const std::size_t nxi = output_order_*num_outputs_ + (input_order_ > 0 ? num_inputs_*(input_order_-1) : 0) + num_aux_inputs_*aux_input_order_;
        ublas::vector<real_type> u(num_inputs_, 0);
        ublas::vector<real_type> xi(nxi, 0);
        ublas::vector<real_type> y(num_outputs_, 0);
        std::size_t u_ix = 0;
        std::size_t xi_ix = 0;
        std::size_t y_ix = 0;
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
                    y(y_ix++) = *val_it;
                }
                else
                {
                    xi(xi_ix++) = *val_it;
                }
            }
        }
        for (std::size_t i = 0; i < nvms; ++i)
        {
            for (std::size_t j = 0; j < num_vm_perf_cats; ++j)
            {
                const virtual_machine_performance_category cat = vm_perf_cats_[j];

                for (typename std::deque<real_type>::const_reverse_iterator val_it = in_util_history_[i].at(cat).rbegin(),
                                                                            val_end_it = in_util_history_[i].at(cat).rend();
                     val_it != val_end_it;
                     ++val_it)
                {
                    //if (val_it == in_util_history_[i].at(cat).rbegin())
                    //{
                    //  // This value will go in u(k)
                    //  continue;
                    //}
                    xi(xi_ix++) = *val_it;
                }
                for (typename std::deque<real_type>::const_reverse_iterator val_it = in_cap_history_[i].at(cat).rbegin(),
                                                                            val_end_it = in_cap_history_[i].at(cat).rend();
                     val_it != val_end_it;
                     ++val_it)
                {
                    if (val_it == in_cap_history_[i].at(cat).rbegin())
                    {
                        u(u_ix++) = *val_it;
                    }
                    else
                    {
                        xi(xi_ix++) = *val_it;
                    }
                }
            }
        }

        // post conditions
        DCS_DEBUG_ASSERT( xi_ix == nxi );
        DCS_DEBUG_ASSERT( u_ix == num_inputs_ );
        DCS_DEBUG_ASSERT( y_ix == num_outputs_ );

        {
            ublas::vector<real_type> inputs(nxi+num_inputs_, 0);
            ublas::subrange(inputs, 0, nxi) = xi;
            ublas::subrange(inputs, nxi, inputs.size()) = u;
            fl::DataSetEntry<real_type> entry(inputs.begin(), inputs.end(), y.begin(), y.end());
            //fl::DataSet<real_type> trainset(nxi+num_inputs_, num_outputs_);
            anfis_trainset_.add(entry);
        }

        if (anfis_initialized_)
        {
            // Train the ANFIS model
            real_type rmse = 0;
#if 1
            const std::size_t min_trainset_size_online = 1;
            const std::size_t min_trainset_size_offline = 10;
#else
            const std::size_t min_trainset_size_online = 10;
            const std::size_t min_trainset_size_offline = 10;
#endif
            if ((p_anfis_trainer_->isOnline() && anfis_trainset_.size() >= min_trainset_size_online)
                || anfis_trainset_.size() >= min_trainset_size_offline)
            {
                rmse = p_anfis_trainer_->train(anfis_trainset_);
{
std::ostringstream oss;
oss << "rubis_guazzone2015anfis_trainset_n" << ctrl_count_ << ".dat";
std::ofstream ofs(oss.str().c_str());
fl::detail::MatrixOutput(ofs, anfis_trainset_.data());
ofs.close();
}
                anfis_trainset_.clear();
            }
DCS_DEBUG_TRACE("ANFIS TRAINED -> RMSE: " << rmse);//XXX

            DCS_DEBUG_ASSERT( p_anfis_eng_->numberOfInputVariables() == (nxi+num_inputs_) );
            DCS_DEBUG_ASSERT( p_anfis_eng_->numberOfOutputVariables() == num_outputs_ );

            // Load the $\xi$ and $u$ vector into the ANFIS model
            for (std::size_t i = 0; i < nxi; ++i)
            {
                p_anfis_eng_->getInputVariable(i)->setValue(xi(i));
            }
            for (std::size_t i = 0; i < num_inputs_; ++i)
            {
                p_anfis_eng_->getInputVariable(i+nxi)->setValue(u(i));
            }

            // Apply the inputs to the ANFIS model
            p_anfis_eng_->process();
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
#if 1
                // The first training is always performed as a batch (offline) training
                bool is_online = p_anfis_trainer_->isOnline();
                p_anfis_trainer_->setIsOnline(false);
                rmse = p_anfis_trainer_->train(anfis_trainset_, 20);
                p_anfis_trainer_->setIsOnline(is_online);
#else
                rmse = p_anfis_trainer_->train(anfis_trainset_, 20);
#endif
DCS_DEBUG_TRACE("ANFIS TRAINED FIRST TIME -> RMSE: " << rmse);//XXX
                anfis_trainset_.clear();
            }
        }
    }

    private: std::vector<real_type> perform_mpc_control()
    {
        namespace ublas = boost::numeric::ublas;

        typedef typename base_type::target_value_map::const_iterator target_iterator;

        //const std::size_t nxi = output_order_*num_outputs_ + (input_order_ > 0 ? ((num_inputs_+num_aux_inputs_)*(input_order_-1)+num_aux_inputs_) : 0);
        const std::size_t nxi = output_order_*num_outputs_ + aux_input_order_*num_aux_inputs_ + (input_order_ > 0 ? (input_order_-1)*num_inputs_ : 0);
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

//[XXX]
//{
//  fl::FisExporter fisExp;
//  fl::FllExporter fllExp;
//  std::ostringstream oss;
//  oss << "rubis_guazzone2015anfis_n" << ctrl_count_;
//  fisExp.toFile(oss.str() + ".fis", p_anfis_eng_.get());
//  fllExp.toFile(oss.str() + ".fll", p_anfis_eng_.get());
//}
//[/XXX]
        // Linearizes the fuzzy system for the MPC controller

        const std::size_t nzeta = nxi;
        const std::size_t neta = nu;
        const std::size_t ntheta = 1;

        ublas::matrix<real_type> Zeta_star(ny, nzeta, 0);
        ublas::matrix<real_type> Eta_star(ny, neta, 0);
        ublas::matrix<real_type> Theta_star(ny, ntheta, 0);

        for (std::size_t i = 0; i < num_outputs_; ++i)
        {
            const fl::Accumulated* p_fuzzyOutput = p_anfis_eng_->getOutputVariable(i)->fuzzyOutput();
            //const std::size_t numRules = p_fuzzyOutput->numberOfTerms();

            real_type wsum = 0;
            for (std::size_t j = 0,
                             nj = p_fuzzyOutput->numberOfTerms();
                 j < nj;
                 ++j)
            {
                const fl::Activated* p_activated = p_fuzzyOutput->getTerm(j);
                const real_type w = p_activated->getDegree();

                wsum += w;

                const std::vector<real_type> coeffs = dynamic_cast<const fl::Linear*>(p_activated->getTerm())->coefficients();

                for (std::size_t h = 0,
                                 nh = coeffs.size();
                     h < nh;
                     ++h)
                {
                    const fl::scalar value = w*coeffs[h];

                    if (h < nzeta)
                    {
                        Zeta_star(i,h) += value;
                    }
                    else if (h < (nzeta+neta))
                    {
                        Eta_star(i,h-nzeta) += value;
                    }
                    else
                    {
                        Theta_star(i,h-nzeta-neta) += value;
                    }
                }
            }
            if (wsum > 0)
            {
/*
                for (std::size_t j = 0; j < nzeta; ++j)
                {
                    Zeta_star(i,j) /= wsum;
                }
                for (std::size_t j = 0; j < neta; ++j)
                {
                    Eta_star(i,j) /= wsum;
                }
                for (std::size_t j = 0; j < ntheta; ++j)
                {
                    Theta_star(i,j) /= wsum;
                }
*/
                ublas::row(Zeta_star, i) /= wsum;
                ublas::row(Eta_star, i) /= wsum;
                ublas::row(Theta_star, i) /= wsum;
            }
            else
            {
                ::dcs::log_warn(DCS_LOGGING_AT, "Unable to compute ANFIS value: rule coverage problem");
            }
DCS_DEBUG_TRACE("OUTPUT #" << i << " - VALUE: " << p_anfis_eng_->getOutputVariable(i)->getValue() << " - FUZZY OUTPUT: " << p_fuzzyOutput->toString() << " - FUZZY OUTPUT VALUE: " << p_anfis_eng_->getOutputVariable(i)->fuzzyOutputValue() << " - WSum: " << wsum);//XXX
std::cerr << "Zeta*(i,:) : " << ublas::row(Zeta_star, i) << std::endl;//XXX
std::cerr << "Eta*(i,:) : " << ublas::row(Eta_star, i) << std::endl;//XXX
std::cerr << "Theta*(i,:) : " << ublas::row(Theta_star, i) << std::endl;//XXX
std::cerr << "OUTPUT*(i) : " << ublas::inner_prod(ublas::row(Zeta_star, i), xi) + ublas::inner_prod(ublas::row(Eta_star, i), u) + Theta_star(i,0)  << " vs. ANFIS OUTPUT(i) : " << p_anfis_eng_->getOutputVariable(i)->getValue() << std::endl;//XXX
        }

        const std::size_t nxlin = nxi+1;
std::cerr << "# XLIN: " << nxlin << std::endl;//XXX
        ublas::vector<real_type> xlin(nxlin, 0);
        ublas::matrix<real_type> A(nxlin, nxlin, 0);
        ublas::matrix<real_type> B(nxlin, nu, 0);
        ublas::matrix<real_type> C(ny, nxlin, 0);

/*
        // Fills x_{lin} vector and A and B matrices
        std::size_t ha = 0;
        std::size_t da = 0;
        std::size_t hb = 0;
        std::size_t db = 0;
        for (std::size_t i = 0; i < nxlin; ++i)
        {
            if (i < nxi)
            {
                xlin(i) = xi(i);
            }
            else
            {
                xlin(i) = 1;
            }

            if (ha < ny)
            {
                if (i == ha*output_order_)
                {
                    for (std::size_t j = 0; j < nzeta; ++j)
                    {
                        A(i,j) = Zeta_star(i,j);
                    }
                    for (std::size_t j = 0; j < ntheta; ++j)
                    {
                        A(i,nzeta+j) = Theta_star(i,j);
                    }

                    da = ha*output_order_;
                    ++ha;
                }
                else
                {
                    A(i,da++) = 1;
                }
            }
            else if ((input_order_ > 2 || aux_input_order_ > 0) && i >= (ny*output_order_+nu))
            {
                A(i,da++) = 1;
            }

            if (hb < ny)
            {
                if (i == hb*output_order_)
                {
                    for (std::size_t j = 0; j < neta; ++j)
                    {
                        B(i,j) = Eta_star(i,j);
                    }

                    db = hb*output_order_;
                    ++hb;
                }
                else if (input_order_ > 1 && i >= (ny*output_order_) && i < (ny*output_order_+nu))
                {
                    B(i,db++) = 1;
                }
            }
        }
        A(nxlin-1,nxlin-1) = 1;
*/

        std::size_t offs = 0;

        // Fill x_{lin} vector
        ublas::subrange(xlin, 0, nxi) = xi;
        xlin(nxi) = 1;

        // Fill A matrix
        offs = 0;
        for (std::size_t i = 0; i < ny; ++i)
        {
            ublas::subrange(A, i*output_order_, i*output_order_+1, 0, nxi) = ublas::subrange(Zeta_star, i, i+1, 0, nxi);
            A(i*output_order_,nxi) = Theta_star(i,0);
            ublas::subrange(A, i*output_order_+1, i*output_order_+output_order_, i*output_order_, i*output_order_+output_order_-1) = ublas::identity_matrix<double>(output_order_-1, output_order_-1);
        }
        offs += ny*output_order_;
        if (aux_input_order_ > 0)
        {
            for (std::size_t i = 0; i < num_aux_inputs_; ++i)
            {
                ublas::subrange(A, offs+i*aux_input_order_, offs+(i+1)*aux_input_order_, offs+i*aux_input_order_, offs+(i+1)*aux_input_order_) = ublas::identity_matrix<double>(aux_input_order_, aux_input_order_);
            }
            offs += num_aux_inputs_*aux_input_order_;
        }
        if (input_order_ > 2)
        {
            for (std::size_t i = 0; i < nu; ++i)
            {
                ublas::subrange(A, offs+i*(input_order_-1)+1, offs+(i+1)*(input_order_-1), offs+i*(input_order_-2), offs+(i+1)*(input_order_-2)) = ublas::identity_matrix<double>(input_order_-2, input_order_-2);
            }
            offs += nu*(input_order_-1);
        }
        A(nxlin-1,nxlin-1) = 1;

        // Fill B matrix
        offs = 0;
        for (std::size_t i = 0; i < ny; ++i)
        {
            ublas::row(B, i*output_order_) = ublas::row(Eta_star, i);
        }
        offs += ny*output_order_;
        offs += num_aux_inputs_*aux_input_order_;
        if (input_order_ > 1)
        {
            for (std::size_t i = 0; i < nu; ++i)
            {
                B(offs+i*input_order_, i) = 1;
            }
        }

        // Fill C matrix
        std::size_t h = 0;
        for (std::size_t i = 0; i < ny; ++i)
        {
            if (h < ny && i == h*output_order_)
            {
                    C(i,i) = 1;
                    ++h;
            }
        }
std::cerr << "x_{lin} : " << xlin << std::endl;//XXX
std::cerr << "A : " << A << std::endl;//XXX
std::cerr << "B : " << B << std::endl;//XXX
std::cerr << "C : " << C << std::endl;//XXX

        ublas::matrix<real_type> Wy = mpc_tracking_weight_*ublas::identity_matrix<real_type>(ny);
        ublas::matrix<real_type> Wdu = mpc_control_weight_*ublas::identity_matrix<real_type>(nu);
        ublas::vector<real_type> ymin = ublas::scalar_vector<real_type>(ny, -std::numeric_limits<real_type>::infinity());
        ublas::vector<real_type> ymax = ublas::scalar_vector<real_type>(ny, +std::numeric_limits<real_type>::infinity());
        ublas::vector<real_type> dymin = ublas::scalar_vector<real_type>(ny, -std::numeric_limits<real_type>::infinity());
        ublas::vector<real_type> dymax = ublas::scalar_vector<real_type>(ny, +std::numeric_limits<real_type>::infinity());
        ublas::vector<real_type> umin = ublas::scalar_vector<real_type>(nu, 0);
        ublas::vector<real_type> umax = ublas::scalar_vector<real_type>(nu, 1);
        ublas::vector<real_type> dumin = ublas::scalar_vector<real_type>(nu, -std::numeric_limits<real_type>::infinity());
        ublas::vector<real_type> dumax = ublas::scalar_vector<real_type>(nu, +std::numeric_limits<real_type>::infinity());
        //ublas::vector<real_type> yref = ublas::scalar_vector<real_type>(ny, slo_value);
        ublas::vector<real_type> u_opt;

        //FIXME: should we scale the output vector and the reference output vector so that the control input is computed wrt to relative error instead of absolute error?

#ifdef DCS_TESTBED_USE_MATLAB_LINEAR_MPC
        *p_mpc_ctrl_ = dcs::control::matlab_linear_mpc_controller<real_type>(Wy,
#else
        *p_mpc_ctrl_ = dcs::control::linear_mpc_controller<real_type>(Wy,
#endif // DCS_TESTBED_USE_MATLAB_LINEAR_MPC
                                                                    Wdu,
                                                                    ymin,
                                                                    ymax,
                                                                    dymin,
                                                                    dymax,
                                                                    umin,
                                                                    umax,
                                                                    dumin,
                                                                    dumax,
                                                                    prediction_horizon_,
                                                                    control_horizon_);
        p_mpc_ctrl_->solve(A, B, C);
        u_opt = p_mpc_ctrl_->control(xlin, u, yref);
DCS_DEBUG_TRACE("Optimal control from MPC: " << u_opt);///XXX

        return std::vector<real_type>(u_opt.begin(), u_opt.end());
    }


    private: real_type beta_; ///< The EWMA smoothing factor for Cres
    private: std::size_t input_order_; ///< The number of past inputs to consider that can influence the current output
    private: std::size_t aux_input_order_; ///< The number of past auxiliary inputs to consider that can influence the current output
    private: std::size_t output_order_; ///< The number of past outputs to consider that can influence the current output
    private: std::size_t prediction_horizon_; ///< The prediction horizon used by MPC
    private: std::size_t control_horizon_; ///< The prediction horizon used by MPC
    private: real_type forget_factor_; ///< The forgetting factor used by the wRLS algorithm
    private: real_type mpc_tracking_weight_; ///< The weight to apply to the tracking part of the MPC objective function
    private: real_type mpc_control_weight_; ///< The weight to apply to the tracking part of the MPC objective function
    private: std::size_t num_inputs_; ///< The number of system inputs
    private: std::size_t num_aux_inputs_; ///< The number of auxliary system inputs
    private: std::size_t num_outputs_; ///< The number of system outputs
    private: boost::shared_ptr<fl::anfis::Engine> p_anfis_eng_; ///< The fuzzy modeling engine based on ANFIS
    private: boost::shared_ptr<fl::SubtractiveClusteringFisBuilder<fl::anfis::Engine> > p_anfis_builder_; ///< Builder for the ANFIS model
    private: boost::shared_ptr<fl::anfis::Jang1993HybridLearningAlgorithm> p_anfis_trainer_; ///< Training algorithm for the ANFIS model
#ifdef DCS_TESTBED_USE_MATLAB_LINEAR_MPC
    private: boost::shared_ptr<dcs::control::matlab_linear_mpc_controller<real_type> > p_mpc_ctrl_; ///< The MPC controller
#else
    private: boost::shared_ptr<dcs::control::linear_mpc_controller<real_type> > p_mpc_ctrl_; ///< The MPC controller
#endif // DCS_TESTBED_USE_MATLAB_LINEAR_MPC
    private: std::size_t ctrl_count_; ///< Number of times control function has been invoked
    private: std::size_t ctrl_skip_count_; ///< Number of times control has been skipped
    private: std::size_t ctrl_fail_count_; ///< Number of times control has failed
    private: in_sensor_map in_sensors_;
    private: out_sensor_map out_sensors_;
    private: std::string dat_fname_;
    private: ::boost::shared_ptr< std::ofstream > p_dat_ofs_;
    private: std::vector<virtual_machine_performance_category> vm_perf_cats_;
    //private: std::vector<application_performance_category> app_perf_cats_;
    private: std::vector< std::map< virtual_machine_performance_category, std::deque<real_type> > > in_cap_history_;
    private: std::vector< std::map< virtual_machine_performance_category, std::deque<real_type> > > in_util_history_;
    private: std::map< application_performance_category, std::deque<real_type> > out_perf_history_;
    private: bool anfis_initialized_;
    private: fl::DataSet<real_type> anfis_trainset_;
}; // guazzone2015_anfis_ssmpc_application_manager

}} // Namespace dcs::testbed

#endif // DCS_TESTBED_GUAZZONE2015_ANFIS_SSMPC_APPLICATION_MANAGER_HPP

/* vim: set tabstop=4 expandtab shiftwidth=4 softtabstop=4: */
