/**
 * \file dcs/testbed/lq_application_manager.hpp
 *
 * \brief Linear-Quadratic (LQ) system manager.
 *
 * \author Marco Guazzone (marco.guazzone@gmail.com)
 *
 * <hr/>
 *
 * Copyright (C) 2012       Marco Guazzone (marco.guazzone@gmail.com)
 *                          [Distributed Computing System (DCS) Group,
 *                           Computer Science Institute,
 *                           Department of Science and Technological Innovation,
 *                           University of Piemonte Orientale,
 *                           Alessandria (Italy)]
 *
 * This file is part of dcsxx-testbed (below referred to as "this program").
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef DCS_TESTBED_LQ_SYSTEM_MANAGER_HPP
#define DCS_TESTBED_LQ_SYSTEM_MANAGER_HPP


#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics/stats.hpp>
#include <boost/accumulators/statistics/mean.hpp>
#include <boost/numeric/ublas/matrix.hpp>
#include <boost/numeric/ublas/vector.hpp>
#include <boost/smart_ptr.hpp>
#include <cstddef>
#include <dcs/assert.hpp>
#include <dcs/exception.hpp>
#include <dcs/control/design/dlqry.hpp>
#include <dcs/testbed/application_performance_category.hpp>
#include <dcs/testbed/base_application_manager.hpp>
#include <dcs/testbed/system_identification_strategies.hpp>
#include <limits>
#include <stdexcept>


namespace dcs { namespace testbed {

template <typename TraitsT>
class lq_application_manager: public base_application_manager<TraitsT>
{
	private: typedef base_application_manager<TraitsT> base_type;
	public: typedef typename base_type::traits_type traits_type;
	public: typedef typename base_type::uint_type uint_type;
	public: typedef typename traits_type::real_type real_type;
	private: typedef typename base_type::app_type app_type;
	private: typedef typename base_type::app_pointer app_pointer;
	private: typedef typename app_type::sensor_type sensor_type;
	private: typedef typename app_type::sensor_pointer sensor_pointer;
	public: typedef ::boost::numeric::ublas::vector<real_type> numeric_vector_type;
	public: typedef ::boost::numeric::ublas::matrix<real_type> numeric_matrix_type;
	private: typedef base_arx_system_identification_strategy<traits_type> sysid_strategy_type;
	private: typedef ::boost::shared_ptr<sysid_strategy_type> sysid_strategy_pointer;
	private: typedef ::std::vector<real_type> obs_container;


	public: lq_application_manager()
	: ts_(0),
	  tc_(0),
	  nx_(0),
	  nu_(0),
	  ny_(0),
	  x_offset_(0),
	  u_offset_(0),
	  ctl_count_(0),
	  ewma_sf_(0.70),
	  ewma_s_(0)
	{
	}

	public: void sysid_strategy(sysid_strategy_pointer const& p_strategy)
	{
		p_sysid_alg_ = p_strategy;
	}

	public: sysid_strategy_pointer sysid_strategy()
	{
		return p_sysid_alg_;
	}

	public: sysid_strategy_pointer sysid_strategy() const
	{
		return p_sysid_alg_;
	}

	private: void do_sampling_time(uint_type val)
	{
		DCS_ASSERT(val > 0,
				   DCS_EXCEPTION_THROW(::std::invalid_argument,
									   "Invalid sampling time: non-positive value"));

		ts_ = val;
	}

	private: uint_type do_sampling_time() const
	{
		return ts_;
	}

	private: void do_control_time(uint_type val)
	{
		DCS_ASSERT(val > 0,
				   DCS_EXCEPTION_THROW(::std::invalid_argument,
									   "Invalid control time: non-positive value"));

		tc_ = val;
	}

	private: uint_type do_control_time() const
	{
		return tc_;
	}

	private: void do_app(app_pointer const& p_app)
	{
		p_app_ = p_app;
	}

	private: app_pointer do_app()
	{
		return p_app_;
	}

	private: app_pointer do_app() const
	{
		return p_app_;
	}

	private: void do_reset()
	{
		// pre: p_app != null
		DCS_ASSERT(p_app_,
				   DCS_EXCEPTION_THROW(::std::runtime_error,
									   "Application is not set"));
		// pre: p_app != null
		DCS_ASSERT(p_sysid_alg_,
				   DCS_EXCEPTION_THROW(::std::runtime_error,
									   "System identification strategy is not set"));

		p_out_sens_ = p_app_->sensor(response_time_application_performance);
		p_sysid_alg_->init();
#if defined(DCS_TESTBED_EXP_LQ_APP_MGR_USE_ALT_SS) && DCS_TESTBED_LQ_APP_MGR_USE_ALT_SS == 'X'
		nx_ = p_sysid_alg_->num_outputs()*p_sysid_alg_->output_order()+p_sysid_alg_->num_inputs()*(p_sysid_alg_->input_order()-1);
		nu_ = p_sysid_alg_->num_inputs();
		ny_ = 1;
		x_offset_ = (nx_ > 0) ? (nx_-p_sysid_alg_->num_outputs()) : 0;
		u_offset_ = 0;
#elif defined(DCS_TESTBED_EXP_LQ_APP_MGR_USE_ALT_SS) && DCS_TESTBED_LQ_APP_MGR_USE_ALT_SS == 'C'
		nx_ = p_sysid_alg_->num_outputs()*p_sysid_alg_->output_order();
		nu_ = p_sysid_alg_->num_inputs();
		ny_ = 1;
		x_offset_ = (nx_ > 0) ? (nx_-p_sysid_alg_->num_outputs()) : 0;
		u_offset_ = 0;
#else // DCS_TESTBED_EXP_LQ_APP_MGR_ALT_SS
		nx_ = p_sysid_alg_->num_outputs()*p_sysid_alg_->output_order();
		nu_ = p_sysid_alg_->num_inputs()*p_sysid_alg_->input_order();
		ny_ = 1;
		x_offset_ = (nx_ > 0) ? (nx_-p_sysid_alg_->num_outputs()) : 0;
		u_offset_ = (nu_ > 0) ? (nu_-p_sysid_alg_->num_inputs()) : 0;
#endif // DCS_TESTBED_EXP_LQ_APP_MGR_ALT_SS
		x_ = numeric_vector_type(nx_, ::std::numeric_limits<real_type>::quiet_NaN());
		u_ = numeric_vector_type(nu_, ::std::numeric_limits<real_type>::quiet_NaN());
		y_ = numeric_vector_type(ny_, ::std::numeric_limits<real_type>::quiet_NaN());
		ctl_count_ = 0;
	}

	private: void do_sample()
	{
		typedef typename sensor_type::observation_type observation_type;
		typedef ::std::vector<observation_type> observation_container;
		typedef typename observation_container::const_iterator observation_iterator;

		p_out_sens_->sense();
		if (p_out_sens_->has_observations())
		{
			observation_container obs = p_out_sens_->observations();
			observation_iterator end_it = obs.end();
			for (observation_iterator it = obs.begin();
				 it != end_it;
				 ++it)
			{
				out_obs_.push_back(it->value());
			}
		}
	}

	private: void do_control()
	{
		namespace ublas = ::boost::numeric::ublas;

		typedef typename app_type::vm_pointer vm_pointer;
		typedef ::std::vector<vm_pointer> vm_container;
		typedef typename vm_container::const_iterator vm_iterator;

		const ::std::size_t np = p_sysid_alg_->num_outputs();
		const ::std::size_t ns = p_sysid_alg_->num_inputs();
		const ::std::size_t na = p_sysid_alg_->output_order();
		const ::std::size_t nb = p_sysid_alg_->input_order();
		const ::std::size_t nk = p_sysid_alg_->input_delay();

		bool skip_ctl = false;
		numeric_vector_type p(np, 0); // model output
		numeric_vector_type s(ns, 0); // model input

		++ctl_count_;

		// Update measures
		if (out_obs_.size() > 0)
		{
			typedef typename obs_container::const_iterator obs_iterator;

#if defined(DCS_TESTBED_APP_MGR_APPLY_EWMA_TO_EACH_OBSERVATION)
			bool init_check = (ctl_count_ < 1) ? true : false;
			obs_iterator end_it = out_obs_.end();
			for (obs_iterator it = out_obs_.begin();
				 it != end_it;
				 ++it)
			{
				real_type val(*it);

				if (init_check)
				{
					ewma_s_ = val;
					init_check = false;
				}
				else
				{
					ewma_s_ = ewma_sf_*val+(1-ewma_sf_)*ewma_s_;
				}
			}
#else // DCS_TESTBED_APP_MGR_APPLY_EWMA_TO_EACH_OBSERVATION
			::boost::accumulators::accumulator_set< real_type, ::boost::accumulators::stats< ::boost::accumulators::tag::mean > > acc;
			obs_iterator end_it = out_obs_.end();
			for (obs_iterator it = out_obs_.begin();
				 it != end_it;
				 ++it)
			{
				acc(*it);
			}

			real_type aggr_obs = ::boost::accumulators::mean(acc);
			if (ctl_count_ < 1)
			{
				ewma_s_ = aggr_obs;
			}
			else
			{
				ewma_s_ = ewma_sf_*aggr_obs+(1-ewma_sf_)*ewma_s_;
			}
#endif // DCS_TESTBED_APP_MGR_APPLY_EWMA_TO_EACH_OBSERVATION
		}
		else
		{
			// No observation collected during the last control interval
			//TODO: what can we do?
			// - Skip control?
			// - Use the last EWMA value (if ctl_count_ > 1)?
			if (ctl_count_ < 1)
			{
				skip_ctl = true;
			}
		}

		if (!skip_ctl)
		{
			// Rotate old with new inputs/outputs:
			//  x(k) = [p(k-n_a+1) ... p(k)]^T
			//       = [x_{n_p:n_x}(k-1) p(k)]^T
			//  u(k) = [s(k-n_b+1) ... s(k)]^T
			//       = [u_{n_s:n_u}(k-1) s(k)]^T
			// Check if a measure rotation is needed (always but the first time)
DCS_DEBUG_TRACE("Old x=" << x_);
DCS_DEBUG_TRACE("Old u=" << u_);
DCS_DEBUG_TRACE("Old y=" << y_);
			if (ctl_count_ > 1)
			{
				// throw away old observations from the state vector and make room for new ones
				if (nx_ > 0)
				{
#if defined(DCS_TESTBED_EXP_LQ_APP_MGR_USE_ALT_SS)
					if (nb > 1)
					{
						if (nb > 2)
						{
							ublas::subrange(x_, 0, (nb-2)*ns) = ublas::subrange(x_, ns, (nb-1)*ns);
						}
						ublas::subrange(x_, (nb-2)*ns, (nb-1)*ns) = u_;
					}
					ublas::subrange(x_, ns*(nb-1), nx_-np) = ublas::subrange(x_, (nb-1)*ns+np, nx_);
					ublas::subrange(x_, nx_-np, nx_) = ublas::scalar_vector<real_type>(np, ::std::numeric_limits<real_type>::quiet_NaN());
#else // DCS_TESTBED_EXP_LQ_APP_MGR_USE_ALT_SS
					ublas::subrange(x_, 0, nx_-np) = ublas::subrange(x_, np, nx_);
					ublas::subrange(x_, nx_-np, nx_) = ublas::scalar_vector<real_type>(np, ::std::numeric_limits<real_type>::quiet_NaN());
#endif // DCS_TESTBED_EXP_LQ_APP_MGR_USE_ALT_SS
				}
				// throw away old observations from the input vector and make room for new ones
				if (nu_ > 0)
				{
#if defined(DCS_TESTBED_EXP_LQ_APP_MGR_USE_ALT_SS)
					u_ = ublas::scalar_vector<real_type>(ns, ::std::numeric_limits<real_type>::quiet_NaN());
#else // DCS_TESTBED_EXP_LQ_APP_MGR_USE_ALT_SS
					ublas::subrange(u_, 0, nu_-ns) = ublas::subrange(u_, ns, nu_);
					ublas::subrange(u_, nu_-ns, nu_) = ublas::scalar_vector<real_type>(ns, ::std::numeric_limits<real_type>::quiet_NaN());
#endif // DCS_TESTBED_EXP_LQ_APP_MGR_USE_ALT_SS
				}
			}
DCS_DEBUG_TRACE("Prep x=" << x_);
DCS_DEBUG_TRACE("Prep u=" << u_);
DCS_DEBUG_TRACE("Prep y=" << y_);

			if (nu_ > 0)
			{
				::std::size_t i(0);
				vm_container vms = this->app()->vms();

				// check: size consistency
				DCS_DEBUG_ASSERT( ns >= vms.size() );

				vm_iterator vm_end_it = vms.end();
				for (vm_iterator vm_it = vms.begin();
					 vm_it != vm_end_it;
					 ++vm_it)
				{
					vm_pointer p_vm(*vm_it);

					// check: p_vm != null
					DCS_DEBUG_ASSERT( p_vm );

					s(i) = p_vm->cpu_share()*100.0*p_vm->max_num_vcpus();
				}
			}
			if (ny_ > 0)
			{
				y_ = ublas::element_div(yc_, yr_)-ublas::scalar_vector<real_type>(ny_, 1);
			}
DCS_DEBUG_TRACE("New x=" << x_);
DCS_DEBUG_TRACE("New u=" << u_);
DCS_DEBUG_TRACE("New y=" << y_);

			do_lq_control();
		}

		// Reset measures
		out_obs_.clear();
	}

	private: virtual void do_lq_control() = 0;


	private: uint_type ts_; ///< Sampling time (in ms)
	private: uint_type tc_; ///< Control time (in ms)
	private: app_pointer p_app_; ///< Pointer to the managed application
	private: sensor_pointer p_out_sens_; ///< Sensor for the application output
	private: sysid_strategy_pointer p_sysid_alg_;
	private: obs_container out_obs_; ///< Application output observations collected in the last control interval
	private: ::std::size_t nx_; ///< Number of states
	private: ::std::size_t nu_; ///< Number of inputs
	private: ::std::size_t ny_; ///< Number of outputs
	private: ::std::size_t x_offset_; ///< Offset used to rotate the state vector to make space for new observations
	private: ::std::size_t u_offset_; ///< Offset used to rotate the input vector to make space for new observations
	private: numeric_vector_type x_; ///< The state vector for the state-space representation
	private: numeric_vector_type u_; ///< The input vector for the state-space representation
	private: numeric_vector_type y_; ///< The output vector for the state-space representation
	private: numeric_vector_type yr_; ///< The output vector to be tracked 
	private: numeric_vector_type yc_; ///< The output vector 
	private: ::std::size_t ctl_count_; ///< Number of times control has been performed
	private: real_type ewma_sf_; ///< EWMA smoothing factor
	private: real_type ewma_s_; ///< Current EWMA value
//	private: mutex_type smp_mutex_;
};

template <typename TraitsT>
class lqry_application_manager: public lq_application_manager<TraitsT>
{
	private: typedef lq_application_manager<TraitsT> base_type;
	public: typedef typename base_type::traits_type traits_type;
	private: typedef typename traits_type::real_type real_type;
	private: typedef ::dcs::control::dlqry_controller<real_type> controller_type;


	public: lqry_application_manager()
	{
	}

	private: void do_lq_control()
	{
		typedef typename controller_type::vector_type vector_type;

/*TODO
		ctrl_.solve();
		vector_type u = ctrl_.control(x);
		for (::std::size i = 0; i < u.size(); ++i)
		{
			this->app()->vm(i).cpu_share(u[i]);
		}
*/
	}


	private: controller_type ctrl_;
};

}} // Namespace dcs::testbed

#endif // DCS_TESTBED_LQ_SYSTEM_MANAGER_HPP
