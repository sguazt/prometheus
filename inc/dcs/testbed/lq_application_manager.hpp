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
#include <boost/numeric/ublas/traits.hpp>
#include <boost/numeric/ublasx/operation/all.hpp>
#include <boost/numeric/ublasx/operation/inv.hpp>
#include <boost/numeric/ublasx/operation/isfinite.hpp>
#include <boost/numeric/ublasx/operation/num_columns.hpp>
#include <boost/numeric/ublasx/operation/num_rows.hpp>
#include <boost/numeric/ublasx/operation/size.hpp>
#include <boost/smart_ptr.hpp>
#include <cstddef>
#include <dcs/assert.hpp>
#include <dcs/control/analysis/controllability.hpp>
#include <dcs/control/analysis/detectability.hpp>
#include <dcs/control/analysis/observability.hpp>
#include <dcs/control/analysis/stabilizability.hpp>
#include <dcs/control/design/dlqry.hpp>
#include <dcs/debug.hpp>
#include <dcs/exception.hpp>
#include <dcs/logging.hpp>
#include <dcs/macro.hpp>
#include <dcs/math/traits/float.hpp>
#include <dcs/testbed/application_performance_category.hpp>
#include <dcs/testbed/base_application_manager.hpp>
#include <dcs/testbed/system_identification_strategies.hpp>
#include <limits>
#include <map>
#include <stdexcept>


namespace dcs { namespace testbed {

namespace detail { namespace /*<unnamed>*/ {


#if defined(DCS_TESTBED_EXP_LQ_APP_MGR_USE_ALT_SS) && DCS_TESTBED_EXP_LQ_APP_MGR_USE_ALT_SS == 'X'

template <
//	typename TraitsT,
	typename SysIdentStrategyT,
	typename AMatrixExprT,
	typename BMatrixExprT,
	typename CMatrixExprT,
	typename DMatrixExprT
>
void make_ss(SysIdentStrategyT const& sys_ident_strategy,
			 ::boost::numeric::ublas::matrix_container<AMatrixExprT>& A,
			 ::boost::numeric::ublas::matrix_container<BMatrixExprT>& B,
			 ::boost::numeric::ublas::matrix_container<CMatrixExprT>& C,
			 ::boost::numeric::ublas::matrix_container<DMatrixExprT>& D)
{
//DCS_DEBUG_TRACE("BEGIN make_ss");//XXX
	namespace ublas = ::boost::numeric::ublas;

	typedef typename ublas::promote_traits<
				typename ublas::promote_traits<
					typename ublas::promote_traits<
						typename ublas::matrix_traits<AMatrixExprT>::value_type,
						typename ublas::matrix_traits<BMatrixExprT>::value_type
					>::promote_type,
					typename ublas::matrix_traits<CMatrixExprT>::value_type
				>::promote_type,
				typename ublas::matrix_traits<DMatrixExprT>::value_type
			>::promote_type value_type;
	typedef ::std::size_t size_type; //FIXME: use type-promotion?

	const size_type rls_n_a(sys_ident_strategy.output_order());
	const size_type rls_n_b(sys_ident_strategy.input_order());
//	const size_type rls_d(sys_ident_strategy.input_delay());
	const size_type rls_n_y(sys_ident_strategy.num_outputs());
	const size_type rls_n_u(sys_ident_strategy.num_inputs());
	const size_type n_x(rls_n_a*rls_n_y+(rls_n_b-1)*rls_n_u);
	const size_type n_u(rls_n_u);
//	const size_type n(::std::max(n_x,n_u));
	const size_type n_y(1);

	// Create the state matrix A
	// A=[ 0        I          0         ...  0    0        I          0         ...  0  ;
	//     0        0          I         ...  0    0        0          I         ...  0  ;
	//     .        .          .         ...  .    .        .          0         ...  .
	//     .        .          .         ...  .    .        .          0         ...  .
	//     .        .          .         ...  .    .        .          0         ...  .
	// 	   0        0          0         ...  I    0        0          0         ...  I  ;
	// 	   B_{n_b}  B_{n_b-1}  B_{n_b-2} ...  B_2 -A_{n_a} -A_{n_a-1} -A_{n_a-2} ... -A_1]
	if (n_x > 0)
	{
		size_type broffs(n_x-rls_n_y); // The bottom row offset
		size_type cboffs0(rls_n_u);
		size_type cboffs1(cboffs0+((rls_n_b > 2) ? (rls_n_b-2)*rls_n_u : 0));
		size_type caoffs0(cboffs1+rls_n_y);
		size_type caoffs1(caoffs0+((rls_n_a > 1) ? (rls_n_a-1)*rls_n_y : 0));

		A().resize(n_x, n_x, false);

		// The upper part of A is set to [0_{k,rls_n_u} I_{k,kb} 0_{k,rls_n_y} I_{k,ka}],
		// where: k=n_x-rls_n_y, kb=(rls_n_b-2)*rls_n_u, ka=(rls_n_a-1)*rls_n_y.
		if (cboffs0 > 0)
		{
			ublas::subrange(A(), 0, broffs, 0, cboffs0) = ublas::zero_matrix<value_type>(broffs,rls_n_u);
		}
		if (cboffs1 > cboffs0)
		{
			ublas::subrange(A(), 0, broffs, cboffs0, cboffs1) = ublas::identity_matrix<value_type>(broffs,cboffs1-cboffs0);
		}
		if (caoffs0 > cboffs1)
		{
			ublas::subrange(A(), 0, broffs, cboffs1, caoffs0) = ublas::zero_matrix<value_type>(broffs,caoffs0-cboffs1);
		}
		if (caoffs1 > caoffs0)
		{
			ublas::subrange(A(), 0, broffs, caoffs0, caoffs1) = ublas::identity_matrix<value_type>(broffs,caoffs1-caoffs0);
		}

		// Fill A with B_2, ..., B_{n_b}
		for (size_type i = 1; i < rls_n_b; ++i)
		{
			// Copy matrix B_i from \hat{\Theta} into A.
			// In A the matrix B_i has to go in (rls_n_b-i)-th position:
			//   A(k:(k+n),((rls_n_b-i-1)*rls_n_u):((rls_n_b-i)*rls_n_u)) <- B_i

			size_type c2((rls_n_b-i)*rls_n_u);
			size_type c1(c2-rls_n_u);

			ublas::subrange(A(), broffs, n_x, c1, c2) = sys_ident_strategy.B(i+1);
		}

		// Fill A with A_1, ..., A_{n_a}
		for (size_type i = 0; i < rls_n_a; ++i)
		{
			// Copy matrix -A_i from \hat{\Theta} into A.
			// In A the matrix A_i has to go in ((rls_n_b-1)*rls_n_u+rls_n_a-i)-th position:
			//   A(k:(k+n),((rls_n_b-1)*rls_n_u+(rls_n_a-i-1)*rls_n_y):((rls_n_b-1)*rls_n_u+(rls_n_a-i)*rls_n_y)) <- -A_i

			size_type c2(cboffs1+(rls_n_a-i)*rls_n_y);
			size_type c1(c2-rls_n_y);

			////ublas::subrange(A(), broffs, n_x, c1, c2) = sys_ident_strategy.A(i+1);
			ublas::subrange(A(), broffs, n_x, c1, c2) = -sys_ident_strategy.A(i+1);
			//ublas::subrange(A(), broffs, n_x, c1, c2) = sys_ident_strategy.A(i+1);
		}
	}
	else
	{
		A().resize(0, 0, false);
	}
//DCS_DEBUG_TRACE("A="<<A);//XXX

	// Create the input matrix B
	// B=[I  ;
	//    0  ;
	//    .	 ;
	//    .	 ;
	//    .	 ;
	//    0  ;
	//    B_1]
	if (n_x > 0)
	{
		size_type broffs(n_x-rls_n_u); // The bottom row offset

		B().resize(n_x, n_u, false);

		// The upper part of B is set to [I_{n_u,n_u} 0_{k,n_u}]
		// where: k=n_x-rls_n_u.
		ublas::subrange(B(), 0, n_u, 0, n_u) = ublas::identity_matrix<value_type>(n_u,n_u);
		ublas::subrange(B(), n_u, broffs, 0, n_u) = ublas::zero_matrix<value_type>(broffs-n_u,n_u);
		// The bottom part of B with B_1
		ublas::subrange(B(), broffs, n_x, 0, n_u) = sys_ident_strategy.B(1);
	}
	else
	{
		B().resize(0, 0, false);
	}
//DCS_DEBUG_TRACE("B="<<B);//XXX

	// Create the output matrix C
	if (n_x > 0)
	{
		size_type rcoffs(n_x-rls_n_y); // The right most column offset

		C().resize(n_y, n_x, false);

		ublas::subrange(C(), 0, n_y, 0, rcoffs) = ublas::zero_matrix<value_type>(n_y,rcoffs);
		ublas::subrange(C(), 0, n_y, rcoffs, n_x) = ublas::scalar_matrix<value_type>(n_y, rls_n_y, 1);
	}
	else
	{
		C().resize(0, 0, false);
	}
//DCS_DEBUG_TRACE("C="<<C);//XXX

	// Create the transmission matrix D
	{
		D().resize(n_y, n_u, false);

		D() = ublas::zero_matrix<value_type>(n_y, n_u);
	}
//DCS_DEBUG_TRACE("D="<<D);//XXX

//DCS_DEBUG_TRACE("END make_ss");//XXX
}

#elif defined(DCS_DES_TESTBED_EXP_LQ_APP_MGR_ALT_SS) && DCS_DES_TESTBED_EXP_LQ_APP_MGR_ALT_SS == 'C'

/// Convert an ARX structure to a state-space model in the canonical controllable form.
template <
//	typename TraitsT,
	typename SysIdentStrategyT,
	typename AMatrixExprT,
	typename BMatrixExprT,
	typename CMatrixExprT,
	typename DMatrixExprT
>
//void make_ss(rls_ff_mimo_proxy<TraitsT> const& sys_ident_strategy,
void make_ss(SysIdentStrategyT const& sys_ident_strategy,
			 ::boost::numeric::ublas::matrix_container<AMatrixExprT>& A,
			 ::boost::numeric::ublas::matrix_container<BMatrixExprT>& B,
			 ::boost::numeric::ublas::matrix_container<CMatrixExprT>& C,
			 ::boost::numeric::ublas::matrix_container<DMatrixExprT>& D)
{
//DCS_DEBUG_TRACE("BEGIN make_ss");//XXX
	namespace ublas = ::boost::numeric::ublas;

	typedef typename ublas::promote_traits<
				typename ublas::promote_traits<
					typename ublas::promote_traits<
						typename ublas::matrix_traits<AMatrixExprT>::value_type,
						typename ublas::matrix_traits<BMatrixExprT>::value_type
					>::promote_type,
					typename ublas::matrix_traits<CMatrixExprT>::value_type
				>::promote_type,
				typename ublas::matrix_traits<DMatrixExprT>::value_type
			>::promote_type value_type;
	typedef ::std::size_t size_type; //FIXME: use type-promotion?

	const size_type rls_n_a(sys_ident_strategy.output_order());
	const size_type rls_n_b(sys_ident_strategy.input_order());
//	const size_type rls_d(sys_ident_strategy.input_delay());
	const size_type rls_n_y(sys_ident_strategy.num_outputs());
	const size_type rls_n_u(sys_ident_strategy.num_inputs());
	const size_type n_x(rls_n_a*rls_n_y);
	const size_type n_u(rls_n_b*rls_n_u);
//	const size_type n(::std::max(n_x,n_u));
	const size_type n_y(1);

	DCS_ASSERT(
			rls_n_y <= 1 && rls_n_u <= 1,
			DCS_EXCEPTION_THROW(
				::std::runtime_error,
				"Actually, only SISO cases are hanlded"
			)
		);
	DCS_ASSERT(
			rls_n_y == rls_n_u,
			DCS_EXCEPTION_THROW(
				::std::runtime_error,
				"Actually, only the same number of channel are treated"
			)
		);

	// Create the state matrix A
	// A=[ 0        I          0         ...  0  ;
	//     0        0          I         ...  0  ;
	//     .        .          .         ...  .
	//     .        .          .         ...  .
	//     .        .          .         ...  .
	// 	   0        0          0         ...  I  ;
	// 	  -A_{n_a} -A_{n_a-1} -A_{n_a-2} ... -A_1]
	if (n_x > 0)
	{
		size_type broffs(n_x-rls_n_y); // The bottom row offset

		A().resize(n_x, n_x, false);

		// The upper part of A is set to [0_{k,rls_n_y} I_{k,k}],
		// where: k=n_x-rls_n_y.
		ublas::subrange(A(), 0, broffs, 0, rls_n_y) = ublas::zero_matrix<value_type>(broffs,rls_n_y);
		ublas::subrange(A(), 0, broffs, rls_n_y, n_x) = ublas::identity_matrix<value_type>(broffs,broffs);

		if (rls_n_a > 0)
		{
			// Fill A with A_1, ..., A_{n_a}
			for (size_type i = 0; i < rls_n_a; ++i)
			{
				// Copy matrix -A_i from \hat{\Theta} into A.
				// In A the matrix A_i has to go in (rls_n_a-i)-th position:
				//   A(k:(k+n),((rls_n_a-i-1)*rls_n_y):((rls_n_a-i)*rls_n_y)) <- -A_i

				size_type c2((rls_n_a-i)*rls_n_y);
				size_type c1(c2-rls_n_y);

				////ublas::subrange(A(), broffs, n_x, c1, c2) = sys_ident_strategy.A(i+1);
				ublas::subrange(A(), broffs, n_x, c1, c2) = -sys_ident_strategy.A(i+1);
				//ublas::subrange(A(), broffs, n_x, c1, c2) = sys_ident_strategy.A(i+1);
			}
		}
		else
		{
			ublas::subrange(A(), broffs, n_x, 0, n_x) = ublas::zero_matrix<value_type>(rls_n_y,n_x);
		}
	}
	else
	{
		A().resize(0, 0, false);
	}
//DCS_DEBUG_TRACE("A="<<A);//XXX

	// Create the input matrix B
	// B=[0;
	//    .;
	//    .;
	//    .;
	//    0;
	//    I]
	if (n_x > 0 && rls_n_b > 0)
	{
		size_type broffs(n_x-rls_n_u); // The bottom row offset

		B().resize(n_x, n_u, false);

		// The upper part of B is set to 0_{k,n_u}
		// where: k=n_x-rls_n_u.
		ublas::subrange(B(), 0, broffs, 0, n_u) = ublas::zero_matrix<value_type>(broffs, n_u);
		// The lower part of B is set to I_{n_u,n_u}
		ublas::subrange(B(), broffs, n_x, 0, n_u) = ublas::identity_matrix<value_type(n_u, n_u);
	}
	else
	{
		B().resize(0, 0, false);
	}
//DCS_DEBUG_TRACE("B="<<B);//XXX

	// Create the output matrix C
	// C=[M_n ... M_0]
	// where M_i=B_i-B_0*A_i
	// NOTE: in our case B_0=0, so M_i=B_i
	if (n_x > 0)
	{
		C().resize(n_y, n_x, false);

		for (size_type i = 0; i < rls_n_b; ++i)
		{
			size_type c2((rls_n_b-i)*rls_n_u);
			size_type c1(c2-rls_n_u);

			ublas::subrange(C(), 0, n_y, c1, c2) = sys_ident_strategy.B(i+1);
		}
	}
	else
	{
		C().resize(0, 0, false);
	}
//DCS_DEBUG_TRACE("C="<<C);//XXX

	// Create the transmission matrix D
	// D=[B0]
	// NOTE: in our case B_0=0, so D=[0]
	{
		D().resize(n_y, n_u, false);

		D() = ublas::zero_matrix<value_type>(n_y, n_u);
	}
//DCS_DEBUG_TRACE("D="<<D);//XXX

//DCS_DEBUG_TRACE("END make_ss");//XXX
}

#else // DCS_DES_TESTBED_EXP_LQ_APP_MGR_ALT_SS

template <
//	typename TraitsT,
	typename SysIdentStrategyT,
	typename AMatrixExprT,
	typename BMatrixExprT,
	typename CMatrixExprT,
	typename DMatrixExprT
>
//void make_ss(rls_ff_mimo_proxy<TraitsT> const& sys_ident_strategy,
void make_ss(SysIdentStrategyT const& sys_ident_strategy,
			 ::boost::numeric::ublas::matrix_container<AMatrixExprT>& A,
			 ::boost::numeric::ublas::matrix_container<BMatrixExprT>& B,
			 ::boost::numeric::ublas::matrix_container<CMatrixExprT>& C,
			 ::boost::numeric::ublas::matrix_container<DMatrixExprT>& D)
{
//DCS_DEBUG_TRACE("BEGIN make_ss");//XXX
	namespace ublas = ::boost::numeric::ublas;

	typedef typename ublas::promote_traits<
				typename ublas::promote_traits<
					typename ublas::promote_traits<
						typename ublas::matrix_traits<AMatrixExprT>::value_type,
						typename ublas::matrix_traits<BMatrixExprT>::value_type
					>::promote_type,
					typename ublas::matrix_traits<CMatrixExprT>::value_type
				>::promote_type,
				typename ublas::matrix_traits<DMatrixExprT>::value_type
			>::promote_type value_type;
	typedef ::std::size_t size_type; //FIXME: use type-promotion?

	const size_type rls_n_a(sys_ident_strategy.output_order());
	const size_type rls_n_b(sys_ident_strategy.input_order());
//	const size_type rls_d(sys_ident_strategy.input_delay());
	const size_type rls_n_y(sys_ident_strategy.num_outputs());
	const size_type rls_n_u(sys_ident_strategy.num_inputs());
	const size_type n_x(rls_n_a*rls_n_y);
	const size_type n_u(rls_n_b*rls_n_u);
//	const size_type n(::std::max(n_x,n_u));
	const size_type n_y(1);

	// Create the state matrix A
	// A=[ 0        I          0         ...  0  ;
	//     0        0          I         ...  0  ;
	//     .        .          .         ...  .
	//     .        .          .         ...  .
	//     .        .          .         ...  .
	// 	   0        0          0         ...  I  ;
	// 	  -A_{n_a} -A_{n_a-1} -A_{n_a-2} ... -A_1]
	if (n_x > 0)
	{
		size_type broffs(n_x-rls_n_y); // The bottom row offset

		A().resize(n_x, n_x, false);

		// The upper part of A is set to [0_{k,rls_n_y} I_{k,k}],
		// where: k=n_x-rls_n_y.
		ublas::subrange(A(), 0, broffs, 0, rls_n_y) = ublas::zero_matrix<value_type>(broffs,rls_n_y);
		ublas::subrange(A(), 0, broffs, rls_n_y, n_x) = ublas::identity_matrix<value_type>(broffs,broffs);

		// Fill A with A_1, ..., A_{n_a}
		for (size_type i = 0; i < rls_n_a; ++i)
		{
			// Copy matrix -A_i from \hat{\Theta} into A.
			// In A the matrix A_i has to go in (rls_n_a-i)-th position:
			//   A(k:(k+n),((rls_n_a-i-1)*rls_n_y):((rls_n_a-i)*rls_n_y)) <- -A_i

			size_type c2((rls_n_a-i)*rls_n_y);
			size_type c1(c2-rls_n_y);

			////ublas::subrange(A(), broffs, n_x, c1, c2) = sys_ident_strategy.A(i+1);
			ublas::subrange(A(), broffs, n_x, c1, c2) = -sys_ident_strategy.A(i+1);
			//ublas::subrange(A(), broffs, n_x, c1, c2) = sys_ident_strategy.A(i+1);
		}
	}
	else
	{
		A().resize(0, 0, false);
	}
//DCS_DEBUG_TRACE("A="<<A);//XXX

	// Create the input matrix B
	// B=[0 ... 0;
	//    .	... .
	//    .	... .
	//    .	... .
	//    0 ... 0;
	//    B_{n_b} ... B_1]
	if (n_x > 0)
	{
		size_type broffs(n_x-rls_n_u); // The bottom row offset

		B().resize(n_x, n_u, false);

		// The upper part of B is set to 0_{k,n_u}
		// where: k=n_x-rls_n_u.
		ublas::subrange(B(), 0, broffs, 0, n_u) = ublas::zero_matrix<value_type>(broffs,n_u);

		// Fill B with B_1, ..., B_{n_b}
		for (size_type i = 0; i < rls_n_b; ++i)
		{
			// Copy matrix B_i from \hat{\Theta} into B.
			// In \hat{\Theta} the matrix B_i stays at:
			//   B_i <- (\hat{\Theta}(((n_a*n_y)+i):n_b:n_u,:))^T
			// but in B the matrix B_i has to go in (n_b-i)-th position:
			//   B(k:(k+n_x),((n_b-i-1)*n_u):((n_a-i)*n_u)) <- B_i

			size_type c2((rls_n_b-i)*rls_n_u);
			size_type c1(c2-rls_n_u);

			ublas::subrange(B(), broffs, n_x, c1, c2) = sys_ident_strategy.B(i+1);
		}
	}
	else
	{
		B().resize(0, 0, false);
	}
//DCS_DEBUG_TRACE("B="<<B);//XXX

	// Create the output matrix C
	if (n_x > 0)
	{
		size_type rcoffs(n_x-rls_n_y); // The right most column offset

		C().resize(n_y, n_x, false);

		ublas::subrange(C(), 0, n_y, 0, rcoffs) = ublas::zero_matrix<value_type>(n_y,rcoffs);
		ublas::subrange(C(), 0, n_y, rcoffs, n_x) = ublas::scalar_matrix<value_type>(n_y, rls_n_y, 1);
	}
	else
	{
		C().resize(0, 0, false);
	}
//DCS_DEBUG_TRACE("C="<<C);//XXX

	// Create the transmission matrix D
	{
		D().resize(n_y, n_u, false);

		D() = ublas::zero_matrix<value_type>(n_y, n_u);
	}
//DCS_DEBUG_TRACE("D="<<D);//XXX

//DCS_DEBUG_TRACE("END make_ss");//XXX
}

#endif // DCS_DES_TESTBED_EXP_LQ_APP_MGR_ALT_SS

}} // Namespace detail::<unnamed>


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
	private: static const real_type default_ewma_smoothing_factor;


	public: lq_application_manager()
	: ts_(default_sampling_time),
	  tc_(default_control_time),
	  nx_(0),
	  nu_(0),
	  ny_(0),
	  x_offset_(0),
	  u_offset_(0),
	  ctl_count_(0),
	  ctl_skip_count_(0),
	  ctl_fail_count_(0),
	  sysid_fail_count_(0),
	  ewma_sf_(default_ewma_smoothing_factor)
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

	protected: numeric_vector_type optimal_control(numeric_vector_type const& x,
												   numeric_vector_type const& u,
												   numeric_vector_type const& y,
												   numeric_matrix_type const& A,
												   numeric_matrix_type const& B,
												   numeric_matrix_type const& C,
												   numeric_matrix_type const& D)
	{
		return do_optimal_control(x, u, y, A, B, C, D);
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

		//[FIXME]
		DCS_ASSERT(tgt_map_.size() == 1,
				   DCS_EXCEPTION_THROW(::std::logic_error,
									   "Currently, only one application performace category is handled"));
		DCS_ASSERT(tgt_map_.size() == 1,
				   DCS_EXCEPTION_THROW(::std::logic_error,
									   "Currently, only one application performace category is handled"));
		//[/FIXME]

		p_sysid_alg_->init();
#if defined(DCS_TESTBED_EXP_LQ_APP_MGR_USE_ALT_SS) && DCS_TESTBED_LQ_APP_MGR_USE_ALT_SS == 'X'
		nx_ = p_sysid_alg_->num_outputs()*p_sysid_alg_->output_order()+p_sysid_alg_->num_inputs()*(p_sysid_alg_->input_order()-1);
		nu_ = p_sysid_alg_->num_inputs();
		ny_ = p_sysid_alg_->num_outputs();
		x_offset_ = (nx_ > 0) ? (nx_-p_sysid_alg_->num_outputs()) : 0;
		u_offset_ = 0;
#elif defined(DCS_TESTBED_EXP_LQ_APP_MGR_USE_ALT_SS) && DCS_TESTBED_LQ_APP_MGR_USE_ALT_SS == 'C'
		nx_ = p_sysid_alg_->num_outputs()*p_sysid_alg_->output_order();
		nu_ = p_sysid_alg_->num_inputs();
		ny_ = p_sysid_alg_->num_outputs();
		x_offset_ = (nx_ > 0) ? (nx_-p_sysid_alg_->num_outputs()) : 0;
		u_offset_ = 0;
#else // DCS_TESTBED_EXP_LQ_APP_MGR_ALT_SS
		nx_ = p_sysid_alg_->num_outputs()*p_sysid_alg_->output_order();
		nu_ = p_sysid_alg_->num_inputs()*p_sysid_alg_->input_order();
		ny_ = p_sysid_alg_->num_outputs();
		x_offset_ = (nx_ > 0) ? (nx_-p_sysid_alg_->num_outputs()) : 0;
		u_offset_ = (nu_ > 0) ? (nu_-p_sysid_alg_->num_inputs()) : 0;
#endif // DCS_TESTBED_EXP_LQ_APP_MGR_ALT_SS
		x_ = numeric_vector_type(nx_, ::std::numeric_limits<real_type>::quiet_NaN());
		u_ = numeric_vector_type(nu_, ::std::numeric_limits<real_type>::quiet_NaN());
		y_ = numeric_vector_type(ny_, ::std::numeric_limits<real_type>::quiet_NaN());
		//yr_ = ::boost::numeric::ublas::scalar_vector<real_type>(ny_, tgt_map_.at(response_time_application_performance));
		yr_ = numeric_vector_type(ny_, ::std::numeric_limits<real_type>::quiet_NaN());
		typedef typename target_map::const_iterator target_iterator;
		target_iterator tgt_end_it = tgt_map_.end();
		for (target_iterator tgt_it = tgt_map_.begin();
			 tgt_it != tgt_end_it;
			 ++tgt_it)
		{
			application_performance_category cat(tgt_it->first);

			yr_ = numeric_vector_type(ny_, tgt_it->second);
			out_sens_map_[cat] = p_app_->sensor(cat);
		}
		ewma_s_ = numeric_vector_type(p_sysid_alg_->num_inputs(), ::std::numeric_limits<real_type>::quiet_NaN());
		ewma_p_ = numeric_vector_type(p_sysid_alg_->num_outputs(), ::std::numeric_limits<real_type>::quiet_NaN());
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

			// Update inputs/outputs
			if (nx_ > 0)
			{
				//FIXME: fix the assignment below
				for (::std::size_t v = 0; v < np; ++v)
				{
					x_(x_offset_+v) = p(v) = ewma_p_(v);
				}
			}
			if (nu_ > 0)
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

					u_(u_offset_+v) = s(v)
									= p_vm->cpu_share()/**p_vm->max_num_vcpus()*/;

					++v;
				}
			}
			if (ny_ > 0)
			{
				y_ = ublas::element_div(p, yr_)-ublas::scalar_vector<real_type>(ny_, 1);
			}
DCS_DEBUG_TRACE("New x=" << x_);
DCS_DEBUG_TRACE("New u=" << u_);
DCS_DEBUG_TRACE("New y=" << y_);

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
				// Create the state-space representation of the system model:
				// x(k+1) = Ax(k)+Bu(k)
				// y(k)   = Cx(k)+Du(k)

				numeric_matrix_type A;
				numeric_matrix_type B;
				numeric_matrix_type C;
				numeric_matrix_type D;

				detail::make_ss(*p_sysid_alg_, A, B, C, D);

				numeric_vector_type opt_u;
				try
				{
					opt_u = this->optimal_control(x_, u_, y_, A, B, C, D);
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

						real_type new_share = opt_u(u_offset_+v);

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
DCS_DEBUG_TRACE("Optimal control applied");
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

	private: virtual numeric_vector_type do_optimal_control(numeric_vector_type const& x,
															numeric_vector_type const& u,
															numeric_vector_type const& y,
															numeric_matrix_type const& A,
															numeric_matrix_type const& B,
															numeric_matrix_type const& C,
															numeric_matrix_type const& D) = 0;


	private: uint_type ts_; ///< Sampling time (in ms)
	private: uint_type tc_; ///< Control time (in ms)
	private: app_pointer p_app_; ///< Pointer to the managed application
	private: sensor_map out_sens_map_; ///< Sensor map for the application outputs
	private: sysid_strategy_pointer p_sysid_alg_;
	private: observation_map out_obs_map_; ///< Application output observations collected in the last control interval
	private: ::std::size_t nx_; ///< Number of states
	private: ::std::size_t nu_; ///< Number of inputs
	private: ::std::size_t ny_; ///< Number of outputs
	private: ::std::size_t x_offset_; ///< Offset used to rotate the state vector to make space for new observations
	private: ::std::size_t u_offset_; ///< Offset used to rotate the input vector to make space for new observations
	private: numeric_vector_type x_; ///< The state vector for the state-space representation
	private: numeric_vector_type u_; ///< The input vector for the state-space representation
	private: numeric_vector_type y_; ///< The output vector for the state-space representation
	private: numeric_vector_type yr_; ///< The output vector to be tracked 
	private: ::std::size_t ctl_count_; ///< Number of times control function has been invoked
	private: ::std::size_t ctl_skip_count_; ///< Number of times control has been skipped
	private: ::std::size_t ctl_fail_count_; ///< Number of times control has failed
	private: ::std::size_t sysid_fail_count_; ///< Number of times system identification has failed
	private: real_type ewma_sf_; ///< EWMA smoothing factor
	private: numeric_vector_type ewma_s_; ///< Current EWMA values for inputs
	private: numeric_vector_type ewma_p_; ///< Current EWMA values for outputs
	private: target_map tgt_map_; ///< Mapping between application performance categories and target values
}; // lq_application_manager

template <typename T>
const typename lq_application_manager<T>::uint_type lq_application_manager<T>::default_sampling_time = 1;

template <typename T>
const typename lq_application_manager<T>::uint_type lq_application_manager<T>::default_control_time = 5;

template <typename T>
const typename lq_application_manager<T>::real_type lq_application_manager<T>::default_min_share = 0.20;

template <typename T>
const typename lq_application_manager<T>::real_type lq_application_manager<T>::default_max_share = 1.00;

template <typename T>
const typename lq_application_manager<T>::real_type lq_application_manager<T>::default_ewma_smoothing_factor = 0.70;


template <typename TraitsT>
class lqry_application_manager: public lq_application_manager<TraitsT>
{
	private: typedef lq_application_manager<TraitsT> base_type;
	public: typedef typename base_type::traits_type traits_type;
	private: typedef typename traits_type::real_type real_type;
	private: typedef typename traits_type::uint_type uint_type;
	private: typedef ::dcs::control::dlqry_controller<real_type> controller_type;
	private: typedef typename base_type::numeric_vector_type numeric_vector_type;
	private: typedef typename base_type::numeric_matrix_type numeric_matrix_type;


	public: lqry_application_manager()
	{
	}

	private: numeric_vector_type do_optimal_control(numeric_vector_type const& x,
													numeric_vector_type const& u,
													numeric_vector_type const& y,
													numeric_matrix_type const& A,
													numeric_matrix_type const& B,
													numeric_matrix_type const& C,
													numeric_matrix_type const& D)
	{
		DCS_MACRO_SUPPRESS_UNUSED_VARIABLE_WARNING( u );
		DCS_MACRO_SUPPRESS_UNUSED_VARIABLE_WARNING( y );

		namespace ublas = ::boost::numeric::ublas;
		namespace ublasx = ::boost::numeric::ublasx;

		// Check: if (A,B) is stabilizable, then the assoicated DARE has a
		//        positive semidefinite solution.
		//        (sufficient and necessary condition)
		if (!::dcs::control::is_stabilizable(A, B, true))
		{
			::std::ostringstream oss;
			oss << "System (A,B) is not stabilizable (the associated DARE cannot have a positive semidefinite solution) [with A=" << A << " and B=" << B << "]";
			::dcs::log_warn(DCS_LOGGING_AT, oss.str());

			DCS_EXCEPTION_THROW(::std::runtime_error, oss.str());
		}
		// Check: if (A,B) stabilizable and (C'QC,A) detectable, then the
		//        associated DARE has a unique and stabilizing solution such
		//        that the closed-loop system:
		//          x(k+1) = Ax(k) + Bu(k) = (A + BK)x(k)
		//        is stable (K is the LQRY-optimal state feedback gain).
		//        (sufficient and necessary condition)
		numeric_matrix_type QQ(ublas::prod(ctlr_.Q(), C));
		QQ = ublas::prod(ublas::trans(C), QQ);
		if (!::dcs::control::is_detectable(A, QQ, true))
		{
			::std::ostringstream oss;
			oss << "System (C'QC,A) is not detectable (closed-loop system will not be stable) [with " << A << ", Q=" << ctlr_.Q() << " and C=" << C << "]";
			::dcs::log_warn(DCS_LOGGING_AT, oss.str());

			DCS_EXCEPTION_THROW(::std::runtime_error, oss.str());
		}

		uint_type nx(ublas::num_columns(A));
		uint_type nu(ublas::num_columns(B));
		uint_type ny(ublas::num_rows(C));
		numeric_vector_type r(ny, 1);//FIXME

		numeric_vector_type opt_u;

		ctlr_.solve(A, B, C, D);
		opt_u = ublas::real(ctlr_.control(x));

		uint_type ncp(nx+nu);
		uint_type nrp(nx+ny);

		numeric_matrix_type P(nrp, ncp, 0);
		ublas::subrange(P, 0, nx, 0, nx) = ublas::identity_matrix<real_type>(nx, nx) - A;
		ublas::subrange(P, 0, nx, nx, ncp) = B;
		ublas::subrange(P, nx, nrp, 0, nx) = -C;
		ublas::subrange(P, nx, nrp, nx, ncp) = D;
		numeric_matrix_type Pt(ublas::trans(P));
		numeric_matrix_type PP(ublas::prod(P, Pt));
		bool inv = ublasx::inv_inplace(PP);
		if (inv)
		{
			PP = ublas::prod(Pt, PP);
			numeric_vector_type yd(nrp,0);
			ublas::subrange(yd, nx, nrp) = r;
			numeric_vector_type xdud(ublas::prod(PP, yd));
			DCS_DEBUG_TRACE("COMPENSATION: P=" << P << " ==> (xd,ud)=" << xdud << ", opt_u=" << opt_u);//XXX
			opt_u = opt_u + ublas::subrange(xdud, nx, ncp);
			DCS_DEBUG_TRACE("COMPENSATION: P=" << P << " ==> (xd,ud)=" << xdud << ", NEW opt_u=" << opt_u);//XXX
		}
		else
		{
			DCS_EXCEPTION_THROW( ::std::runtime_error, "Cannot compute equilibrium control input: Rosenbrock's system matrix is not invertible" );
		}

		return opt_u;
	}


	private: controller_type ctlr_;
};

}} // Namespace dcs::testbed

#endif // DCS_TESTBED_LQ_SYSTEM_MANAGER_HPP
