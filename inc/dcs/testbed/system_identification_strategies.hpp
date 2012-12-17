/**
 * \file dcs/testbed/system_identification_strategies.hpp
 *
 * \brief Class containing wrappers for different system identification
 *  strategies.
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

#ifndef DCS_TESTBED_SYSTEM_IDENTIFICATION_STRATEGIES_HPP
#define DCS_TESTBED_SYSTEM_IDENTIFICATION_STRATEGIES_HPP


#include <algorithm>
#include <boost/numeric/ublas/expression_types.hpp>
#ifdef DCS_DEBUG
#	include <boost/numeric/ublas/io.hpp>
#endif // DCS_DEBUG
#include <boost/numeric/ublas/matrix.hpp>
#include <boost/numeric/ublas/matrix_expression.hpp>
#include <boost/numeric/ublas/matrix_proxy.hpp>
#include <boost/numeric/ublas/vector.hpp>
#include <boost/numeric/ublas/vector_proxy.hpp>
#include <boost/numeric/ublas/traits.hpp>
#include <boost/numeric/ublasx/operation/max.hpp>
#include <boost/numeric/ublasx/operation/num_columns.hpp>
#include <boost/numeric/ublasx/operation/num_rows.hpp>
#include <boost/numeric/ublasx/operation/rcond.hpp>
#include <boost/numeric/ublasx/operation/size.hpp>
#include <boost/smart_ptr.hpp>
#include <cmath>
#include <dcs/assert.hpp>
#include <dcs/debug.hpp>
#include <dcs/exception.hpp>
#include <dcs/macro.hpp>
#include <dcs/sysid/algorithm/rls.hpp>
#include <dcs/testbed/system_identification_strategy_params.hpp>
#include <stdexcept>
#include <vector>


namespace dcs { namespace testbed {

template <typename TraitsT>
class base_arx_system_identification_strategy
{
	public: typedef TraitsT traits_type;
	public: typedef typename traits_type::real_type real_type;
	public: typedef ::boost::numeric::ublas::matrix<real_type> matrix_type;
	public: typedef ::boost::numeric::ublas::vector<real_type> vector_type;
	public: typedef typename ::boost::numeric::ublas::promote_traits<
							typename ::boost::numeric::ublas::matrix_traits<matrix_type>::size_type,
							typename ::boost::numeric::ublas::vector_traits<vector_type>::size_type
					>::promote_type size_type;


	public: base_arx_system_identification_strategy()
	: n_a_(0),
	  n_b_(0),
	  d_(0),
	  n_y_(0),
	  n_u_(0),
	  count_(0)
	{
	}

	public: base_arx_system_identification_strategy(size_type n_a, size_type n_b, size_type d, size_type n_y, size_type n_u)
	: n_a_(n_a),
	  n_b_(n_b),
	  d_(d),
	  n_y_(n_y),
	  n_u_(n_u),
	  count_(0)
	{
	}

	public: base_arx_system_identification_strategy(base_system_identification_strategy_params<traits_type> const& params)
	: n_a_(params.output_order()),
	  n_b_(params.input_order()),
	  d_(params.input_delay()),
	  n_y_(params.num_outputs()),
	  n_u_(params.num_inputs()),
	  count_(0)
	{
	}

	public: virtual ~base_arx_system_identification_strategy()
	{
	}

	public: size_type input_order() const
	{
		return n_b_;
	}

	public: size_type output_order() const
	{
		return n_a_;
	}

	public: size_type input_delay() const
	{
		return d_;
	}

	public: size_type num_inputs() const
	{
		return n_u_;
	}

	public: size_type num_outputs() const
	{
		return n_y_;
	}

	public: matrix_type Theta_hat() const
	{
		return do_Theta_hat();
	}

	public: matrix_type P() const
	{
		return do_P();
	}

	public: vector_type phi() const
	{
		return do_phi();
	}

	public: void init()
	{
		count_ = 0;

		do_init();
	}

	public: template <typename YVectorExprT, typename UVectorExprT>
		vector_type estimate(::boost::numeric::ublas::vector_expression<YVectorExprT> const& y,
							 ::boost::numeric::ublas::vector_expression<UVectorExprT> const& u)
	{
		++count_;

		return do_estimate(y, u);
	}

	public: matrix_type A(size_type k) const
	{
		return do_A(k);
	}

	public: matrix_type B(size_type k) const
	{
		return do_B(k);
	}

	public: size_type count() const
	{
		return count_;
	}

	public: void reset()
	{
		init();

		do_reset();
	}

	protected: virtual void do_reset()
	{
	}

	private: virtual matrix_type do_Theta_hat() const = 0;

	private: virtual matrix_type do_P() const = 0;

	private: virtual vector_type do_phi() const = 0;

	private: virtual void do_init() = 0;

	private: virtual vector_type do_estimate(vector_type const& y, vector_type const& u) = 0;

	private: virtual matrix_type do_A(size_type k) const = 0;

	private: virtual matrix_type do_B(size_type k) const = 0;


	/// The memory for the control output.
	private: size_type n_a_;
	/// The memory for the control input.
	private: size_type n_b_;
	/// Input delay (dead time).
	private: size_type d_;
	/// The size of the control output vector.
	private: size_type n_y_;
	/// The size of the augmented control input vector.
	private: size_type n_u_;
	/// Count the number of times RLS has been applied
	private: size_type count_;
}; // base_arx_system_identification_strategy


template <typename TraitsT>
class rls_arx_system_identification_strategy: public base_arx_system_identification_strategy<TraitsT>
{
	private: typedef base_arx_system_identification_strategy<TraitsT> base_type;
	public: typedef TraitsT traits_type;
	public: typedef typename traits_type::real_type real_type;
	public: typedef typename traits_type::uint_type uint_type;
	public: typedef typename base_type::size_type size_type;


	public: rls_arx_system_identification_strategy()
	: base_type(),
	  max_cov_heuristic_(false),
	  max_cov_heuristic_max_val_(0),
	  cond_cov_heuristic_(false),
	  cond_cov_heuristic_trust_digits_(0)
	{
	}

	public: rls_arx_system_identification_strategy(size_type n_a, size_type n_b, size_type d, size_type n_y, size_type n_u)
	: base_type(n_a, n_b, d, n_y, n_u),
	  max_cov_heuristic_(false),
	  max_cov_heuristic_max_val_(0),
	  cond_cov_heuristic_(false),
	  cond_cov_heuristic_trust_digits_(0)
	{
	}

	public: rls_arx_system_identification_strategy(rls_system_identification_strategy_params<traits_type> const& params)
	: base_type(params),
	  max_cov_heuristic_(false),
	  max_cov_heuristic_max_val_(0),
	  cond_cov_heuristic_(false),
	  cond_cov_heuristic_trust_digits_(0)
	{
	}

	public: void max_covariance_heuristic(bool value)
	{
		max_cov_heuristic_ = value;
	}

	public: bool max_covariance_heuristic() const
	{
		return max_cov_heuristic_;
	}

	public: void max_covariance_heuristic_max_value(real_type value)
	{
		max_cov_heuristic_max_val_ = value;
	}

	public: real_type max_covariance_heuristic_max_value() const
	{
		return max_cov_heuristic_max_val_;
	}

	public: void condition_number_covariance_heuristic(bool value)
	{
		cond_cov_heuristic_ = value;
	}

	public: bool condition_number_covariance_heuristic() const
	{
		return cond_cov_heuristic_;
	}

	public: void condition_number_covariance_heuristic_max_value(uint_type value)
	{
		cond_cov_heuristic_trust_digits_ = value;
	}

	public: uint_type condition_number_covariance_heuristic_trusted_digits() const
	{
		return cond_cov_heuristic_trust_digits_;
	}


	private: bool max_cov_heuristic_;
	private: real_type max_cov_heuristic_max_val_;
	private: bool cond_cov_heuristic_;
	private: uint_type cond_cov_heuristic_trust_digits_;
}; // rls_arx_system_identification_strategy


/**
 * \brief Proxy for directly applying the Recursive Least Square with
 *  forgetting-factor algorithm to a MIMO system model.
 *
 * \author Marco Guazzone (marco.guazzone@gmail.com)
 */
template <typename TraitsT>
class rls_ff_arx_mimo_proxy: public rls_arx_system_identification_strategy<TraitsT>
{
	private: typedef rls_arx_system_identification_strategy<TraitsT> base_type;
	public: typedef TraitsT traits_type;
	public: typedef typename traits_type::real_type real_type;
	public: typedef typename base_type::matrix_type matrix_type;
	public: typedef typename base_type::vector_type vector_type;
	public: typedef typename base_type::size_type size_type;


	public: rls_ff_arx_mimo_proxy()
	: base_type(),
	  ff_(0),
	  Theta_hat_(),
	  P_(),
	  phi_()
	{
	}

	public: rls_ff_arx_mimo_proxy(size_type n_a, size_type n_b, size_type d, size_type n_y, size_type n_u, real_type ff)
	: base_type(n_a, n_b, d, n_y, n_u),
	  ff_(ff),
	  Theta_hat_(),
	  P_(),
	  phi_()
	{
	}

	public: rls_ff_arx_mimo_proxy(rls_ff_system_identification_strategy_params<traits_type> const& params)
	: base_type(params),
	  ff_(params.forgetting_factor()),
	  Theta_hat_(),
	  P_(),
	  phi_()
	{
	}

	public: real_type forgetting_factor() const
	{
		return ff_;
	}

	private: matrix_type do_Theta_hat() const
	{
		return Theta_hat_;
	}

	private: matrix_type do_P() const
	{
		return P_;
	}

	private: vector_type do_phi() const
	{
		return phi_;
	}

	private: void do_init()
	{
		// Prepare the data structures for the RLS algorithm 
		::dcs::sysid::rls_arx_mimo_init(this->output_order(),
										this->input_order(),
										this->input_delay(),
										this->num_outputs(),
										this->num_inputs(),
										Theta_hat_,
										P_,
										phi_);

	}

	private: vector_type do_estimate(vector_type const& y, vector_type const& u)
	{
		// Apply enabled heuristics
		bool reset(false);
		// Apply the "max-covariance" heuristic (if enabled)
		if (!reset && this->max_covariance_heuristic())
		{
			if (::boost::numeric::ublasx::max(P_) > this->max_covariance_heuristic_max_value())
			{
				reset = true;
			}
		}
		// Apply the "condition-number-covariance" heuristic (if enabled)
		if (!reset && this->condition_number_covariance_heuristic())
		{
			// Use the following rule-of-thumb [1]:
			//   if cond(A)*eps >= 0.5*10^{-d} then d significant digits of the
			//   solution of the problem involving A are at least correct.
			// NOTE #1: we use reciprocal condition estimator since it's less
			//          time-consuming.
			// NOTE #2: we use log10 to avoid to compute 10^d which can
			//          potentially lead to overflow problems.
			//
			// References:
			// [1] Holistic Numerical Methods Institute,
			//     "Chapter 04.09 -- Adequacy of Solutions"
			//     Found online at http://numericalmethods.eng.usf.edu/mws/gen/04sle/mws_gen_sle_spe_adequacy.pdf

			real_type check_val = ::std::log10(static_cast<real_type>(2)*::std::numeric_limits<real_type>::epsilon())
								  + this->condition_number_covariance_heuristic_trusted_digits();

			if (::std::log10(::boost::numeric::ublasx::rcond(P_)) > check_val)
			{
				reset = true;
			}
		}
		if (reset)
		{
			this->reset();
		}

		// Estimate system parameters
//DCS_DEBUG_TRACE("BEGIN estimation");//XXX
//DCS_DEBUG_TRACE("y(k): " << y);//XXX
//DCS_DEBUG_TRACE("u(k): " << u);//XXX
//DCS_DEBUG_TRACE("Theta_hat(k): " << Theta_hat_);//XXX
//DCS_DEBUG_TRACE("P(k): " << P_);//XXX
//DCS_DEBUG_TRACE("phi(k): " << phi_);//XXX
		vector_type y_hat;
		y_hat = ::dcs::sysid::rls_ff_arx_mimo(y,
											  u,
											  ff_,
											  this->output_order(),
											  this->input_order(),
											  this->input_delay(),
											  Theta_hat_,
											  P_,
											  phi_);
//DCS_DEBUG_TRACE("New Theta_hat(k): " << Theta_hat_);//XXX
//DCS_DEBUG_TRACE("New P(k): " << P_);//XXX
//DCS_DEBUG_TRACE("New phi(k): " << phi_);//XXX
//DCS_DEBUG_TRACE("END estimation");//XXX
		return y_hat;
	}

	/// Return matrix A_k from \hat{\Theta}.
	private: matrix_type do_A(size_type k) const
	{
		namespace ublas = ::boost::numeric::ublas;

		DCS_DEBUG_ASSERT( k >= 1 && k <= this->output_order() );

		// Remember:
		//   \hat{\Theta} = [a_{11}^{1},     a_{21}^{1},     ...,  a_{n_y1}^{1};
		//                   ...,            ...,            ...,  ...;
		//                   a_{11}^{n_a},   a_{21}^{n_a},   ...,  a_{n_y1}^{n_a};
		//                   ...,            ...,            ...,  ...;
		//                   a_{1n_y}^{1},   a_{2n_y}^{1},   ...,  a_{n_yn_y}^{1};
		//                   ...,            ...,            ...,  ...;
		//                   a_{1n_y}^{n_a}, a_{2n_y}^{n_a}, ...,  a_{n_yn_y}^{n_a};
		//                   b_{11}^{1},     b_{21}^{1},     ...,  b_{n_y1}^{1};
		//                   ...,            ...,            ...,  ...;
		//                   b_{11}^{n_b},   b_{21}^{n_b},   ...,  b_{n_y1}^{n_b};
		//                   ...,            ...,            ...,  ...;
		//                   b_{1n_u}^{1},   b_{2n_u}^{1},   ...,  b_{n_yn_u}^{1};
		//                   ...,            ...,            ...,  ...;
		//                   b_{1n_u}^{n_b}, b_{2n_u}^{n_b}, ...,  b_{n_yn_u}^{n_b}]
		// So in \hat{\Theta} the matrix A_k stays at:
		//   A_k <- (\hat{\Theta}(k:n_a:n_y,:))^T
		matrix_type A_k; // (this->num_outputs(), this->num_outputs());
		A_k = ublas::trans(ublas::subslice(Theta_hat_, k-1, this->output_order(), this->num_outputs(), 0, 1, this->num_outputs()));

		return A_k;
	}

	/// Return matrix B_k from \hat{\Theta}.
	private: matrix_type do_B(size_type k) const
	{
		namespace ublas = ::boost::numeric::ublas;

		DCS_DEBUG_ASSERT( k >= 1 && k <= this->input_order() );

		// Remember:
		//   \hat{\Theta} = [a_{11}^{1},     a_{21}^{1},     ...,  a_{n_y1}^{1};
		//                   ...,            ...,            ...,  ...;
		//                   a_{11}^{n_a},   a_{21}^{n_a},   ...,  a_{n_y1}^{n_a};
		//                   ...,            ...,            ...,  ...;
		//                   a_{1n_y}^{1},   a_{2n_y}^{1},   ...,  a_{n_yn_y}^{1};
		//                   ...,            ...,            ...,  ...;
		//                   a_{1n_y}^{n_a}, a_{2n_y}^{n_a}, ...,  a_{n_yn_y}^{n_a};
		//                   b_{11}^{1},     b_{21}^{1},     ...,  b_{n_y1}^{1};
		//                   ...,            ...,            ...,  ...;
		//                   b_{11}^{n_b},   b_{21}^{n_b},   ...,  b_{n_y1}^{n_b};
		//                   ...,            ...,            ...,  ...;
		//                   b_{1n_u}^{1},   b_{2n_u}^{1},   ...,  b_{n_yn_u}^{1};
		//                   ...,            ...,            ...,  ...;
		//                   b_{1n_u}^{n_b}, b_{2n_u}^{n_b}, ...,  b_{n_yn_u}^{n_b}]
		// So in \hat{\Theta} the matrix B_k stays at:
		//   B_k <- (\hat{\Theta}(((n_a*n_y)+k):n_b:n_u,:))^T
		matrix_type B_k; // (n_y_, n_u_);
		B_k = ublas::trans(ublas::subslice(Theta_hat_, this->output_order()*this->num_outputs()+k-1, this->input_order(), this->num_inputs(), 0, 1, this->num_outputs()));

		return B_k;
	}


	/// Forgetting factor.
	private: real_type ff_;
	/// Matrix of system parameters estimated by RLS: [A_1 ... A_{n_a} B_1 ... B_{n_b}].
	private: matrix_type Theta_hat_;
	/// The covariance matrix.
	private: matrix_type P_;
	/// The regression vector.
	private: vector_type phi_;
}; // rls_ff_arx_mimo_proxy


/**
 * \brief Proxy to identify a MIMO system model by applying the Recursive Least
 *  Square with forgetting-factor algorithm to several MISO system models.
 *
 * \author Marco Guazzone (marco.guazzone@gmail.com)
 */
template <typename TraitsT>
class rls_ff_arx_miso_proxy: public rls_arx_system_identification_strategy<TraitsT>
{
	private: typedef rls_arx_system_identification_strategy<TraitsT> base_type;
	public: typedef TraitsT traits_type;
	public: typedef typename traits_type::real_type real_type;
	public: typedef typename base_type::matrix_type matrix_type;
	public: typedef typename base_type::vector_type vector_type;
	public: typedef typename base_type::size_type size_type;


	public: rls_ff_arx_miso_proxy()
	: base_type(),
	  ff_(0),
	  theta_hats_(),
	  Ps_(),
	  phis_()
	{
	}

	public: rls_ff_arx_miso_proxy(size_type n_a, size_type n_b, size_type d, size_type n_y, size_type n_u, real_type ff)
	: base_type(n_a,n_b,d,n_y,n_u),
	  ff_(ff),
	  theta_hats_(n_y),
	  Ps_(n_y),
	  phis_(n_y)
	{
	}

	public: rls_ff_arx_miso_proxy(rls_ff_system_identification_strategy_params<traits_type> const& params)
	: base_type(params),
	  ff_(params.forgetting_factor()),
	  theta_hats_(params.num_outputs()),
	  Ps_(params.num_outputs()),
	  phis_(params.num_outputs())
	{
	}

	public: real_type forgetting_factor() const
	{
		return ff_;
	}

	private: matrix_type do_Theta_hat() const
	{
		namespace ublas = ::boost::numeric::ublas;

		const size_type na(this->output_order());
		const size_type nb(this->input_order());
		const size_type ny(this->num_outputs());
		const size_type nu(this->num_inputs());
		const size_type nay(na*ny);
		const size_type nbu(nb*nu);
		const size_type n(nay+nbu);
		matrix_type X(n, ny, real_type/*zero()*/());

		for (size_type i = 0; i < ny; ++i)
		{
			// ith output => ith column of Theta_hat
			// ith column of Theta_hat = [0; ...; 0; a_{ii}^{1}
			const size_type k(i*na);
			ublas::matrix_column<matrix_type> mc(X,i);
			ublas::subrange(mc, k, k+na) = ublas::subrange(theta_hats_[i], 0, na);
			ublas::subrange(mc, nay, n) = ublas::subrange(theta_hats_[i], na, na+nbu);
		}

		return X;
	}

	private: matrix_type do_P() const
	{
		matrix_type aux_P;
//TODO: implement me!
		return aux_P;
	}

	private: vector_type do_phi() const
	{
		namespace ublas = ::boost::numeric::ublas;

		const size_type na(this->output_order());
		const size_type ny(this->num_outputs());
		const size_type nay(na*ny);
		const size_type nbu(this->input_order()*this->num_inputs());
		const size_type n(nay+nbu);
		vector_type x(n);

		for (size_type i = 0; i < ny; ++i)
		{
			const size_type k(i*na);
			ublas::subrange(x, k, k+na) = ublas::subrange(phis_[i], 0, na);
		}

		ublas::subrange(x, nay, n) = ublas::subrange(phis_[0], na, na+nbu);

		return x;
	}

	private: void do_init()
	{
		const size_type na(this->output_order());
		const size_type nb(this->input_order());
		const size_type d(this->input_delay());
		const size_type ny(this->num_outputs());
		const size_type nu(this->num_inputs());

		// Prepare the data structures for the RLS algorithm 
		for (size_type i = 0; i < ny; ++i)
		{
			::dcs::sysid::rls_arx_miso_init(na,
											nb,
											d,
											nu,
											theta_hats_[i],
											Ps_[i],
											phis_[i]);
		}
	}

	private: vector_type do_estimate(vector_type const& y, vector_type const& u)
	{
		const size_type na(this->output_order());
		const size_type nb(this->input_order());
		const size_type d(this->input_delay());
		const size_type ny(this->num_outputs());

		// Apply enabled heuristics
		bool reset(false);
		// Apply the "max-covariance" heuristic (if enabled)
		if (this->max_covariance_heuristic())
		{
			for (size_type i = 0; i < ny && !reset; ++i)
			{
				if (::boost::numeric::ublasx::max(Ps_[i]) > this->max_covariance_heuristic_max_value())
				{
					reset = true;
				}
			}
		}
		// Apply the "condition-number-covariance" heuristic (if enabled)
		if (this->condition_number_covariance_heuristic())
		{
			// Use the following rule-of-thumb:
			//   if cond(A)*eps >= 0.5*10^{-d} then d significant digits of the
			//   solution of the problem involving A are at least correct.
			// NOTE #1: we use reciprocal condition estimator since it's less
			//          time-consuming.
			// NOTE #2: we use log10 to avoid to compute 10^d which can
			//          potentially lead to overflow problems.
			//
			// References:
			// [1] Holistic Numerical Methods Institute,
			//     "Chapter 04.09 -- Adequacy of Solutions"
			//     Found online at http://numericalmethods.eng.usf.edu/mws/gen/04sle/mws_gen_sle_spe_adequacy.pdf

			real_type check_val = ::std::log10(static_cast<real_type>(2)*::std::numeric_limits<real_type>::epsilon())
								  + this->condition_number_covariance_heuristic_trusted_digits();

			for (size_type i = 0; i < ny && !reset; ++i)
			{
				if (::std::log10(::boost::numeric::ublasx::rcond(Ps_[i])) > check_val)
				{
					reset = true;
				}
			}
		}
		if (reset)
		{
			this->reset();
		}

		// Estimate system parameters
DCS_DEBUG_TRACE("BEGIN estimation");//XXX
DCS_DEBUG_TRACE("y(k): " << y);//XXX
DCS_DEBUG_TRACE("u(k): " << u);//XXX
//::std::cerr << "BEGIN estimation" << ::std::endl;//XXX
//::std::cerr << "y(k): " << y << ::std::endl;//XXX
//::std::cerr << "u(k): " << u << ::std::endl;//XXX
		vector_type y_hat(ny);
		for (size_type i = 0; i < ny; ++i)
		{
DCS_DEBUG_TRACE("theta_hat["<< i << "](k): " << theta_hats_[i]);//XXX
DCS_DEBUG_TRACE("P["<< i << "](k): " << Ps_[i]);//XXX
DCS_DEBUG_TRACE("phi["<< i << "](k): " << phis_[i]);//XXX
//::std::cerr << "theta_hat["<< i << "](k): " << theta_hats_[i] << ::std::endl;//XXX
//::std::cerr << "P["<< i << "](k): " << Ps_[i] << ::std::endl;//XXX
//::std::cerr << "phi["<< i << "](k): " << phis_[i] << ::std::endl;//XXX
			y_hat(i) = ::dcs::sysid::rls_ff_arx_miso(y(i),
													 u,
													 ff_,
													 na,
													 nb,
													 d,
													 theta_hats_[i],
													 Ps_[i],
													 phis_[i]);
DCS_DEBUG_TRACE("New theta_hat["<< i << "](k): " << theta_hats_[i]);//XXX
DCS_DEBUG_TRACE("New P["<< i << "](k): " << Ps_[i]);//XXX
DCS_DEBUG_TRACE("New rcond(P["<< i << "](k)): " << ::boost::numeric::ublasx::rcond(Ps_[i]));//XXX
DCS_DEBUG_TRACE("New phi["<< i << "](k): " << phis_[i]);//XXX
DCS_DEBUG_TRACE("New e["<< i << "](k): " << (y(i)-y_hat(i)));//XXX
//::std::cerr << "New theta_hat["<< i << "](k): " << theta_hats_[i] << ::std::endl;//XXX
//::std::cerr << "New P["<< i << "](k): " << Ps_[i] << ::std::endl;//XXX
//::std::cerr << "New rcond(P["<< i << "](k)): " << ::boost::numeric::ublasx::rcond(Ps_[i]) << ::std::endl;//XXX
//::std::cerr << "New phi["<< i << "](k): " << phis_[i] << ::std::endl;//XXX
//::std::cerr << "New e["<< i << "](k): " << (y(i)-y_hat(i)) << ::std::endl;//XXX
		}
DCS_DEBUG_TRACE("New y_hat(k): " << y_hat);//XXX
DCS_DEBUG_TRACE("END estimation");//XXX
//::std::cerr << "New y_hat(k): " << y_hat << ::std::endl;//XXX
//::std::cerr << "END estimation" << ::std::endl;//XXX

		return y_hat;
	}

	/// Return matrix A_k from \hat{\Theta}.
	private: matrix_type do_A(size_type k) const
	{
		namespace ublas = ::boost::numeric::ublas;

		DCS_DEBUG_ASSERT( k >= 1 && k <= this->output_order() );

		const size_type ny(this->num_outputs());

		// Remember, for each output i=1,...,n_y:
		//   \hat{\theta}_i = [a_{ii}^{1};
		//                     ...;
		//                     a_{ii}^{n_a};
		//                     b_{i1}^{1};
		//                     ...;
		//                     b_{i1}^{n_b};
		//                     ...;
		//                     b_{in_u}^{1};
		//                     ...;
		//                     b_{in_u}^{n_b}]
		// So in \hat{\theta}_i the ith diagonal element of matrix A_k stays at:
		//   A_k(i,i) <- \hat{\theta}_i(k)
		matrix_type A_k(ny, ny, real_type/*zero*/());
		for (size_type i = 0; i < ny; ++i)
		{
			A_k(i,i) = theta_hats_[i](k-1);
		}

		return A_k;
	}

	/// Return matrix B_k from \hat{\Theta}.
	private: matrix_type do_B(size_type k) const
	{
		namespace ublas = ::boost::numeric::ublas;

		DCS_DEBUG_ASSERT( k >= 1 && k <= this->input_order() );

		const size_type na(this->output_order());
		const size_type nb(this->input_order());
		const size_type ny(this->num_outputs());
		const size_type nu(this->num_inputs());

		// Remember, for each output i=1,...,n_y:
		//   \hat{\theta}_i = [a_{ii}^{1};
		//                     ...;
		//                     a_{ii}^{n_a};
		//                     b_{i1}^{1};
		//                     ...;
		//                     b_{i1}^{n_b};
		//                     ...;
		//                     b_{in_u}^{1};
		//                     ...;
		//                     b_{in_u}^{n_b}]
		// So in \hat{\theta}_i the ith row of matrix B_k stays at:
		//   B_k(i,:) <- (\hat{\theta}_i(((n_a+k):n_b:n_u))^T
		matrix_type B_k(ny, nu);
		for (size_type i = 0; i < ny; ++i)
		{
			ublas::row(B_k, i) = ublas::subslice(theta_hats_[i], na+k-1, nb, nu);
		}

		return B_k;
	}


	/// Forgetting factor.
	private: real_type ff_;
	/// Matrix of system parameters estimated by RLS: [A_1 ... A_{n_a} B_1 ... B_{n_b}].
	private: ::std::vector<vector_type> theta_hats_;
	/// The covariance matrix.
	private: ::std::vector<matrix_type> Ps_;
	/// The regression vector.
	private: ::std::vector<vector_type> phis_;
}; // rls_ff_arx_miso_proxy


/**
 * \brief Proxy to identify a MIMO system model by applying the Recursive Least
 *  Square with forgetting-factor algorithm to several MISO system models.
 *
 * The forgetting-factor is varied according to the following law [1]:
 * \f[
 *   \lambda(t) = \lambda_{\text{min}}+(1-\lambda_{\text{min}})2^{-\text{NINT}[\rho \epsilon^2(t)]}
 * \f]
 * where
 * - \f$\rho\f$, the <em>sensitivity gain</em>, is a design parameter.
 * - \f$\epsilon\f$, is the estimation error (i.e., the difference between the
 *    value of the current observed output and the one of the current estimated output).
 * - \f$\text{NINT}[\cdot]\f$ is the nearest integer of \f$[\cdot\]\f$.
 * .
 *
 * References:
 * -# Park et al.
 *    "Fast Tracking RLS Algorithm Using Novel Variable Forgetting Factor with Unity Zone",
 *    Electronic Letters, Vol. 23, 1991.
 * .
 *
 * \author Marco Guazzone (marco.guazzone@gmail.com)
 */
template <typename TraitsT>
class rls_park1991_arx_miso_proxy: public rls_arx_system_identification_strategy<TraitsT>
{
	private: typedef rls_arx_system_identification_strategy<TraitsT> base_type;
	public: typedef TraitsT traits_type;
	public: typedef typename traits_type::real_type real_type;
	public: typedef typename base_type::matrix_type matrix_type;
	public: typedef typename base_type::vector_type vector_type;
	public: typedef typename base_type::size_type size_type;


	public: rls_park1991_arx_miso_proxy()
	: base_type(),
	  ff_(0),
	  rho_(0),
	  theta_hats_(),
	  Ps_(),
	  phis_()
	{
	}

	public: rls_park1991_arx_miso_proxy(size_type n_a, size_type n_b, size_type d, size_type n_y, size_type n_u, real_type ff, real_type rho)
	: base_type(n_a,n_b,d,n_y,n_u),
	  ff_(ff),
	  rho_(rho),
	  theta_hats_(n_y),
	  Ps_(n_y),
	  phis_(n_y)
	{
	}

	public: rls_park1991_arx_miso_proxy(rls_park1991_system_identification_strategy_params<traits_type> const& params)
	: base_type(params),
	  ff_(params.forgetting_factor()),
	  rho_(params.sensitivity_gain()),
	  theta_hats_(params.num_outputs()),
	  Ps_(params.num_outputs()),
	  phis_(params.num_outputs())
	{
	}

	public: real_type forgetting_factor() const
	{
		return ff_;
	}

	public: real_type sensitivity_gain() const
	{
		return rho_;
	}

	private: matrix_type do_Theta_hat() const
	{
		namespace ublas = ::boost::numeric::ublas;

		const size_type na(this->output_order());
		const size_type nb(this->input_order());
		const size_type ny(this->num_outputs());
		const size_type nu(this->num_inputs());
		const size_type nay(na*ny);
		const size_type nbu(nb*nu);
		const size_type n(nay+nbu);
		matrix_type X(n, ny, real_type/*zero()*/());

		for (size_type i = 0; i < ny; ++i)
		{
			// ith output => ith column of Theta_hat
			// ith column of Theta_hat = [0; ...; 0; a_{ii}^{1}
			const size_type k(i*na);
			ublas::matrix_column<matrix_type> mc(X,i);
			ublas::subrange(mc, k, k+na) = ublas::subrange(theta_hats_[i], 0, na);
			ublas::subrange(mc, nay, n) = ublas::subrange(theta_hats_[i], na, na+nbu);
		}

		return X;
	}

	private: matrix_type do_P() const
	{
		matrix_type aux_P;
//TODO: implement me!!
		return aux_P;
	}

	private: vector_type do_phi() const
	{
		namespace ublas = ::boost::numeric::ublas;

		const size_type na(this->output_order());
		const size_type ny(this->num_outputs());
		const size_type nay(na*ny);
		const size_type nbu(this->input_order()*this->num_inputs());
		const size_type n(nay+nbu);
		vector_type x(n);

		for (size_type i = 0; i < ny; ++i)
		{
			const size_type k(i*na);
			ublas::subrange(x, k, k+na) = ublas::subrange(phis_[i], 0, na);
		}

		ublas::subrange(x, nay, n) = ublas::subrange(phis_[0], na, na+nbu);

		return x;
	}

	private: void do_init()
	{
		const size_type na(this->output_order());
		const size_type nb(this->input_order());
		const size_type d(this->input_delay());
		const size_type ny(this->num_outputs());
		const size_type nu(this->num_inputs());

		// Prepare the data structures for the RLS algorithm 
		for (size_type i = 0; i < ny; ++i)
		{
			::dcs::sysid::rls_arx_miso_init(na,
											nb,
											d,
											nu,
											theta_hats_[i],
											Ps_[i],
											phis_[i]);
		}
	}

	private: vector_type do_estimate(vector_type const& y, vector_type const& u)
	{
		const size_type na(this->output_order());
		const size_type nb(this->input_order());
		const size_type d(this->input_delay());
		const size_type ny(this->num_outputs());

		// Apply enabled heuristics
		bool reset(false);
		// Apply the "max-covariance" heuristic (if enabled)
		if (this->max_covariance_heuristic())
		{
			for (size_type i = 0; i < ny && !reset; ++i)
			{
				if (::boost::numeric::ublasx::max(Ps_[i]) > this->max_covariance_heuristic_max_value())
				{
					reset = true;
				}
			}
		}
		// Apply the "condition-number-covariance" heuristic (if enabled)
		if (this->condition_number_covariance_heuristic())
		{
			// Use the following rule-of-thumb:
			//   if cond(A)*eps >= 0.5*10^{-d} then d significant digits of the
			//   solution of the problem involving A are at least correct.
			// NOTE #1: we use reciprocal condition estimator since it's less
			//          time-consuming.
			// NOTE #2: we use log10 to avoid to compute 10^d which can
			//          potentially lead to overflow problems.
			//
			// References:
			// [1] Holistic Numerical Methods Institute,
			//     "Chapter 04.09 -- Adequacy of Solutions"
			//     Found online at http://numericalmethods.eng.usf.edu/mws/gen/04sle/mws_gen_sle_spe_adequacy.pdf

			real_type check_val = ::std::log10(static_cast<real_type>(2)*::std::numeric_limits<real_type>::epsilon())
								  + this->condition_number_covariance_heuristic_trusted_digits();

			for (size_type i = 0; i < ny && !reset; ++i)
			{
				if (::std::log10(::boost::numeric::ublasx::rcond(Ps_[i])) > check_val)
				{
					reset = true;
				}
			}
		}
		if (reset)
		{
			this->reset();
		}

		// Estimate system parameters
DCS_DEBUG_TRACE("BEGIN estimation");//XXX
DCS_DEBUG_TRACE("y(k): " << y);//XXX
DCS_DEBUG_TRACE("u(k): " << u);//XXX
//::std::cerr << "BEGIN estimation" << ::std::endl;//XXX
//::std::cerr << "y(k): " << y << ::std::endl;//XXX
//::std::cerr << "u(k): " << u << ::std::endl;//XXX
		vector_type y_hat(ny);
		for (size_type i = 0; i < ny; ++i)
		{
DCS_DEBUG_TRACE("theta_hat["<< i << "](k): " << theta_hats_[i]);//XXX
DCS_DEBUG_TRACE("P["<< i << "](k): " << Ps_[i]);//XXX
DCS_DEBUG_TRACE("phi["<< i << "](k): " << phis_[i]);//XXX
//::std::cerr << "theta_hat["<< i << "](k): " << theta_hats_[i] << ::std::endl;//XXX
//::std::cerr << "P["<< i << "](k): " << Ps_[i] << ::std::endl;//XXX
//::std::cerr << "phi["<< i << "](k): " << phis_[i] << ::std::endl;//XXX
			y_hat(i) = ::dcs::sysid::rls_park1991_arx_miso(y(i),
													 u,
													 ff_,
													 rho_,
													 na,
													 nb,
													 d,
													 theta_hats_[i],
													 Ps_[i],
													 phis_[i]);
DCS_DEBUG_TRACE("New theta_hat["<< i << "](k): " << theta_hats_[i]);//XXX
DCS_DEBUG_TRACE("New P["<< i << "](k): " << Ps_[i]);//XXX
DCS_DEBUG_TRACE("New rcond(P["<< i << "](k)): " << ::boost::numeric::ublasx::rcond(Ps_[i]));//XXX
DCS_DEBUG_TRACE("New phi["<< i << "](k): " << phis_[i]);//XXX
DCS_DEBUG_TRACE("New e["<< i << "](k): " << (y(i)-y_hat(i)));//XXX
//::std::cerr << "New theta_hat["<< i << "](k): " << theta_hats_[i] << ::std::endl;//XXX
//::std::cerr << "New P["<< i << "](k): " << Ps_[i] << ::std::endl;//XXX
//::std::cerr << "New rcond(P["<< i << "](k)): " << ::boost::numeric::ublasx::rcond(Ps_[i]) << ::std::endl;//XXX
//::std::cerr << "New phi["<< i << "](k): " << phis_[i] << ::std::endl;//XXX
//::std::cerr << "New e["<< i << "](k): " << (y(i)-y_hat(i)) << ::std::endl;//XXX
		}
DCS_DEBUG_TRACE("New y_hat(k): " << y_hat);//XXX
DCS_DEBUG_TRACE("END estimation");//XXX
//::std::cerr << "New y_hat(k): " << y_hat << ::std::endl;//XXX
//::std::cerr << "END estimation" << ::std::endl;//XXX

		return y_hat;
	}

	/// Return matrix A_k from \hat{\Theta}.
	private: matrix_type do_A(size_type k) const
	{
		namespace ublas = ::boost::numeric::ublas;

		DCS_DEBUG_ASSERT( k >= 1 && k <= this->output_order() );

		const size_type ny(this->num_outputs());

		// Remember, for each output i=1,...,n_y:
		//   \hat{\theta}_i = [a_{ii}^{1};
		//                     ...;
		//                     a_{ii}^{n_a};
		//                     b_{i1}^{1};
		//                     ...;
		//                     b_{i1}^{n_b};
		//                     ...;
		//                     b_{in_u}^{1};
		//                     ...;
		//                     b_{in_u}^{n_b}]
		// So in \hat{\theta}_i the ith diagonal element of matrix A_k stays at:
		//   A_k(i,i) <- \hat{\theta}_i(k)
		matrix_type A_k(ny, ny, real_type/*zero*/());
		for (size_type i = 0; i < ny; ++i)
		{
			A_k(i,i) = theta_hats_[i](k-1);
		}

		return A_k;
	}

	/// Return matrix B_k from \hat{\Theta}.
	private: matrix_type do_B(size_type k) const
	{
		namespace ublas = ::boost::numeric::ublas;

		DCS_DEBUG_ASSERT( k >= 1 && k <= this->input_order() );

		const size_type na(this->output_order());
		const size_type nb(this->input_order());
		const size_type ny(this->num_outputs());
		const size_type nu(this->num_inputs());

		// Remember, for each output i=1,...,n_y:
		//   \hat{\theta}_i = [a_{ii}^{1};
		//                     ...;
		//                     a_{ii}^{n_a};
		//                     b_{i1}^{1};
		//                     ...;
		//                     b_{i1}^{n_b};
		//                     ...;
		//                     b_{in_u}^{1};
		//                     ...;
		//                     b_{in_u}^{n_b}]
		// So in \hat{\theta}_i the ith row of matrix B_k stays at:
		//   B_k(i,:) <- (\hat{\theta}_i(((n_a+k):n_b:n_u))^T
		matrix_type B_k(ny, nu);
		for (size_type i = 0; i < ny; ++i)
		{
			ublas::row(B_k, i) = ublas::subslice(theta_hats_[i], na+k-1, nb, nu);
		}

		return B_k;
	}


	/// Forgetting factor.
	private: real_type ff_;
	/// Sensitivity gain.
	private: real_type rho_;
	/// Matrix of system parameters estimated by RLS: [A_1 ... A_{n_a} B_1 ... B_{n_b}].
	private: ::std::vector<vector_type> theta_hats_;
	/// The covariance matrix.
	private: ::std::vector<matrix_type> Ps_;
	/// The regression vector.
	private: ::std::vector<vector_type> phis_;
}; // rls_park1991_arx_miso_proxy


/**
 * \brief Proxy to identify a MIMO system model by applying the Recursive Least
 *  Square with forgetting-factor algorithm to several MISO system models.
 *
 * \author Marco Guazzone (marco.guazzone@gmail.com)
 */
template <typename TraitsT>
class rls_kulhavy1984_arx_miso_proxy: public rls_arx_system_identification_strategy<TraitsT>
{
	private: typedef rls_arx_system_identification_strategy<TraitsT> base_type;
	public: typedef TraitsT traits_type;
	public: typedef typename traits_type::real_type real_type;
	public: typedef typename base_type::matrix_type matrix_type;
	public: typedef typename base_type::vector_type vector_type;
	public: typedef typename base_type::size_type size_type;


	public: rls_kulhavy1984_arx_miso_proxy()
	: base_type(),
	  ff_(0),
	  theta_hats_(),
	  Ps_(),
	  phis_()
	{
	}

	public: rls_kulhavy1984_arx_miso_proxy(size_type n_a, size_type n_b, size_type d, size_type n_y, size_type n_u, real_type ff)
	: base_type(n_a,n_b,d,n_y,n_u),
	  ff_(ff),
	  theta_hats_(n_y),
	  Ps_(n_y),
	  phis_(n_y)
	{
	}

	public: rls_kulhavy1984_arx_miso_proxy(rls_kulhavy1984_system_identification_strategy_params<traits_type> const& params)
	: base_type(params),
	  ff_(params.forgetting_factor()),
	  theta_hats_(params.num_outputs()),
	  Ps_(params.num_outputs()),
	  phis_(params.num_outputs())
	{
	}

	public: real_type forgetting_factor() const
	{
		return ff_;
	}

	private: matrix_type do_Theta_hat() const
	{
		namespace ublas = ::boost::numeric::ublas;

		const size_type na(this->output_order());
		const size_type nb(this->input_order());
		const size_type ny(this->num_outputs());
		const size_type nu(this->num_inputs());
		const size_type nay(na*ny);
		const size_type nbu(nb*nu);
		const size_type n(nay+nbu);
		matrix_type X(n, ny, real_type/*zero()*/());

		for (size_type i = 0; i < ny; ++i)
		{
			// ith output => ith column of Theta_hat
			// ith column of Theta_hat = [0; ...; 0; a_{ii}^{1}
			const size_type k(i*na);
			ublas::matrix_column<matrix_type> mc(X,i);
			ublas::subrange(mc, k, k+na) = ublas::subrange(theta_hats_[i], 0, na);
			ublas::subrange(mc, nay, n) = ublas::subrange(theta_hats_[i], na, na+nbu);
		}

		return X;
	}

	private: matrix_type do_P() const
	{
		matrix_type aux_P;
//TODO: implement me!!
		return aux_P;
	}

	private: vector_type do_phi() const
	{
		namespace ublas = ::boost::numeric::ublas;

		const size_type na(this->output_order());
		const size_type ny(this->num_outputs());
		const size_type nay(na*ny);
		const size_type nbu(this->input_order()*this->num_inputs());
		const size_type n(nay+nbu);
		vector_type x(n);

		for (size_type i = 0; i < ny; ++i)
		{
			const size_type k(i*na);
			ublas::subrange(x, k, k+na) = ublas::subrange(phis_[i], 0, na);
		}

		ublas::subrange(x, nay, n) = ublas::subrange(phis_[0], na, na+nbu);

		return x;
	}

	private: void do_init()
	{
		const size_type na(this->output_order());
		const size_type nb(this->input_order());
		const size_type d(this->input_delay());
		const size_type ny(this->num_outputs());
		const size_type nu(this->num_inputs());

		// Prepare the data structures for the RLS algorithm 
		for (size_type i = 0; i < ny; ++i)
		{
			::dcs::sysid::rls_arx_miso_init(na,
											nb,
											d,
											nu,
											theta_hats_[i],
											Ps_[i],
											phis_[i]);
		}
	}

	private: vector_type do_estimate(vector_type const& y, vector_type const& u)
	{
		const size_type na(this->output_order());
		const size_type nb(this->input_order());
		const size_type d(this->input_delay());
		const size_type ny(this->num_outputs());

		// Apply enabled heuristics
		bool reset(false);
		// Apply the "max-covariance" heuristic (if enabled)
		if (this->max_covariance_heuristic())
		{
			for (size_type i = 0; i < ny && !reset; ++i)
			{
				if (::boost::numeric::ublasx::max(Ps_[i]) > this->max_covariance_heuristic_max_value())
				{
					reset = true;
				}
			}
		}
		// Apply the "condition-number-covariance" heuristic (if enabled)
		if (this->condition_number_covariance_heuristic())
		{
			// Use the following rule-of-thumb:
			//   if cond(A)*eps >= 0.5*10^{-d} then d significant digits of the
			//   solution of the problem involving A are at least correct.
			// NOTE #1: we use reciprocal condition estimator since it's less
			//          time-consuming.
			// NOTE #2: we use log10 to avoid to compute 10^d which can
			//          potentially lead to overflow problems.
			//
			// References:
			// [1] Holistic Numerical Methods Institute,
			//     "Chapter 04.09 -- Adequacy of Solutions"
			//     Found online at http://numericalmethods.eng.usf.edu/mws/gen/04sle/mws_gen_sle_spe_adequacy.pdf

			real_type check_val = ::std::log10(static_cast<real_type>(2)*::std::numeric_limits<real_type>::epsilon())
								  + this->condition_number_covariance_heuristic_trusted_digits();

			for (size_type i = 0; i < ny && !reset; ++i)
			{
				if (::std::log10(::boost::numeric::ublasx::rcond(Ps_[i])) > check_val)
				{
					reset = true;
				}
			}
		}
		if (reset)
		{
			this->reset();
		}

		// Estimate system parameters
DCS_DEBUG_TRACE("BEGIN estimation");//XXX
DCS_DEBUG_TRACE("y(k): " << y);//XXX
DCS_DEBUG_TRACE("u(k): " << u);//XXX
		vector_type y_hat(ny);
		for (size_type i = 0; i < ny; ++i)
		{
DCS_DEBUG_TRACE("theta_hat["<< i << "](k): " << theta_hats_[i]);//XXX
DCS_DEBUG_TRACE("P["<< i << "](k): " << Ps_[i]);//XXX
DCS_DEBUG_TRACE("phi["<< i << "](k): " << phis_[i]);//XXX
			y_hat(i) = ::dcs::sysid::rls_kulhavy1984_arx_miso(y(i),
													 u,
													 ff_,
													 na,
													 nb,
													 d,
													 theta_hats_[i],
													 Ps_[i],
													 phis_[i]);
DCS_DEBUG_TRACE("New theta_hat["<< i << "](k): " << theta_hats_[i]);//XXX
DCS_DEBUG_TRACE("New P["<< i << "](k): " << Ps_[i]);//XXX
DCS_DEBUG_TRACE("New rcond(P["<< i << "](k)): " << ::boost::numeric::ublasx::rcond(Ps_[i]));//XXX
DCS_DEBUG_TRACE("New phi["<< i << "](k): " << phis_[i]);//XXX
DCS_DEBUG_TRACE("New e["<< i << "](k): " << (y(i)-y_hat(i)));//XXX
		}
DCS_DEBUG_TRACE("New y_hat(k): " << y_hat);//XXX
DCS_DEBUG_TRACE("END estimation");//XXX

		return y_hat;
	}

	/// Return matrix A_k from \hat{\Theta}.
	private: matrix_type do_A(size_type k) const
	{
		namespace ublas = ::boost::numeric::ublas;

		DCS_DEBUG_ASSERT( k >= 1 && k <= this->output_order() );

		const size_type ny(this->num_outputs());

		// Remember, for each output i=1,...,n_y:
		//   \hat{\theta}_i = [a_{ii}^{1};
		//                     ...;
		//                     a_{ii}^{n_a};
		//                     b_{i1}^{1};
		//                     ...;
		//                     b_{i1}^{n_b};
		//                     ...;
		//                     b_{in_u}^{1};
		//                     ...;
		//                     b_{in_u}^{n_b}]
		// So in \hat{\theta}_i the ith diagonal element of matrix A_k stays at:
		//   A_k(i,i) <- \hat{\theta}_i(k)
		matrix_type A_k(ny, ny, real_type/*zero*/());
		for (size_type i = 0; i < ny; ++i)
		{
			A_k(i,i) = theta_hats_[i](k-1);
		}

		return A_k;
	}

	/// Return matrix B_k from \hat{\Theta}.
	private: matrix_type do_B(size_type k) const
	{
		namespace ublas = ::boost::numeric::ublas;

		DCS_DEBUG_ASSERT( k >= 1 && k <= this->input_order() );

		const size_type na(this->output_order());
		const size_type nb(this->input_order());
		const size_type ny(this->num_outputs());
		const size_type nu(this->num_inputs());

		// Remember, for each output i=1,...,n_y:
		//   \hat{\theta}_i = [a_{ii}^{1};
		//                     ...;
		//                     a_{ii}^{n_a};
		//                     b_{i1}^{1};
		//                     ...;
		//                     b_{i1}^{n_b};
		//                     ...;
		//                     b_{in_u}^{1};
		//                     ...;
		//                     b_{in_u}^{n_b}]
		// So in \hat{\theta}_i the ith row of matrix B_k stays at:
		//   B_k(i,:) <- (\hat{\theta}_i(((n_a+k):n_b:n_u))^T
		matrix_type B_k(ny, nu);
		for (size_type i = 0; i < ny; ++i)
		{
			ublas::row(B_k, i) = ublas::subslice(theta_hats_[i], na+k-1, nb, nu);
		}

		return B_k;
	}


	/// Forgetting factor.
	private: real_type ff_;
	/// Matrix of system parameters estimated by RLS: [A_1 ... A_{n_a} B_1 ... B_{n_b}].
	private: ::std::vector<vector_type> theta_hats_;
	/// The covariance matrix.
	private: ::std::vector<matrix_type> Ps_;
	/// The regression vector.
	private: ::std::vector<vector_type> phis_;
}; // rls_kulhavy1984_arx_miso_proxy


/**
 * \brief Proxy to identify a MIMO system model by applying the Recursive Least
 *  Square with forgetting-factor algorithm to several MISO system models.
 *
 * \author Marco Guazzone (marco.guazzone@gmail.com)
 */
template <typename TraitsT>
class rls_bittanti1990_arx_miso_proxy: public rls_arx_system_identification_strategy<TraitsT>
{
	private: typedef rls_arx_system_identification_strategy<TraitsT> base_type;
	public: typedef TraitsT traits_type;
	public: typedef typename traits_type::real_type real_type;
	public: typedef typename base_type::matrix_type matrix_type;
	public: typedef typename base_type::vector_type vector_type;
	public: typedef typename base_type::size_type size_type;


	public: rls_bittanti1990_arx_miso_proxy()
	: base_type(),
	  ff_(0),
	  delta_(0),
	  theta_hats_(),
	  Ps_(),
	  phis_()
	{
	}

	public: rls_bittanti1990_arx_miso_proxy(size_type n_a, size_type n_b, size_type d, size_type n_y, size_type n_u, real_type ff, real_type delta)
	: base_type(n_a,n_b,d,n_y,n_u),
	  ff_(ff),
	  delta_(delta),
	  theta_hats_(n_y),
	  Ps_(n_y),
	  phis_(n_y)
	{
	}

	public: rls_bittanti1990_arx_miso_proxy(rls_bittanti1990_system_identification_strategy_params<traits_type> const& params)
	: base_type(params),
	  ff_(params.forgetting_factor()),
	  delta_(params.correction_factor()),
	  theta_hats_(params.num_outputs()),
	  Ps_(params.num_outputs()),
	  phis_(params.num_outputs())
	{
	}

	public: real_type forgetting_factor() const
	{
		return ff_;
	}

	public: real_type correction_factor() const
	{
		return delta_;
	}

	private: matrix_type do_Theta_hat() const
	{
		namespace ublas = ::boost::numeric::ublas;

		const size_type na(this->output_order());
		const size_type nb(this->input_order());
		const size_type ny(this->num_outputs());
		const size_type nu(this->num_inputs());
		const size_type nay(na*ny);
		const size_type nbu(nb*nu);
		const size_type n(nay+nbu);
		matrix_type X(n, ny, real_type/*zero()*/());

		for (size_type i = 0; i < ny; ++i)
		{
			// ith output => ith column of Theta_hat
			// ith column of Theta_hat = [0; ...; 0; a_{ii}^{1}
			const size_type k(i*na);
			ublas::matrix_column<matrix_type> mc(X,i);
			ublas::subrange(mc, k, k+na) = ublas::subrange(theta_hats_[i], 0, na);
			ublas::subrange(mc, nay, n) = ublas::subrange(theta_hats_[i], na, na+nbu);
		}

		return X;
	}

	private: matrix_type do_P() const
	{
		matrix_type aux_P;
//TODO: implement me!!
		return aux_P;
	}

	private: vector_type do_phi() const
	{
		namespace ublas = ::boost::numeric::ublas;

		const size_type na(this->output_order());
		const size_type ny(this->num_outputs());
		const size_type nay(na*ny);
		const size_type nbu(this->input_order()*this->num_inputs());
		const size_type n(nay+nbu);
		vector_type x(n);

		for (size_type i = 0; i < ny; ++i)
		{
			const size_type k(i*na);
			ublas::subrange(x, k, k+na) = ublas::subrange(phis_[i], 0, na);
		}

		ublas::subrange(x, nay, n) = ublas::subrange(phis_[0], na, na+nbu);

		return x;
	}

	private: void do_init()
	{
		const size_type na(this->output_order());
		const size_type nb(this->input_order());
		const size_type d(this->input_delay());
		const size_type ny(this->num_outputs());
		const size_type nu(this->num_inputs());

		// Prepare the data structures for the RLS algorithm 
		for (size_type i = 0; i < ny; ++i)
		{
			::dcs::sysid::rls_arx_miso_init(na,
											nb,
											d,
											nu,
											theta_hats_[i],
											Ps_[i],
											phis_[i]);
		}
	}

	private: vector_type do_estimate(vector_type const& y, vector_type const& u)
	{
		const size_type na(this->output_order());
		const size_type nb(this->input_order());
		const size_type d(this->input_delay());
		const size_type ny(this->num_outputs());

		// Apply enabled heuristics
		bool reset(false);
		// Apply the "max-covariance" heuristic (if enabled)
		if (this->max_covariance_heuristic())
		{
			for (size_type i = 0; i < ny && !reset; ++i)
			{
				if (::boost::numeric::ublasx::max(Ps_[i]) > this->max_covariance_heuristic_max_value())
				{
					reset = true;
				}
			}
		}
		// Apply the "condition-number-covariance" heuristic (if enabled)
		if (this->condition_number_covariance_heuristic())
		{
			// Use the following rule-of-thumb:
			//   if cond(A)*eps >= 0.5*10^{-d} then d significant digits of the
			//   solution of the problem involving A are at least correct.
			// NOTE #1: we use reciprocal condition estimator since it's less
			//          time-consuming.
			// NOTE #2: we use log10 to avoid to compute 10^d which can
			//          potentially lead to overflow problems.
			//
			// References:
			// [1] Holistic Numerical Methods Institute,
			//     "Chapter 04.09 -- Adequacy of Solutions"
			//     Found online at http://numericalmethods.eng.usf.edu/mws/gen/04sle/mws_gen_sle_spe_adequacy.pdf

			real_type check_val = ::std::log10(static_cast<real_type>(2)*::std::numeric_limits<real_type>::epsilon())
								  + this->condition_number_covariance_heuristic_trusted_digits();

			for (size_type i = 0; i < ny && !reset; ++i)
			{
				if (::std::log10(::boost::numeric::ublasx::rcond(Ps_[i])) > check_val)
				{
					reset = true;
				}
			}
		}
		if (reset)
		{
			this->reset();
		}

		// Estimate system parameters
DCS_DEBUG_TRACE("BEGIN estimation");//XXX
DCS_DEBUG_TRACE("y(k): " << y);//XXX
DCS_DEBUG_TRACE("u(k): " << u);//XXX
		vector_type y_hat(ny);
		for (size_type i = 0; i < ny; ++i)
		{
DCS_DEBUG_TRACE("theta_hat["<< i << "](k): " << theta_hats_[i]);//XXX
DCS_DEBUG_TRACE("P["<< i << "](k): " << Ps_[i]);//XXX
DCS_DEBUG_TRACE("phi["<< i << "](k): " << phis_[i]);//XXX
			y_hat(i) = ::dcs::sysid::rls_bittanti1990_arx_miso(y(i),
															   u,
															   ff_,
															   na,
															   nb,
															   d,
															   theta_hats_[i],
															   Ps_[i],
															   phis_[i],
															   delta_);
DCS_DEBUG_TRACE("New theta_hat["<< i << "](k): " << theta_hats_[i]);//XXX
DCS_DEBUG_TRACE("New P["<< i << "](k): " << Ps_[i]);//XXX
DCS_DEBUG_TRACE("New rcond(P["<< i << "](k)): " << ::boost::numeric::ublasx::rcond(Ps_[i]));//XXX
DCS_DEBUG_TRACE("New phi["<< i << "](k): " << phis_[i]);//XXX
DCS_DEBUG_TRACE("New e["<< i << "](k): " << (y(i)-y_hat(i)));//XXX
		}
DCS_DEBUG_TRACE("New y_hat(k): " << y_hat);//XXX
DCS_DEBUG_TRACE("END estimation");//XXX

		return y_hat;
	}

	/// Return matrix A_k from \hat{\Theta}.
	private: matrix_type do_A(size_type k) const
	{
		namespace ublas = ::boost::numeric::ublas;

		DCS_DEBUG_ASSERT( k >= 1 && k <= this->output_order() );

		const size_type ny(this->num_outputs());

		// Remember, for each output i=1,...,n_y:
		//   \hat{\theta}_i = [a_{ii}^{1};
		//                     ...;
		//                     a_{ii}^{n_a};
		//                     b_{i1}^{1};
		//                     ...;
		//                     b_{i1}^{n_b};
		//                     ...;
		//                     b_{in_u}^{1};
		//                     ...;
		//                     b_{in_u}^{n_b}]
		// So in \hat{\theta}_i the ith diagonal element of matrix A_k stays at:
		//   A_k(i,i) <- \hat{\theta}_i(k)
		matrix_type A_k(ny, ny, real_type/*zero*/());
		for (size_type i = 0; i < ny; ++i)
		{
			A_k(i,i) = theta_hats_[i](k-1);
		}

		return A_k;
	}

	/// Return matrix B_k from \hat{\Theta}.
	private: matrix_type do_B(size_type k) const
	{
		namespace ublas = ::boost::numeric::ublas;

		DCS_DEBUG_ASSERT( k >= 1 && k <= this->input_order() );

		const size_type na(this->output_order());
		const size_type nb(this->input_order());
		const size_type ny(this->num_outputs());
		const size_type nu(this->num_inputs());

		// Remember, for each output i=1,...,n_y:
		//   \hat{\theta}_i = [a_{ii}^{1};
		//                     ...;
		//                     a_{ii}^{n_a};
		//                     b_{i1}^{1};
		//                     ...;
		//                     b_{i1}^{n_b};
		//                     ...;
		//                     b_{in_u}^{1};
		//                     ...;
		//                     b_{in_u}^{n_b}]
		// So in \hat{\theta}_i the ith row of matrix B_k stays at:
		//   B_k(i,:) <- (\hat{\theta}_i(((n_a+k):n_b:n_u))^T
		matrix_type B_k(ny, nu);
		for (size_type i = 0; i < ny; ++i)
		{
			ublas::row(B_k, i) = ublas::subslice(theta_hats_[i], na+k-1, nb, nu);
		}

		return B_k;
	}


	/// Forgetting factor.
	private: real_type ff_;
	/// Bittanti's correction factor.
	private: real_type delta_;
	/// Matrix of system parameters estimated by RLS: [A_1 ... A_{n_a} B_1 ... B_{n_b}].
	private: ::std::vector<vector_type> theta_hats_;
	/// The covariance matrix.
	private: ::std::vector<matrix_type> Ps_;
	/// The regression vector.
	private: ::std::vector<vector_type> phis_;
}; // rls_bittanti1990_arx_miso_proxy


template <typename TraitsT>
::boost::shared_ptr< base_arx_system_identification_strategy<TraitsT> > make_system_identification_strategy(base_system_identification_strategy_params<TraitsT> const& params)
{
	typedef TraitsT traits_type;
	typedef base_arx_system_identification_strategy<traits_type> strategy_type;
	typedef ::boost::shared_ptr<strategy_type> strategy_pointer;

	strategy_pointer ptr_strategy;

	switch (params.category())
	{
		case rls_bittanti1990_system_identification_strategy:
			{
				typedef rls_bittanti1990_system_identification_strategy_params<traits_type> const* strategy_params_impl_pointer;

				strategy_params_impl_pointer ptr_params_impl = dynamic_cast<strategy_params_impl_pointer>(&params);
				if (!ptr_params_impl)
				{
					DCS_EXCEPTION_THROW(::std::runtime_error, "Failed to retrieve RLS FF strategy parameters.");
				}
				if (ptr_params_impl->mimo_as_miso())
				{
					typedef rls_bittanti1990_arx_miso_proxy<traits_type> strategy_impl_type;

					ptr_strategy = ::boost::make_shared<strategy_impl_type>(*ptr_params_impl);
				}
				else
				{
					DCS_EXCEPTION_THROW(::std::runtime_error, "MIMO RLS (Bittanti, 1990) has not been implemented yet.");
//					typedef rls_bittanti1990_arx_mimo_proxy<traits_type> strategy_impl_type;
//
//					ptr_strategy = ::boost::make_shared<strategy_impl_type>(*ptr_params_impl);
				}
			}
			break;
		case rls_ff_system_identification_strategy:
			{
				typedef rls_ff_system_identification_strategy_params<traits_type> const* strategy_params_impl_pointer;

				strategy_params_impl_pointer ptr_params_impl = dynamic_cast<strategy_params_impl_pointer>(&params);
				if (!ptr_params_impl)
				{
					DCS_EXCEPTION_THROW(::std::runtime_error, "Failed to retrieve RLS FF strategy parameters.");
				}
				if (ptr_params_impl->mimo_as_miso())
				{
					typedef rls_ff_arx_miso_proxy<traits_type> strategy_impl_type;

					ptr_strategy = ::boost::make_shared<strategy_impl_type>(*ptr_params_impl);
				}
				else
				{
					typedef rls_ff_arx_mimo_proxy<traits_type> strategy_impl_type;

					ptr_strategy = ::boost::make_shared<strategy_impl_type>(*ptr_params_impl);
				}
			}
			break;
		case rls_kulhavy1984_system_identification_strategy:
			{
				typedef rls_kulhavy1984_system_identification_strategy_params<traits_type> const* strategy_params_impl_pointer;

				strategy_params_impl_pointer ptr_params_impl = dynamic_cast<strategy_params_impl_pointer>(&params);
				if (!ptr_params_impl)
				{
					DCS_EXCEPTION_THROW(::std::runtime_error, "Failed to retrieve RLS FF strategy parameters.");
				}
				if (ptr_params_impl->mimo_as_miso())
				{
					typedef rls_kulhavy1984_arx_miso_proxy<traits_type> strategy_impl_type;

					ptr_strategy = ::boost::make_shared<strategy_impl_type>(*ptr_params_impl);
				}
				else
				{
					DCS_EXCEPTION_THROW(::std::runtime_error, "MIMO RLS (Kulhavy, 1984) has not been implemented yet.");
//					typedef rls_kulhavy1984_arx_mimo_proxy<traits_type> strategy_impl_type;
//
//					ptr_strategy = ::boost::make_shared<strategy_impl_type>(*ptr_params_impl);
				}
			}
			break;
		case rls_park1991_system_identification_strategy:
			{
				typedef rls_park1991_system_identification_strategy_params<traits_type> const* strategy_params_impl_pointer;

				strategy_params_impl_pointer ptr_params_impl = dynamic_cast<strategy_params_impl_pointer>(&params);
				if (!ptr_params_impl)
				{
					DCS_EXCEPTION_THROW(::std::runtime_error, "Failed to retrieve RLS FF strategy parameters.");
				}
				if (ptr_params_impl->mimo_as_miso())
				{
					typedef rls_park1991_arx_miso_proxy<traits_type> strategy_impl_type;

					ptr_strategy = ::boost::make_shared<strategy_impl_type>(*ptr_params_impl);
				}
				else
				{
					DCS_EXCEPTION_THROW(::std::runtime_error, "MIMO RLS (Park, 1991) has not been implemented yet.");
//					typedef rls_park1991_arx_mimo_proxy<traits_type> strategy_impl_type;
//
//					ptr_strategy = ::boost::make_shared<strategy_impl_type>(*ptr_params_impl);
				}
			}
			break;
	}

	return ptr_strategy;
}

}} // Namespace dcs::testbed


#endif // DCS_TESTBED_SYSTEM_IDENTIFICATION_STRATEGIES_HPP
