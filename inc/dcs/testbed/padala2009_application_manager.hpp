/**
 * \file dcs/testbed/padala2009_application_manager.hpp
 *
 * \brief Class modeling the Application Manager component based on the work
 *  of (Padala et al, 2009)
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

#ifndef DCS_TESTBED_PADALA2009_APPLICATION_MANAGER_HPP
#define DCS_TESTBED_PADALA2009_APPLICATION_MANAGER_HPP

//@{ Consistency checks for macros

#ifndef DCS_TESTBED_EXP_PADALA2009_APP_MGR_NEGATIVE_SHARE_ACTION
# define DCS_TESTBED_EXP_PADALA2009_APP_MGR_NEGATIVE_SHARE_ACTION 'C'
#endif // DCS_TESTBED_EXP_PADALA2009_APP_MGR_NEGATIVE_SHARE_ACTION
#if    DCS_TESTBED_EXP_PADALA2009_APP_MGR_NEGATIVE_SHARE_ACTION != 'C' \
    && DCS_TESTBED_EXP_PADALA2009_APP_MGR_NEGATIVE_SHARE_ACTION != 'M' \
    && DCS_TESTBED_EXP_PADALA2009_APP_MGR_NEGATIVE_SHARE_ACTION != 'R' \
    && DCS_TESTBED_EXP_PADALA2009_APP_MGR_NEGATIVE_SHARE_ACTION != 'W'
# error Unknwon value for DCS_TESTBED_EXP_PADALA2009_APP_MGR_NEGATIVE_SHARE_ACTION.
#endif // DCS_TESTBED_EXP_PADALA2009_APP_MGR_NEGATIVE_SHARE_ACTION

//@} Consistency checks for macros


#include <algorithm>
#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics/stats.hpp>
#include <boost/accumulators/statistics/mean.hpp>
#include <boost/numeric/ublas/matrix.hpp>
#include <boost/numeric/ublas/matrix_expression.hpp>
#include <boost/numeric/ublas/matrix_proxy.hpp>
#include <boost/numeric/ublas/vector.hpp>
#include <boost/numeric/ublas/vector_expression.hpp>
#include <boost/numeric/ublas/vector_proxy.hpp>
#include <boost/numeric/ublasx/operation/all.hpp>
#ifdef DCS_TESTBED_EXP_PADALA2009_APP_MGR_USE_ARX_B0_SIGN_HEURISTIC
# include <boost/numeric/ublasx/operation/any.hpp>
#endif // DCS_TESTBED_EXP_PADALA2009_APP_MGR_USE_ARX_B0_SIGN_HEURISTIC
#include <boost/numeric/ublasx/operation/isfinite.hpp>
#include <boost/smart_ptr.hpp>
#include <cstddef>
#include <dcs/assert.hpp>
//#include <dcs/control/analysis/controllability.hpp>
//#include <dcs/control/analysis/detectability.hpp>
//#include <dcs/control/analysis/observability.hpp>
//#include <dcs/control/analysis/stabilizability.hpp>
#include <dcs/debug.hpp>
#include <dcs/exception.hpp>
#include <dcs/logging.hpp>
#include <dcs/macro.hpp>
#include <dcs/math/traits/float.hpp>
#include <dcs/testbed/application_performance_category.hpp>
#include <dcs/testbed/base_application_manager.hpp>
#include <dcs/testbed/system_identification_strategies.hpp>
#ifdef DCS_TESTBED_EXP_PADALA2009_APP_MGR_USE_ARX_B0_SIGN_HEURISTIC
# include <functional>
#endif // DCS_TESTBED_EXP_PADALA2009_APP_MGR_USE_ARX_B0_SIGN_HEURISTIC
#include <limits>
#include <map>
#include <stdexcept>


//TODO:
// - Currently the code in this class assumes the single resource (CPU) case.
//


namespace dcs { namespace testbed {

template <typename TraitsT>
class padala2009_application_manager: public base_application_manager<TraitsT>
{
	private: typedef base_application_manager<TraitsT> base_type;
	public: typedef typename base_type::traits_type traits_type;
	public: typedef typename base_type::uint_type uint_type;
	public: typedef typename traits_type::real_type real_type;
	private: typedef typename base_type::app_type app_type;
	private: typedef typename base_type::app_pointer app_pointer;
	private: typedef typename app_type::sensor_type sensor_type;
	private: typedef typename app_type::sensor_pointer sensor_pointer;
	protected: typedef ::boost::numeric::ublas::vector<real_type> numeric_vector_type;
	protected: typedef ::boost::numeric::ublas::matrix<real_type> numeric_matrix_type;
	private: typedef base_arx_system_identification_strategy<traits_type> sysid_strategy_type;
	private: typedef ::boost::shared_ptr<sysid_strategy_type> sysid_strategy_pointer;
	private: typedef ::std::vector<real_type> observation_container;
	private: typedef ::std::map<application_performance_category,observation_container> observation_map;
	private: typedef ::std::map<application_performance_category,real_type> target_map;
	private: typedef ::std::map<application_performance_category,sensor_pointer> sensor_map;


	private: static const uint_type default_sampling_time;
	private: static const uint_type default_control_time;
	private: static const real_type default_min_share;
	private: static const real_type default_max_share;
	private: static const real_type default_stability_factor;
	private: static const real_type default_ewma_smoothing_factor;


	public: padala2009_application_manager()
	: ts_(default_sampling_time),
	  tc_(default_control_time),
      ctl_count_(0),
      ctl_skip_count_(0),
      ctl_fail_count_(0),
      sysid_fail_count_(0),
	  ewma_sf_(default_ewma_smoothing_factor),
	  q_(default_stability_factor)
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

	public: void target_value(application_performance_category cat, real_type val)
	{
		tgt_map_[cat] = val;
	}

	public: void stability_factor(real_type value)
	{
		q_ = value;
	}

	public: real_type stability_factor() const
	{
		return q_;
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
		// pre: p_sysid_alg_ != null
		DCS_ASSERT(p_sysid_alg_,
				   DCS_EXCEPTION_THROW(::std::runtime_error,
									   "System identification strategy is not set"));
		//[FIXME]
		DCS_ASSERT(tgt_map_.size() == 1,
				   DCS_EXCEPTION_THROW(::std::logic_error,
									   "Currently, only one application performace category is handled"));
		DCS_ASSERT(tgt_map_.size() == 1,
				   DCS_EXCEPTION_THROW(::std::logic_error,
									   "Currently, only one application performace category is handled"));
		//[/FIXME]

		p_sysid_alg_->init();

		const ::std::size_t ns = p_sysid_alg_->num_inputs();
		const ::std::size_t np = p_sysid_alg_->num_outputs();

		yr_ = numeric_vector_type(np, ::std::numeric_limits<real_type>::quiet_NaN());
		typedef typename target_map::const_iterator target_iterator;
		target_iterator tgt_end_it = tgt_map_.end();
		for (target_iterator tgt_it = tgt_map_.begin();
			 tgt_it != tgt_end_it;
			 ++tgt_it)
		{
			application_performance_category cat(tgt_it->first);

			yr_ = numeric_vector_type(np, tgt_it->second);
			out_sens_map_[cat] = p_app_->sensor(cat);
		}
		ewma_s_ = numeric_vector_type(ns, ::std::numeric_limits<real_type>::quiet_NaN());
		ewma_p_ = numeric_vector_type(np, ::std::numeric_limits<real_type>::quiet_NaN());
		ctl_count_ = ctl_skip_count_
				   = ctl_fail_count_
				   = sysid_fail_count_
				   = 0;
	}

	private: void do_sample()
	{
		typedef typename sensor_type::observation_type obs_type;
		typedef ::std::vector<obs_type> obs_container;
		typedef typename obs_container::const_iterator obs_iterator;
		typedef typename sensor_map::const_iterator sensor_iterator;

		DCS_DEBUG_TRACE("(" << this << ") BEGIN Do SAMPLE - Count: " << ctl_count_ << "/" << ctl_skip_count_ << "/" << sysid_fail_count_ << "/" << ctl_fail_count_);

		sensor_iterator sens_end_it = out_sens_map_.end();
		for (sensor_iterator sens_it = out_sens_map_.begin();
			 sens_it != sens_end_it;
			 ++sens_it)
		{
			application_performance_category cat(sens_it->first);
			sensor_pointer p_sens(sens_it->second);

			p_sens->sense();
			if (p_sens->has_observations())
			{
				obs_container obs = p_sens->observations();
				obs_iterator end_it = obs.end();
				for (obs_iterator it = obs.begin();
					 it != end_it;
					 ++it)
				{
					out_obs_map_[cat].push_back(it->value());
				}
			}
		}

		DCS_DEBUG_TRACE("(" << this << ") END Do SAMPLE - Count: " << ctl_count_ << "/" << ctl_skip_count_ << "/" << sysid_fail_count_ << "/" << ctl_fail_count_);
	}

    private: void do_control()
    {
		namespace ublas = ::boost::numeric::ublas;
		namespace ublasx = ::boost::numeric::ublasx;

		typedef typename app_type::vm_pointer vm_pointer;
		typedef ::std::vector<vm_pointer> vm_container;
		typedef typename vm_container::iterator vm_iterator;
		typedef typename vm_container::const_iterator vm_citerator;

		DCS_DEBUG_TRACE("(" << this << ") BEGIN Do CONTROL - Count: " << ctl_count_ << "/" << ctl_skip_count_ << "/" << sysid_fail_count_ << "/" << ctl_fail_count_);

		const ::std::size_t np = p_sysid_alg_->num_outputs();
		const ::std::size_t ns = p_sysid_alg_->num_inputs();
		const ::std::size_t na = p_sysid_alg_->output_order();
		const ::std::size_t nb = p_sysid_alg_->input_order();
		const ::std::size_t nk = p_sysid_alg_->input_delay();

		bool skip_ctl = false;
		numeric_vector_type p(np, 0); // model output (performance measure)
		numeric_vector_type s(ns, 0); // model input (resource share)

		vm_container vms = this->app()->vms();

		++ctl_count_;

		// Update measures
		if (out_obs_map_.size() > 0)
		{
			typedef typename observation_map::const_iterator obs_map_iterator;
			typedef typename observation_container::const_iterator obs_iterator;

#if defined(DCS_TESTBED_APP_MGR_APPLY_EWMA_TO_EACH_OBSERVATION)
			//bool init_check = (ctl_count_ < 1) ? true : false;
			obs_map_iterator map_end_it = out_obs_map_.end();
			for (obs_map_iterator map_it = out_obs_map_.begin();
				 map_it != map_end_it;
				 ++map_it)
			{
				application_performance_category cat(map_it->first);

				obs_iterator end_it = map_it->second.end();
				for (obs_iterator it = map_it->second.begin();
					 it != end_it;
					 ++it)
				{
					real_type val(*it);

					if (::std::isnan(ewma_p_(0)))
					{
						ewma_p_(0) = val;
					}
					else
					{
						ewma_p_(0) = ewma_sf_*val+(1-ewma_sf_)*ewma_p_(0);
					}
				}
			}
#else // DCS_TESTBED_APP_MGR_APPLY_EWMA_TO_EACH_OBSERVATION
			obs_map_iterator map_end_it = out_obs_map_.end();
			for (obs_map_iterator map_it = out_obs_map_.begin();
				 map_it != map_end_it;
				 ++map_it)
			{
				application_performance_category cat(map_it->first);

				::boost::accumulators::accumulator_set< real_type, ::boost::accumulators::stats< ::boost::accumulators::tag::mean > > acc;
				obs_iterator end_it = map_it->second.end();
				for (obs_iterator it = map_it->second.begin();
					 it != end_it;
					 ++it)
				{
					acc(*it);
				}

				real_type aggr_obs = ::boost::accumulators::mean(acc);
				//if (ctl_count_ < 1)
				if (::std::isnan(ewma_p_(0)))
                {
                    ewma_p_(0) = aggr_obs;
                }
                else
                {
                    ewma_p_(0) = ewma_sf_*aggr_obs+(1-ewma_sf_)*ewma_p_(0);
                }
            }
#endif // DCS_TESTBED_APP_MGR_APPLY_EWMA_TO_EACH_OBSERVATION
DCS_DEBUG_TRACE("Observed Smoothed Output: " << ewma_p_);//XXX
		}
		else if (np > 0)
		{
			// No observation collected during the last control interval
			//TODO: what can we do?
			// - Skip control?
			// - Use the last EWMA value (if ctl_count_ > 1)?
			skip_ctl = true;
		}
		if (ns > 0)
		{
			::std::size_t v(0);

			vm_citerator vm_end_it = vms.end();
			for (vm_citerator vm_it = vms.begin();
				 vm_it != vm_end_it;
				 ++vm_it)
			{
				vm_pointer p_vm(*vm_it);

				// check: p_vm != null
				DCS_DEBUG_ASSERT( p_vm );

				real_type val(p_vm->cpu_share());

				if (::std::isnan(ewma_s_(v)))
				{
					ewma_s_(v) = val;
				}
				else
				{
					ewma_s_(v) = ewma_sf_*val+(1-ewma_sf_)*ewma_s_(v);
				}
			}
DCS_DEBUG_TRACE("Observed Smoothed Input: " << ewma_s_);//XXX
		}

		if (!skip_ctl)
		{
			// Update inputs/outputs
			if (np > 0)
			{
				//FIXME: fix the assignment below
				//       Should we normalize/deviate/...?
				p = ublas::element_div(ewma_p_, yr_);
			}
			if (ns > 0)
			{
				//FIXME: actual share should be scaled according to the capacity of the "reference" machine

				::std::size_t v(0);

				vm_citerator vm_end_it = vms.end();
				for (vm_citerator vm_it = vms.begin();
					 vm_it != vm_end_it;
					 ++vm_it)
				{
					vm_pointer p_vm(*vm_it);

					// check: p_vm != null
					DCS_DEBUG_ASSERT( p_vm );

					s(v) = p_vm->cpu_share()/**p_vm->max_num_vcpus()*/;

					++v;
				}
			}

			// Estimate system params
			bool ok(true);
			numeric_vector_type ph;
			try
			{
				ph = p_sysid_alg_->estimate(p, s);
DCS_DEBUG_TRACE("RLS estimation:");//XXX
DCS_DEBUG_TRACE("p=" << p);//XXX
DCS_DEBUG_TRACE("s=" << s);//XXX
DCS_DEBUG_TRACE("p_hat=" << ph);//XXX
DCS_DEBUG_TRACE("Theta_hat=" << p_sysid_alg_->Theta_hat());//XXX
DCS_DEBUG_TRACE("P=" << p_sysid_alg_->P());//XXX
DCS_DEBUG_TRACE("phi=" << p_sysid_alg_->phi());//XXX

				if (!ublasx::all(ublasx::isfinite(p_sysid_alg_->Theta_hat())))
				{
					::std::ostringstream oss;
					oss << "Unable to estimate system parameters: infinite values in system parameters";
					::dcs::log_warn(DCS_LOGGING_AT, oss.str());

					ok = false;
				}
			}
			catch (::std::exception const& e)
			{
				DCS_DEBUG_TRACE( "Caught exception: " << e.what() );

				::std::ostringstream oss;
				oss << "Unable to estimate system parameters: " << e.what();
				::dcs::log_warn(DCS_LOGGING_AT, oss.str());

				ok = false;
			}

			if (ok && p_sysid_alg_->count() > (na+nb+nk-1))
			{
				numeric_vector_type opt_s;
				try
				{
					// $\bar{\mathbf{u}}_a^*(k)= (\mathbf{b}_0\mathbf{b}_0^T+q\mathbf{I})^{-1}((1+\sum_{i=1}^{n_a}\hat{y}_a(k-i)-\sum_{i=1}^{n_b}\mathbf{b}_i^T\mathbf{u}_a(k-i))\mathbf{b}_0+q\sum_{i=1}^{n_b}\mathbf{u}_a(k-i))$
					// (b1^2 + q)^{-1}*(q*(u2 + u3) + (r - b2*u2 - b3*u3 + a1*y1 + a2*y2 + a3*y3)*b1)

#ifdef DCS_TESTBED_EXP_PADALA2009_APP_MGR_USE_ARX_B0_SIGN_HEURISTIC
					// Check on B(1) suggested by Karlsson et al "Dynamic Black-Box Performance Model Estimation for Self-Tuning Regulators", 2005
					// This essentially consider the model as a linear model where u(k) is the free variabile.
					// They compute the first partial derivative wrt to u(k) which gives the matrix B(1).
					// In order to preverse reverse proportionality ==> diag(B(1)) < 0
					if (ublasx::any(p_sysid_alg_->B(1), ::std::bind2nd(::std::greater_equal<real_type>(), 0)))
					{
						DCS_EXCEPTION_THROW(::std::runtime_error,
											"Cannot compute optimal control input: First partial derivative of input-output model has positive elements on the main diagonal");
					}

//FIXME: to be checked
//					// Try to predict a new y value and see if reverse proportionality hold
//					{
//						vector_type pred_out1(n_s_,0);
//						vector_type pred_out2(n_s_,0);
//
//						for (uint_type k = 1; k <= n_a_; ++k)
//						{
//							pred_out1 = pred_out1 - ublas::prod(ptr_ident_strategy_->A(k), ptr_ident_strategy_->y(k));
//						}
//						pred_out2 = pred_out1;
//						for (uint_type k = 1; k <= n_b_; ++k)
//						{
//							pred_out1 = pred_out1 + ublas::prod(ptr_ident_strategy_->B(k), ptr_ident_strategy_->u(k+d_-1));
//							if (k > 1)
//							{
//								pred_out2 = pred_out2 + ublas::prod(ptr_ident_strategy_->B(k), ptr_ident_strategy_->u(k+d_-1));
//							}
//							else
//							{
//								pred_out2 = pred_out2 + ublas::prod(ptr_ident_strategy_->B(1), ublas::scalar_vector<real_type>(n_s_, 1));
//							}
//						}
//
////::std::cerr << "PREDICTION -- y1 = " << pred_out1 << " - y2 = " << pred_out2 << ::std::endl;//XXX
//						if (ublasx::any(pred_out2-pred_out1, ::std::bind2nd(::std::greater<real_type>(), 0)))
//						{
//							//DCS_EXCEPTION_THROW( ::std::runtime_error, "Cannot compute optimal control input: Bad prediction from Input-Output model" );
//							::std::ostringstream oss;
//							oss << "APP: " << app.id() << " - Bad prediction from Input-Output model";
//							::dcs::testbed::log_warn(cls_id_, oss.str());
//						}
//					}
#endif // DCS_TESTBED_EXP_PADALA2009_APP_MGR_USE_ARX_B0_SIGN_HEURISTIC

					real_type r(1); //FIXME: only one reference target is handled
::std::cerr << "HERE.1 - r=" << r << ::std::endl;//XXX

					numeric_vector_type b0(ublas::row(p_sysid_alg_->B(1), 0));
::std::cerr << "HERE.2 - B(0)=" << b0 << ::std::endl;//XXX

					numeric_matrix_type L;
					L = ublas::outer_prod(b0, b0)+q_*ublas::identity_matrix<real_type>(ns, ns);
::std::cerr << "HERE.3 - L=" << L << ::std::endl;//XXX
					bool inv = ublasx::inv_inplace(L);
::std::cerr << "HERE.4 - inv=" << inv << ::std::endl;//XXX
					if (!inv)
					{
						DCS_EXCEPTION_THROW(::std::runtime_error, "Cannot compute optimal control input: Matrix (b_0*b_0^T+q*I_{ns,ns}) is not invertible");
					}

::std::cerr << "HERE.5 - L^{-1}=" << L << ::std::endl;//XXX
					real_type say(0);
					real_type sbu(0);
					numeric_vector_type su(ns, 0);
					for (::std::size_t k = 1; k <= na; ++k)
					{
::std::cerr << "HERE.6 - A(" << k << ")=" << p_sysid_alg_->A(k) << " - y(" << k << ")=" << p_sysid_alg_->y(k) << ::std::endl;//XXX
						say += ublas::prod(p_sysid_alg_->A(k), p_sysid_alg_->y(k))(0);
					}
::std::cerr << "HERE.7 - say=" << say << ::std::endl;//XXX
					for (::std::size_t k = 2; k <= nb; ++k)
					{
::std::cerr << "HERE.8 - B(" << (k-1) << ")=" << p_sysid_alg_->B(k) << " - u(" << (k+nk-1-1) << ")=" << p_sysid_alg_->u(k+nk-2) << ::std::endl;//XXX
						sbu += ublas::prod(p_sysid_alg_->B(k), p_sysid_alg_->u(k+nk-2))(0);
						su = su+p_sysid_alg_->u(k+nk-2);
					}
::std::cerr << "HERE.9 - sbu=" << sbu << " - su=" << su << ::std::endl;//XXX
					numeric_vector_type R;
					R = (r+say-sbu)*b0+q_*su;

::std::cerr << "HERE.10 - OPT_S=" << ublas::prod(L, (r+say-sbu)*b0+q_*su) << ::std::endl;//XXX
//::std::cerr << "HERE.10 - OPT_S-alt=" << ublas::prod(L, (r-say-sbu)*b0+q_*su) << ::std::endl;//XXX
::std::cerr << "HERE.10 - R=" << R << ::std::endl;//XXX
					opt_s = ublas::prod(L, R);
::std::cerr << "HERE.13 - opt_s=" << opt_s << ::std::endl;//XXX
				}
				catch (::std::exception const& e)
				{
					DCS_DEBUG_TRACE( "Caught exception: " << e.what() );

					::std::ostringstream oss;
					oss << "Unable to compute optimal control: " << e.what();
					::dcs::log_warn(DCS_LOGGING_AT, oss.str());

					ok = false;
				}
				if (ok)
				{
DCS_DEBUG_TRACE("Applying optimal control");
					//FIXME: new share should be scaled according to the capacity of the "real" machine
					//FIXME: implement the Physical Machine Manager

					::std::size_t v(0);
					vm_iterator vm_end_it = vms.end();
					for (vm_iterator vm_it = vms.begin();
						 vm_it != vm_end_it;
						 ++vm_it)
					{
						vm_pointer p_vm(*vm_it);

						// check: p_vm != null
						DCS_DEBUG_ASSERT( p_vm );

						real_type new_share = opt_s(v);

						if (new_share >= 0)
						{
							if (::dcs::math::float_traits<real_type>::definitely_less(new_share, default_min_share))
							{
								::std::ostringstream oss;
								oss << "Optimal share (" << new_share << ") too small; adjusted to " << default_min_share;
								::dcs::log_warn(DCS_LOGGING_AT, oss.str());
							}
							if (::dcs::math::float_traits<real_type>::definitely_greater(new_share, default_max_share))
							{
								::std::ostringstream oss;
								oss << "Optimal share (" << new_share << ") too big; adjusted to " << default_max_share;
								::dcs::log_warn(DCS_LOGGING_AT, oss.str());
							}

							new_share = ::std::min(::std::max(new_share, default_min_share), default_max_share);
						}
						else
						{
							++ctl_fail_count_;

							::std::ostringstream oss;
							oss << "Control not applied: computed negative share (" << new_share << ") for VM '" << p_vm->id() << "'";
							::dcs::log_warn(DCS_LOGGING_AT, oss.str());

							ok = false;
						}

DCS_DEBUG_TRACE("VM '" << p_vm->id() << "' - old-share: " << p_vm->cpu_share() << " - new-share: " << new_share);
						p_vm->cpu_share(new_share);

						++v;
					}
DCS_DEBUG_TRACE("Optimal control applied");//XXX
				}
				else
				{
					++ctl_fail_count_;

					::std::ostringstream oss;
					oss << "Control not applied: failed to solve the control problem";
					::dcs::log_warn(DCS_LOGGING_AT, oss.str());
				}
			}
			else if (!ok)
			{
				p_sysid_alg_->reset();
				++sysid_fail_count_;

				::std::ostringstream oss;
				oss << "Control not applied: failed to solve the identification problem";
				::dcs::log_warn(DCS_LOGGING_AT, oss.str());
			}
		}
		else
		{
			++ctl_skip_count_;
		}

		// Reset measures
		out_obs_map_.clear();

		DCS_DEBUG_TRACE("(" << this << ") END Do CONTROL - Count: " << ctl_count_ << "/" << ctl_skip_count_ << "/" << sysid_fail_count_ << "/" << ctl_fail_count_);
	}


	private: uint_type ts_; ///< Sampling time (in ms)
	private: uint_type tc_; ///< Control time (in ms)
	private: app_pointer p_app_; ///< Pointer to the managed application
	private: sensor_map out_sens_map_; ///< Sensor map for the application outputs
	private: sysid_strategy_pointer p_sysid_alg_;
	private: observation_map out_obs_map_; ///< Application output observations collected in the last control interval
	private: numeric_vector_type yr_; ///< The output vector to be tracked 
	private: ::std::size_t ctl_count_; ///< Number of times control function has been invoked
	private: ::std::size_t ctl_skip_count_; ///< Number of times control has been skipped
	private: ::std::size_t ctl_fail_count_; ///< Number of times control has failed
	private: ::std::size_t sysid_fail_count_; ///< Number of times system identification has failed
	private: real_type ewma_sf_; ///< EWMA smoothing factor
	private: real_type q_; ///< The stability factor
	private: numeric_vector_type ewma_s_; ///< Current EWMA values for inputs
	private: numeric_vector_type ewma_p_; ///< Current EWMA values for outputs
	private: target_map tgt_map_; ///< Mapping between application performance categories and target values
}; // padala2009_application_manager

template <typename T>
const typename padala2009_application_manager<T>::uint_type padala2009_application_manager<T>::default_sampling_time = 1;

template <typename T>
const typename padala2009_application_manager<T>::uint_type padala2009_application_manager<T>::default_control_time = 5;

template <typename T>
const typename padala2009_application_manager<T>::real_type padala2009_application_manager<T>::default_min_share = 0.20;

template <typename T>
const typename padala2009_application_manager<T>::real_type padala2009_application_manager<T>::default_max_share = 1.00;

template <typename T>
const typename padala2009_application_manager<T>::real_type padala2009_application_manager<T>::default_ewma_smoothing_factor = 0.70;

template <typename T>
const typename padala2009_application_manager<T>::real_type padala2009_application_manager<T>::default_stability_factor = 2.0;

}} // Namespace dcs::testbed

#endif // DCS_TESTBED_PADALA2009_APPLICATION_MANAGER_HPP
