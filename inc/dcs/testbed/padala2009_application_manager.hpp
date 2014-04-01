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
#include <boost/accumulators/statistics/p_square_quantile.hpp>
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
#include <ctime>
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
#include <fstream>
#ifdef DCS_TESTBED_EXP_PADALA2009_APP_MGR_USE_ARX_B0_SIGN_HEURISTIC
# include <functional>
#endif // DCS_TESTBED_EXP_PADALA2009_APP_MGR_USE_ARX_B0_SIGN_HEURISTIC
#include <limits>
#include <map>
#include <sstream>
#include <stdexcept>
#include <vector>


//TODO:
// - Currently the code in this class assumes the single resource (CPU) case.
//


namespace dcs { namespace testbed {

template <typename TraitsT>
class padala2009_application_manager: public base_application_manager<TraitsT>
{
	private: typedef base_application_manager<TraitsT> base_type;
	public: typedef typename base_type::traits_type traits_type;
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
	//private: typedef ::std::map<application_performance_category,numeric_vector_type> observation_map;
	private: typedef ::std::map<application_performance_category,sensor_pointer> sensor_map;


	private: static const real_type default_sampling_time;
	private: static const real_type default_control_time;
	private: static const real_type default_min_share;
	private: static const real_type default_max_share;
	private: static const real_type default_stability_factor;


	public: padala2009_application_manager()
	: q_(default_stability_factor),
	  ctl_count_(0),
      ctl_skip_count_(0),
      ctl_fail_count_(0),
      sysid_fail_count_(0)
	{
		this->sampling_time(default_sampling_time);
		this->control_time(default_control_time);
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

	public: void stability_factor(real_type value)
	{
		q_ = value;
	}

	public: real_type stability_factor() const
	{
		return q_;
	}

	public: void export_data_to(::std::string const& fname)
	{
		dat_fname_ = fname;
	}

	private: void do_reset()
	{
		// pre: p_sysid_alg_ != null
		DCS_ASSERT(p_sysid_alg_,
				   DCS_EXCEPTION_THROW(::std::runtime_error,
									   "System identification strategy is not set"));
		//[FIXME]
		// Currently, we only handle a single performance metric
		// To handle more than one performance metrics we can take the approach used for instance in:
		// - X. Liu, X. Zhu, P. Padala, Z. Wang, S. Singhal,
		//   "Optimal Multivariate Control for Differentiated Services on a Shared Hosting Platform",
		//   HP Labs Technical Report
		DCS_ASSERT(this->target_values().size() == 1,
				   DCS_EXCEPTION_THROW(::std::logic_error,
									   "Currently, only one application performace category is handled"));
		//[/FIXME]

		p_sysid_alg_->init();

		//const ::std::size_t nu = p_sysid_alg_->num_inputs();
		//const ::std::size_t ny = p_sysid_alg_->num_outputs();

		out_sensors_.clear();
		//ybar_.clear();
		typedef typename base_type::target_value_map::const_iterator target_iterator;
		target_iterator tgt_end_it = this->target_values().end();
		for (target_iterator tgt_it = this->target_values().begin();
			 tgt_it != tgt_end_it;
			 ++tgt_it)
		{
			const application_performance_category cat(tgt_it->first);

			//ybar_[cat] = numeric_vector_type(ny, tgt_it->second);
			out_sensors_[cat] = this->app().sensor(cat);
		}

		// Reset counters
		ctl_count_ = ctl_skip_count_
				   = ctl_fail_count_
				   = sysid_fail_count_
				   = 0;

		// Reset output data file
		if (p_dat_ofs_ && p_dat_ofs_->is_open())
		{
			p_dat_ofs_->close();
		}
		p_dat_ofs_.reset();
		if (!dat_fname_.empty())
		{
			p_dat_ofs_ = ::boost::make_shared< ::std::ofstream >(dat_fname_.c_str());
			if (!p_dat_ofs_->good())
			{
				::std::ostringstream oss;
				oss << "Cannot open output data file '" << dat_fname_ << "'";

				DCS_EXCEPTION_THROW(::std::runtime_error, oss.str());
			}

			//TODO: write header
		}
	}

	private: void do_sample()
	{
		typedef typename sensor_map::const_iterator sensor_iterator;

		DCS_DEBUG_TRACE("(" << this << ") BEGIN Do SAMPLE - Count: " << ctl_count_ << "/" << ctl_skip_count_ << "/" << sysid_fail_count_ << "/" << ctl_fail_count_);

		sensor_iterator sens_end_it = out_sensors_.end();
		for (sensor_iterator sens_it = out_sensors_.begin();
			 sens_it != sens_end_it;
			 ++sens_it)
		{
			const application_performance_category cat(sens_it->first);

			sensor_pointer p_sens(sens_it->second);

			// check: p_sens != null
			DCS_DEBUG_ASSERT( p_sens );

			p_sens->sense();
			if (p_sens->has_observations())
			{
				typedef ::std::vector<typename sensor_type::observation_type> obs_container;
				typedef typename obs_container::const_iterator obs_iterator;

				obs_container obs = p_sens->observations();
				obs_iterator end_it = obs.end();
				for (obs_iterator it = obs.begin();
					 it != end_it;
					 ++it)
				{
					this->data_estimator(cat).collect(it->value());
				}
			}
		}

		DCS_DEBUG_TRACE("(" << this << ") END Do SAMPLE - Count: " << ctl_count_ << "/" << ctl_skip_count_ << "/" << sysid_fail_count_ << "/" << ctl_fail_count_);
	}

    private: void do_control()
    {
		namespace ublas = ::boost::numeric::ublas;
		namespace ublasx = ::boost::numeric::ublasx;

		//typedef ::std::map<application_performance_category,numeric_vector_type> performance_container;
		typedef typename base_type::target_value_map::const_iterator target_iterator;
		typedef typename app_type::vm_pointer vm_pointer;
		typedef ::std::vector<vm_pointer> vm_container;
		typedef typename vm_container::iterator vm_iterator;
		typedef typename vm_container::const_iterator vm_citerator;

		DCS_DEBUG_TRACE("(" << this << ") BEGIN Do CONTROL - Count: " << ctl_count_ << "/" << ctl_skip_count_ << "/" << sysid_fail_count_ << "/" << ctl_fail_count_);

		const ::std::size_t ny = p_sysid_alg_->num_outputs();
		const ::std::size_t nu = p_sysid_alg_->num_inputs();
		const ::std::size_t na = p_sysid_alg_->output_order();
		const ::std::size_t nb = p_sysid_alg_->input_order();
		const ::std::size_t nk = p_sysid_alg_->input_delay();

		bool skip_ctl = false;
		::std::map<application_performance_category,numeric_vector_type> yhs; // actual performance measure (this control output)
		numeric_vector_type u(nu, 0); // actual resource share (the control input)

		++ctl_count_;

		vm_container vms = this->app().vms();

		// Update control output measures
		if (ny > 0)
		{
			target_iterator tgt_end_it = this->target_values().end();
			for (target_iterator tgt_it = this->target_values().begin();
				 tgt_it != tgt_end_it;
				 ++tgt_it)
			{
				const application_performance_category cat(tgt_it->first);

				// Compute a summary statistics of collected observation
				if (this->data_estimator(cat).count() > 0)
				{
					const real_type y = this->data_estimator(cat).estimate();
					const real_type yb = this->target_value(cat);

					yhs[cat] = ublas::element_div(ublas::scalar_vector<real_type>(ny, y), ublas::scalar_vector<real_type>(ny, yb));
DCS_DEBUG_TRACE("Observed Normalized Output for Category " << cat << ": " << yhs.at(cat));//XXX
				}
				else
				{
					// No observation collected during the last control interval
					//TODO: what can we do?
					// 1. Skip control?
					// 2. Use a data smoother (e.g., EWMA)?
					// The original paper does not consider this case. So we choose option #1
					DCS_DEBUG_TRACE("No observation collected during the last control interval -> Skip control");
					skip_ctl = true;
					break;
				}

#ifdef DCSXX_TESTBED_EXP_APP_MGR_RESET_ESTIMATION_EVERY_INTERVAL
				this->data_estimator(cat).reset();
#endif // DCSXX_TESTBED_EXP_APP_MGR_RESET_ESTIMATION_EVERY_INTERVAL
			}
		}
		if (!skip_ctl)
		{
			if (nu > 0)
			{
				//FIXME: actual share should be scaled according to the capacity of the "reference" machine

				::std::size_t v(0);

				vm_citerator vm_end_it = vms.end();
				for (vm_citerator vm_it = vms.begin();
					 vm_it != vm_end_it;
					 ++vm_it)
				{
					const vm_pointer p_vm(*vm_it);

					// check: p_vm != null
					DCS_DEBUG_ASSERT( p_vm );

					u(v) = p_vm->cpu_share()/**p_vm->max_num_vcpus()*/;

					++v;
				}
			}

			// Estimate system params
			bool ok(true);
			try
			{
				//FIXME: currently, we handle only one performance category at time
				numeric_vector_type new_yh;
				target_iterator tgt_end_it = this->target_values().end();
				for (target_iterator tgt_it = this->target_values().begin();
					 tgt_it != tgt_end_it;
					 ++tgt_it)
				{
					const application_performance_category cat(tgt_it->first);

					new_yh = p_sysid_alg_->estimate(yhs.at(cat), u);
DCS_DEBUG_TRACE("RLS estimation:");//XXX
DCS_DEBUG_TRACE("Performance category " << cat);//XXX
DCS_DEBUG_TRACE("yh=" << yhs.at(cat));//XXX
DCS_DEBUG_TRACE("u=" << u);//XXX
DCS_DEBUG_TRACE("new_yh=" << new_yh);//XXX
DCS_DEBUG_TRACE("Theta_hat=" << p_sysid_alg_->Theta_hat());//XXX
DCS_DEBUG_TRACE("P=" << p_sysid_alg_->P());//XXX
DCS_DEBUG_TRACE("phi=" << p_sysid_alg_->phi());//XXX
				}

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
				numeric_vector_type opt_u;
				try
				{
					// $\bar{\mathbf{u}}_a^*(k)= (\mathbf{b}_0\mathbf{b}_0^T+q\mathbf{I})^{-1}((1+\sum_{i=1}^{n_a}\hat{y}_a(k-i)-\sum_{i=1}^{n_b}\mathbf{b}_i^T\mathbf{u}_a(k-i))\mathbf{b}_0+q\sum_{i=1}^{n_b}\mathbf{u}_a(k-i))$
					// (b1^2 + q)^{-1}*(q*(u2 + u3) + (r - b2*u2 - b3*u3 + a1*y1 + a2*y2 + a3*y3)*b1)

//#ifdef DCS_TESTBED_EXP_PADALA2009_APP_MGR_USE_ARX_B0_SIGN_HEURISTIC
//					// Check on B(1) suggested by Karlsson et al "Dynamic Black-Box Performance Model Estimation for Self-Tuning Regulators", 2005
//					// This essentially consider the model as a linear model where u(k) is the free variabile.
//					// They compute the first partial derivative wrt to u(k) which gives the matrix B(1).
//					// In order to preverse reverse proportionality ==> diag(B(1)) < 0
//					if (ublasx::any(p_sysid_alg_->B(1), ::std::bind2nd(::std::greater_equal<real_type>(), 0)))
//					{
//						DCS_EXCEPTION_THROW(::std::runtime_error,
//											"Cannot compute optimal control input: First partial derivative of input-output model has positive elements on the main diagonal");
//					}
//
////FIXME: to be checked
////					// Try to predict a new y value and see if reverse proportionality hold
////					{
////						vector_type pred_out1(n_s_,0);
////						vector_type pred_out2(n_s_,0);
////
////						for (uint_type k = 1; k <= n_a_; ++k)
////						{
////							pred_out1 = pred_out1 - ublas::prod(ptr_ident_strategy_->A(k), ptr_ident_strategy_->y(k));
////						}
////						pred_out2 = pred_out1;
////						for (uint_type k = 1; k <= n_b_; ++k)
////						{
////							pred_out1 = pred_out1 + ublas::prod(ptr_ident_strategy_->B(k), ptr_ident_strategy_->u(k+d_-1));
////							if (k > 1)
////							{
////								pred_out2 = pred_out2 + ublas::prod(ptr_ident_strategy_->B(k), ptr_ident_strategy_->u(k+d_-1));
////							}
////							else
////							{
////								pred_out2 = pred_out2 + ublas::prod(ptr_ident_strategy_->B(1), ublas::scalar_vector<real_type>(n_s_, 1));
////							}
////						}
////
//////::std::cerr << "PREDICTION -- y1 = " << pred_out1 << " - y2 = " << pred_out2 << ::std::endl;//XXX
////						if (ublasx::any(pred_out2-pred_out1, ::std::bind2nd(::std::greater<real_type>(), 0)))
////						{
////							//DCS_EXCEPTION_THROW( ::std::runtime_error, "Cannot compute optimal control input: Bad prediction from Input-Output model" );
////							::std::ostringstream oss;
////							oss << "APP: " << app.id() << " - Bad prediction from Input-Output model";
////							::dcs::testbed::log_warn(cls_id_, oss.str());
////						}
////					}
//#endif // DCS_TESTBED_EXP_PADALA2009_APP_MGR_USE_ARX_B0_SIGN_HEURISTIC

					real_type yr(1); //FIXME: only one reference target is handled

					// Compute L = (b_0 b_0^T + qI)^{-1}
					numeric_vector_type b0(ublas::row(p_sysid_alg_->B(1), 0));
					numeric_matrix_type L;
					L = ublas::outer_prod(b0, b0)+q_*ublas::identity_matrix<real_type>(nu, nu);
					bool inv = ublasx::inv_inplace(L);

					DCS_ASSERT(inv,
							   DCS_EXCEPTION_THROW(::std::runtime_error,
												   "Cannot compute optimal control input: Matrix (b_0*b_0^T+q*I_{nu,nu}) is not invertible"));

					// Compute say = (a_1 yh(t-1) + a_2 yh(k-2))
					real_type say(0);
					for (::std::size_t k = 1; k <= na; ++k)
					{
						say += ublas::prod(p_sysid_alg_->A(k), p_sysid_alg_->y(k))(0);
					}
					// Compute:
					//  sbu = (b_1^Tu(k-1) + ... + b_{nu}^Tu(k-nb+1)
					//  su  = u(k-1) + ... + u(k-nb+1)
					real_type sbu(0);
					numeric_vector_type su(nu, 0);
					for (::std::size_t k = 2; k <= nb; ++k)
					{
						sbu += ublas::prod(p_sysid_alg_->B(k), p_sysid_alg_->u(k-2+nk))(0);
						su = su+p_sysid_alg_->u(k-2+nk);
					}
					// Compute R = (yr - a_1 yh(t-1) - a_2 yh(k-2) - b_1^Tu(k-1))b_0 + q u(k-1)
					//           = (yr + say - sbu)b_0 + q su
					numeric_vector_type R;
					R = (yr+say-sbu)*b0+q_*su;
					// Compute opt_u = (b_0 b_0^T + qI)^{-1}((yr - a_1 yh(t-1) - a_2 yh(k-2) - b_1^Tu(k-1))b_0 + q u(k-1))
					//               = L R
					opt_u = ublas::prod(L, R);

					if (ublasx::any(opt_u, ::std::bind2nd(::std::less<real_type>(), 0)))
					{
						++ctl_fail_count_;

						::std::ostringstream oss;
						oss << "Control not applied: computed negative share '" << opt_u << "'";
						::dcs::log_warn(DCS_LOGGING_AT, oss.str());

						ok = false;
					}
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

						const real_type old_share = p_vm->cpu_share();

						real_type new_share = opt_u(v);

						if (::dcs::math::float_traits<real_type>::definitely_less(new_share, default_min_share))
						{
							::std::ostringstream oss;
							oss << "Optimal share (" << new_share << ") too small; adjusted to " << default_min_share;
							::dcs::log_warn(DCS_LOGGING_AT, oss.str());
						}
//						else if (::dcs::math::float_traits<real_type>::definitely_greater(new_share, default_max_share))
//						{
//							::std::ostringstream oss;
//							oss << "Optimal share (" << new_share << ") too big; adjusted to " << default_max_share;
//							::dcs::log_warn(DCS_LOGGING_AT, oss.str());
//						}

						new_share = /*::std::min(*/::std::max(new_share, default_min_share)/*, default_max_share)*/;

//						u_.push_back(new_share);

DCS_DEBUG_TRACE("VM '" << p_vm->id() << "' - old-share: " << old_share << " - new-share: " << new_share);
						if (!::dcs::math::float_traits<real_type>::essentially_equal(old_share, new_share))
						{
							p_vm->cpu_share(new_share);
						}

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

		if (p_dat_ofs_)
		{
			*p_dat_ofs_ << ::std::time(0) << ",";
			const vm_citerator vm_end_it = vms.end();
			for (vm_citerator vm_it = vms.begin();
				 vm_it != vm_end_it;
				 ++vm_it)
			{
				const vm_pointer p_vm(*vm_it);

				// check: p_vm != null
				DCS_DEBUG_ASSERT( p_vm );

				if (vm_it != vms.begin())
				{
					*p_dat_ofs_ << ",";
				}
				*p_dat_ofs_ << p_vm->cpu_cap() << "," << p_vm->cpu_share();
			}
			*p_dat_ofs_ << ",";
			const target_iterator tgt_end_it = this->target_values().end();
			for (target_iterator tgt_it = this->target_values().begin();
			tgt_it != tgt_end_it;
			++tgt_it)
			{
				const application_performance_category cat(tgt_it->first);

				if (tgt_it != this->target_values().begin())
				{
					*p_dat_ofs_ << ",";
				}
				const real_type yh = this->data_estimator(cat).estimate();
				const real_type yr = tgt_it->second;
				const real_type yn = yh/yr;
				*p_dat_ofs_ << yh << "," << yn << "," << yr;
			}
			*p_dat_ofs_ << "," << ctl_count_ << "," << ctl_skip_count_ << "," << sysid_fail_count_ << "," << ctl_fail_count_;
			*p_dat_ofs_ << ::std::endl;
		}

		DCS_DEBUG_TRACE("(" << this << ") END Do CONTROL - Count: " << ctl_count_ << "/" << ctl_skip_count_ << "/" << sysid_fail_count_ << "/" << ctl_fail_count_);
	}

//	private: ::std::vector<real_type> do_control_inputs() const
//	{
//		return u_;
//	}

//	private: ::std::vector<real_type> do_control_outputs() const
//	{
//		return y_;
//	}


	private: real_type q_; ///< The stability factor
	private: sensor_map out_sensors_; ///< Sensor map for the application outputs
	private: sysid_strategy_pointer p_sysid_alg_;
//	private: performance_measure_map yr_; ///< The output vector to be tracked , mapped by application performance category
	private: ::std::size_t ctl_count_; ///< Number of times control function has been invoked
	private: ::std::size_t ctl_skip_count_; ///< Number of times control has been skipped
	private: ::std::size_t ctl_fail_count_; ///< Number of times control has failed
	private: ::std::size_t sysid_fail_count_; ///< Number of times system identification has failed
	private: ::std::string dat_fname_;
	private: ::boost::shared_ptr< ::std::ofstream > p_dat_ofs_;
//	private: numeric_vector_type u_; ///< Current values for inputs
//	private: numeric_vector_type y_; ///< Current (normalized) values for outputs
}; // padala2009_application_manager

template <typename T>
const typename padala2009_application_manager<T>::real_type padala2009_application_manager<T>::default_sampling_time = 1;

template <typename T>
const typename padala2009_application_manager<T>::real_type padala2009_application_manager<T>::default_control_time = 5;

template <typename T>
const typename padala2009_application_manager<T>::real_type padala2009_application_manager<T>::default_min_share = 0.0;

template <typename T>
const typename padala2009_application_manager<T>::real_type padala2009_application_manager<T>::default_max_share = 1.00;

template <typename T>
const typename padala2009_application_manager<T>::real_type padala2009_application_manager<T>::default_stability_factor = 2.0;

}} // Namespace dcs::testbed

#endif // DCS_TESTBED_PADALA2009_APPLICATION_MANAGER_HPP
