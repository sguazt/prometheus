/**
 * \file dcs/testbed/data_estimators.hpp
 *
 * \brief Classes to estimate data.
 *
 * \author Marco Guazzone (marco.guazzone@gmail.com)
 *
 * <hr/>
 *
 * Copyright 2013 Marco Guazzone (marco.guazzone@gmail.com)
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

#ifndef DCS_TESTBED_DATA_ESTIMATORS_HPP
#define DCS_TESTBED_DATA_ESTIMATORS_HPP


#include <algorithm>
#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics/mean.hpp>
#include <boost/accumulators/statistics/p_square_quantile.hpp>
#include <boost/accumulators/statistics/stats.hpp>
#include <boost/circular_buffer.hpp>
//#include <boost/numeric/ublas/vector.hpp>
//#include <boost/numeric/ublasx/operation/log10.hpp>
//#include <boost/numeric/ublasx/operation/logspace.hpp>
#include <boost/random.hpp>
#include <cmath>
#include <cstddef>
#include <dcs/assert.hpp>
#include <dcs/debug.hpp>
#include <dcs/exception.hpp>
#include <dcs/macro.hpp>
#include <dcs/math/function/clamp.hpp>
#include <dcs/math/function/sign.hpp>
#include <dcs/testbed/detail/jni.hpp>
#include <dcs/testbed/detail/quantile.hpp>
#include <dcs/testbed/detail/variance.hpp>
#include <limits>
#include <list>
#include "../../../thirdparty/t-digest/tdigestx.hxx"
#include <stdexcept>
#include <vector>


namespace dcs { namespace testbed {

/**
 * \brief Base class for data estimators.
 *
 * \tparam ValueT Type for sample values.
 *
 * \author Marco Guazzone (marco.guazzone@gmail.com)
 */
template <typename ValueT>
class base_estimator
{
	public: typedef ValueT value_type;
	protected: typedef std::vector<value_type> data_container;


	public: base_estimator()
	: n_(0)
	{
	}

	public: void collect(value_type val)
	{
		this->do_collect(std::vector<value_type>(1, val));

		++n_;
	}

	public: template <typename IterT>
			void collect(IterT first, IterT last)
	{
		const std::vector<value_type> data(first, last);

		this->do_collect(data);

		n_ += data.size();
	}

	public: value_type estimate() const
	{
		return this->do_estimate();
	}

	public: void reset()
	{
		this->do_reset();

//		// post: <# collected observations> == 0
//		DCS_ASSERT(this->count() == 0,
//				   DCS_EXCEPTION_THROW(std::logic_error,
//									   "Number of collected observations greater than zero after reset"));

		n_ = 0;
	}

	// Return the number of collected observations after the last reset
	public: std::size_t count() const
	{
//		return this->do_count();
		return n_;
	}

	private: virtual void do_collect(data_container const& data) = 0; 
	private: virtual value_type do_estimate() const = 0;
	private: virtual void do_reset() = 0;
//	private: virtual std::size_t do_count() const = 0;


	private: std::size_t n_;
}; // base_estimator


/**
 * \brief The LORA-based incremental quantile estimation method from
 *  (Bakshi,2006) for non-stationary stochastic processes.
 *
 * \tparam ValueT Type for sample values.
 *
 * The authors propose different methods according to the characteristics of the
 * underlying stochastic process.
 *
 * We don't consider the case of known probability distribution (see Sec. 2 of
 * the paper) since this is unlikely to happen in the real case.
 *
 * In case of stationary stochastic processes (i.e., same unknown distribution),
 * the proposed LORA method is the following (see Sec. 3 of the paper):
 * \f{align*}
 *  \hat{T}_i &= \hat{T}_{i-1} + \hat{s}_i q \log(\frac{\hat{q}_i}{\hat{p}_i} \frac{p}{q}) \frac{1}{\sqrt{i}},
 *  \hat{p}_i &= \frac{M-n_{T_i}+0.5}{M+1},
 *  \hat{q}_i &= 1-\hat{p}_i,
 *  \hat{s}_i &= \omega \hat{s}_{i-i} + (1-\omega) s_i.
 * \f}
 * where:
 * - \f$n_{T_i}\f$: number of observations \f$x_{ij}\f$ in the \f$i\f$-th sample \f$x_i = (x_{i1},x_{i2},\ldots,x_{iM})\f$ such that \f$x_{ij} > \hat{T}_{i-1}\f$
 * - \f$\beta,\omega\f$: smoothing constants
 * .
 *
 * In case of non-stationary stochastic processes (i.e., different unknown
 * distributions), the proposed LORA method is the following (see Sec. 4 of the
 * paper):
 * \f{align*}
 *  \hat{T}_i &= \hat{T}_{i-1} + \hat{s}_i q \log(\frac{\hat{q}_i}{\hat{p}_i} \frac{p}{q}) \mathrm{gain} (1-\beta),
 *  \hat{p}_i &= \frac{M-n_{T_i}+0.5}{M+1},
 *  \hat{q}_i &= 1-\hat{p}_i,
 *  \hat{s}_i &= \omega \hat{s}_{i-i} + (1-\omega) s_i.
 * \f}
 * where:
 * - \f$n_{T_i}\f$: number of observations \f$x_{ij}\f$ in the \f$i\f$-th sample \f$x_i = (x_{i1},x_{i2},\ldots,x_{iM})\f$ such that \f$x_{ij} > \hat{T}_{i-1}\f$
 * - \f$\beta,\omega\f$: smoothing constants
 * .
 *
 * The initial estimate of the quantile, \f$\hat{T}_0\f$, is set to the sum of the true quantile value, \f$T_p\f$, and normally distributed variable with zero mean and unit variance, i.e.,
 * \f{align*}
 *   \hat{T}_0 &= T_p + N(0,1),
 *   s_0 &= \mathrm{stdev}(x_{01},x_{02},\ldots,x_{0M}).
 * \f}
 *
 *
 * References:
 * -# Yury Bakshi and David A. Hoeflin
 *    "Quantile Estimation: A Minimalistic Approach",
 *    In Proc. of the 2006 Winter Simulation Conference (WSC), 2006.
 * .
 *
 * Note: in this method, the next quantile estimation depends by the standard devation of the collected sample.
 * This can be an issue when the sample is one, that is when the quantile is estimated every time a new observation is seen.
 * Indeed, in this case, the standard deviation is zero.
 * For such reason, we remember past seen samples and store them in a sample window of size M.
 *
 * \author Marco Guazzone (marco.guazzone@gmail.com)
 */
template <typename ValueT>
class bakshi2006_lora_quantile_estimator: public base_estimator<ValueT>
{
	private: typedef base_estimator<ValueT> base_type;
	public: typedef typename base_type::value_type value_type;
	private: typedef typename base_type::data_container data_container;


	public: explicit bakshi2006_lora_quantile_estimator(value_type prob,
														bool stationary = true,
														value_type beta = 0.95,
														value_type omega = 0.95,
														value_type gain = 10,
														std::size_t M = 40)
	: prob_(prob),
	  stationary_(stationary),
	  beta_(beta),
	  omega_(omega),
	  gain_(gain),
	  sample_buf_(M),
	  data_(),
	  tn_(std::numeric_limits<value_type>::quiet_NaN()),
	  sn_(std::numeric_limits<value_type>::quiet_NaN()),
	  n_(0),
	  init_(true)
	{
	}

	private: void do_collect(data_container const& data)
	{
		typedef typename data_container::const_iterator data_iterator;

		const data_iterator data_end_it = data.end();
		for (data_iterator data_it = data.begin(); data_it != data_end_it; ++data_it)
		{
			const value_type val = *data_it;

			data_.push_back(val);
			sample_buf_.push_back(val);
		}
	}

	private: value_type do_estimate() const
	{
		const std::size_t m = data_.size();

		if (m > 0)
		{
			if (init_)
			{
				//NOTE: initialization is done only when we have seen at
				//		least M observations (i.e., the sample buffer buf_len_
				//		is full).
				//		Before this happens, we compute try to compute a
				//		reasonable value for \hat{T}_0 with the samples that we
				//		have collected so far.

				if (sample_buf_.full())
				{
					init_ = false;
				}

				//NOTE: in the original work by (Bakshi et al., 2006), the
				//		initial value T_0 is computed as:
				//        T_0 = T_p + N(0,1)
				//      where N(0,1) represents a white noise error term,
				//      that is a randomly generated value according to a
				//      normal distribution with mean 0 and variance 1.
				//      This works only in the case when the magnitude of
				//      T_p is comparable with the one of N(0,1).
				//      So to make it work in the general case, we change
				//      the initialization step into:
				//        T_0 = T_p * (1 + N(0,1))
				//      That is, the error term now is considered as a
				//      percentage of T_p.

				boost::random::mt19937 rng; //FIXME: actually use the default seed. Should we set a different seed?
				boost::random::normal_distribution<value_type> white_noise_rvg;

				if (m > sample_buf_.size())
				{
					tn_ = detail::quantile(data_.begin(), data_.end(), prob_)*(1+white_noise_rvg(rng)); // See \hat{T}_0
					sn_ = detail::stdev<value_type>(data_.begin(), data_.end()); // See s_0
				}
				else
				{
					tn_ = detail::quantile(sample_buf_.begin(), sample_buf_.end(), prob_)*(1+white_noise_rvg(rng)); // See \hat{T}_0
					sn_ = detail::stdev<value_type>(sample_buf_.begin(), sample_buf_.end()); // See s_0
				}
			}
			else
			{
				std::size_t nt = 0;
				if (m > sample_buf_.size())
				{
					for (std::size_t i = 0; i < m; ++i)
					{
						if (data_[i] > tn_)
						{
							++nt;
						}
					}
				}
				else
				{
					for (std::size_t i = 0,
									 ni = sample_buf_.size();
						 i < ni;
						 ++i)
					{
						if (sample_buf_[i] > tn_)
						{
							++nt;
						}
					}
				}

				const std::size_t M = (m > sample_buf_.size() ? m : sample_buf_.size());
				const value_type q = 1-prob_;
				const value_type pi = (M-nt+0.5)/(M+1); // See \hat{p}_i
				const value_type qi = 1-pi; // See \hat{q}_i
				const value_type si = (m > sample_buf_.size()) ? detail::stdev<ValueT>(data_.begin(), data_.end())
															   : detail::stdev<ValueT>(sample_buf_.begin(), sample_buf_.end()); // See s_i
				const value_type gain = stationary_ ? 1.0/sqrt(n_)
													: gain_*(1-beta_);

				sn_ = omega_*sn_ + (1-omega_)*si; // See \hat{s}_i
				tn_ = tn_ + sn_*q*std::log(qi/pi * prob_/q)*gain; // See \hat{T}_i

				++n_;
			}
			data_.clear();
		}

		return tn_;
	}

	private: void do_reset()
	{
		init_ = true;
		sample_buf_.clear();
		data_.clear();
		tn_ = sn_
			= std::numeric_limits<value_type>::quiet_NaN();
		n_ = 0;
	}

//	private: std::size_t do_count() const
//	{
//		return data_.size();
//	}


	private: value_type prob_; //< The probability of the quantile we want to estimate
	private: bool stationary_; //< Tells if the underlying stochastic process must be considered stationary or not.
	private: value_type beta_; //< The \beta smoothing constant.
	private: value_type omega_; //< The \omega smoothing constant.
	private: value_type gain_; //< The gain constant factor for the non-stationary case.
	private: boost::circular_buffer<value_type> sample_buf_; //< Sample window used in case there are too few samples to update quantile estimate
	private: mutable std::vector<value_type> data_; //< Last collected sample values.
	private: mutable value_type tn_; //< The \hat{T}_i incremental quantile estimate.
	private: mutable value_type sn_; //< The \hat{s}_i incremental standard deviation estimate.
	private: mutable std::size_t n_; //< Keeps track of the number of updates of \hat{T}_i
	private: mutable bool init_; //< Tells if the estimate has been already initialized or not.
}; // bakshi2006_lora_quantile_estimator


/**
 * \brief The incremental quantile estimation method from (Chambers,2006).
 *
 * \tparam ValueT Type for sample values.
 *
 * References:
 * - John M. Chambers, David A. James, Diane Lambert and Scott Vander Wiel
 *   "Monitoring Networked Applications With Incremental Quantile Estimation",
 *   Statistical Science, 21(4):463-475, 2006
 * .
 *
 * \author Marco Guazzone (marco.guazzone@gmail)
 */
template <typename ValueT>
class chambers2006_incremental_quantile_estimator: public base_estimator<ValueT>
{
	private: typedef base_estimator<ValueT> base_type;
	public: typedef typename base_type::value_type value_type;
	private: typedef typename base_type::data_container data_container;


	public: explicit chambers2006_incremental_quantile_estimator(value_type prob, std::size_t nq = 251)
	: prob_(prob),
	  nq_(nq),
	  //nbuf_(1000),
	  q0_(+std::numeric_limits<value_type>::infinity()),
	  qm_(-std::numeric_limits<value_type>::infinity()),
	  quant_(std::numeric_limits<value_type>::quiet_NaN()),
	  nt_(0),
	  data_(),
	  qile_(nq, 0),
	  pval_(nq, 0),
	  init_(true)
	{
		DCS_ASSERT(nq > 0,
				   DCS_EXCEPTION_THROW(std::invalid_argument,
									   "Number of probabilities must be greater than zero"));
	}

	private: static value_type lin_interp(value_type x, value_type x0, value_type x1, value_type p0, value_type p1)
	{
		return p0+(p1-p0)*(x-x0)/(x1-x0);
	}

	private: static value_type trim_prob(value_type p, std::size_t t)
	{
		const value_type p_lo = 0.5/t;
		const value_type p_hi = 1-0.5/t;

		return std::min(std::max(p_lo, p), p_hi);
	}

	private: void update() const
	{
		const std::size_t nq = qile_.size();
		const std::size_t nd = data_.size();

        std::size_t jd = 0;
		std::size_t jq = 1;
		value_type qold;
		value_type qnew;
        std::vector<value_type> newqile(nq);

		std::sort(data_.begin(), data_.end());

		q0_ = data_[0];
		qm_ = data_[nd-1];

        qold = qnew
			 = qile_[0]
			 = newqile[0]
			 = q0_;
        qile_[nq-1] = newqile[nq-1]
				   = qm_;
        pval_[0] = std::min(0.5/(nt_+nd), 0.5*pval_[1]);
        pval_[nq-1] = std::max(1.-0.5/(nt_+nd), 0.5*(1.+pval_[nq-2]));
        for (std::size_t iq = 1; iq < (nq-1); ++iq)
		{
        	value_type target = (nt_+nd)*pval_[iq];
			value_type told=0;
			value_type tnew=0;

            if (tnew < target)
			{
				while (true)
				{
					// We locate a succession of abscissa-ordinate pairs (qnew,tnew)
					// that are the discontinuities of value or slope in Figure 8.5.1(c)
					// of Numerical Recipies 3/E, breaking to perform an interpolation
					// as we cross each target
					if (jq < nq && (jd >= nd || qile_[jq] < data_[jd]))
					{
						// Found slope discontinuity from old CDF
						qnew = qile_[jq];
						tnew = jd + nt_*pval_[jq++];
						if (tnew >= target)
						{
							break;
						}
					}
					else if (jd < nd)
					{
						// Found value discontinuity from batch data CDF
						qnew = data_[jd];
						tnew = told;
						if (qile_[jq] > qile_[jq-1])
						{
							tnew += nt_*(pval_[jq]-pval_[jq-1])*(qnew-qold)/(qile_[jq]-qile_[jq-1]);
						}
						++jd;
						if (tnew >= target)
						{
							break;
						}
						told = tnew++;
						qold = qnew;
						if (tnew >= target)
						{
							break;
						}
					}
					else
					{
						break;
					}
					told = tnew;
					qold = qnew;
				}
			}
			// Perform the new interpolation
            if (tnew == told)
			{
				newqile[iq] = 0.5*(qold+qnew);
			}
            else
			{
				newqile[iq] = qold + (qnew-qold)*(target-told)/(tnew-told);
			}
            told = tnew;
            qold = qnew;
        }
        qile_ = newqile;
	}

	private: void do_collect(data_container const& data)
	{
		typedef typename data_container::const_iterator data_iterator;

		const data_iterator data_end_it = data.end();
		for (data_iterator data_it = data.begin(); data_it != data_end_it; ++data_it)
		{
			const value_type val = *data_it;

			data_.push_back(val);

			if (val < q0_)
			{
				q0_ = val;
			}
			if (val > qm_)
			{
				qm_ = val;
			}
/*
			if (data_.size() == nbuf_)
			{
				this->update();
			}
*/
		}
	}

	private: value_type do_estimate() const
	{
		const std::size_t nd = data_.size();

		if (nd > 0)
		{
			if (init_)
			{
				/* This is the initialization used by Numerical Recipies 3/E */
				for (std::size_t j = 85; j <= 165; ++j)
				{
					pval_[j] = (j-75)/100.0;
				}
				for (std::size_t j = 85; j > 0; --j)
				{
					pval_[j-1] = 0.87191909*pval_[j];
					pval_[250-j+1] = 1.-pval_[j-1];
				}

/* This initialization uses equallly spaced probabilities in log-scale
				namespace ublas = boost::numeric::ublas;
				namespace ublasx = boost::numeric::ublasx;

				const std::size_t nq = pval_.size();
				if (nq > 2)
				{
					const value_type p = (prob_ <= 0.5) ? prob_ : (1-prob_);
					const value_type p0 = std::min(p, 0.0025);
					const value_type p1 = std::max(1.0-p0, 0.9975);

					pval_[0] = 0;
					pval_[nq-1] = 1;
					ublas::vector<value_type> probs = ublasx::log10(ublasx::logspace(p0, p1, nq-2));
					for (std::size_t i = 1; i < nq-1; ++i)
					{
						pval_[i] = probs[i-1];
					}
				}
*/

				init_ = false;
			}

			value_type q = 0;

			this->update();

			nt_ += data_.size();
			data_.clear();

			// Performs a binary search to locate the nearest probability value
			// with respect to the wanted one.

			const std::size_t nq = qile_.size();
			std::size_t jl = 0;
			std::size_t jh = nq-1;
			std::size_t j;
			while ((jh-jl)>1)
			{
				// Get the mid position
				j = (jh+jl) >> 1;
				if (prob_ > pval_[j])
				{
					jl = j;
				}
				else
				{
					jh = j;
				}
			}
			j = jl;

			q = lin_interp(prob_, pval_[j], pval_[j+1], qile_[j], qile_[j+1]);

			quant_ = dcs::math::clamp(q, qile_[0], qile_[nq-1]);
		}

		return quant_;
	}

	private: void do_reset()
	{
		init_ = true;
		data_.clear();
		
		quant_ = std::numeric_limits<value_type>::quiet_NaN();
		q0_ = +std::numeric_limits<value_type>::infinity();
		qm_ = -std::numeric_limits<value_type>::infinity();
		qile_.resize(nq_, 0);
		pval_.resize(nq_, 0);
		nt_ = 0;
	}


	private: value_type prob_;
	private: const std::size_t nq_;
	//private: const std::size_t nbuf_;
	private: mutable value_type q0_;
	private: mutable value_type qm_;
	private: mutable value_type quant_;
	private: mutable std::size_t nt_;
	private: mutable data_container data_;
	private: mutable data_container qile_;
	private: mutable std::vector<value_type> pval_;
	private: mutable bool init_;
}; // chambers2006_incremental_quantile_estimator


/**
 * The EWMA-based incremental quantile estimation method from (Chen,2000).
 *
 * \tparam ValueT Type for sample values.
 *
 * References:
 * - Fei Chen and Diane Lambert and José C. Pinheiro
 *   "Incremental Quantile Estimation for Massive Tracking",
 *   In Proc. of the 6th ACM SIGKDD International Conference on Knowledge Discovery and Data Mining (KDD'00), 2000
 * .
 *
 * \author Marco Guazzone (marco.guazzone@gmail)
 */
template <typename ValueT>
class chen2000_ewma_quantile_estimator: public base_estimator<ValueT>
{
	private: typedef base_estimator<ValueT> base_type;
	public: typedef typename base_type::value_type value_type;
	private: typedef typename base_type::data_container data_container;


	/**
	 * \brief Constructs a new data estimator.
	 *
	 * \param prob The probability value for which we want to estimate the
	 *  quantile value.
	 * \param w The smoothing factor.
	 * \param M The length of sample buffer used to compute the estimate.
	 * \param clamp Tells if the quantile estimate must be clamped between the
	 *  minimum and maximum values seen so far.
	 */
	public: explicit chen2000_ewma_quantile_estimator(value_type prob, value_type w = 0.05, std::size_t M = 1, bool clamp = false)
	: prob_(prob),
	  w_(w),
	  clamp_(clamp),
	  data_(),
	  sample_buf_(M),
	  ewma_(0),
	  qmin_(+std::numeric_limits<value_type>::infinity()),
	  qmax_(-std::numeric_limits<value_type>::infinity()),
	  init_(true)
	{
	}

	private: void do_collect(data_container const& data)
	{
		typedef typename data_container::const_iterator data_iterator;

		data_iterator data_end_it(data.end());
		for (data_iterator data_it = data.begin(); data_it != data_end_it; ++data_it)
		{
			const value_type val = *data_it;

			data_.push_back(val);
			sample_buf_.push_back(val);

			if (clamp_)
			{
				if (val < qmin_)
				{
					qmin_ = val;
				}
				if (val > qmax_)
				{
					qmax_ = val;
				}
			}
		}
	}

	private: value_type do_estimate() const
	{
		const std::size_t m = data_.size();

		if (m > 0)
		{
			const value_type q = (m > sample_buf_.size()) ? detail::quantile(data_.begin(), data_.end(), prob_, false)
														  : detail::quantile(sample_buf_.begin(), sample_buf_.end(), prob_, false);

			if (init_)
			{
				ewma_ = q;

				if (sample_buf_.full())
				{
					init_ = false;
				}
			}
			else
			{
				ewma_ = (1-w_)*ewma_ + w_*q;
			}

			if (clamp_)
			{
				ewma_ = dcs::math::clamp(ewma_, qmin_, qmax_);
			}

			data_.clear();
		}

		return ewma_;
	}

	private: void do_reset()
	{
		init_ = true;
		data_.clear();
		sample_buf_.clear();
		ewma_ = 0;
		qmin_ = +std::numeric_limits<value_type>::infinity();
		qmax_ = -std::numeric_limits<value_type>::infinity();
	}

//	private: std::size_t do_count() const
//	{
//		return data_.size();
//	}


	private: value_type prob_; //< The probability of the quantile we want to estimate
	private: value_type w_; //< Smoothing constant
	private: bool clamp_; //< Tells if the current estimate must be constrained in the range of [qmin_,qmax_].
	private: mutable std::vector<value_type> data_; //< Last collected sample values.
	private: boost::circular_buffer<value_type> sample_buf_; //< Sample window used in case there are too few samples to update quantile estimate
	private: mutable value_type ewma_; //< The current EWMA estimate A_n^* for the quantile
	private: value_type qmin_; //< The minimum quantile value estimated so far.
	private: value_type qmax_; //< The maximum quantile value estimated so far.
	private: mutable bool init_; //< Tells if the estimate has been already initialized or not.
}; // chen2000_ewma_quantile_estimator


/**
 * The EWSA-based incremental quantile estimation method from (Chen,2000).
 *
 * \tparam ValueT Type for sample values.
 *
 * References:
 * - Fei Chen and Diane Lambert and José C. Pinheiro
 *   "Incremental Quantile Estimation for Massive Tracking",
 *   In Proc. of the 6th ACM SIGKDD International Conference on Knowledge Discovery and Data Mining (KDD'00), 2000
 * .
 *
 * \author Marco Guazzone (marco.guazzone@gmail)
 */
template <typename ValueT>
class chen2000_ewsa_quantile_estimator: public base_estimator<ValueT>
{
	private: typedef base_estimator<ValueT> base_type;
	public: typedef typename base_type::value_type value_type;
	private: typedef typename base_type::data_container data_container;


	/**
	 * \brief Constructs a new data estimator.
	 *
	 * \param prob The probability value for which we want to estimate the
	 *  quantile value.
	 * \param w The smoothing factor.
	 * \param M The length of sample buffer used to compute the estimate.
	 * \param clamp Tells if the quantile estimate must be clamped between the
	 *  minimum and maximum values seen so far.
	 */
	public: explicit chen2000_ewsa_quantile_estimator(value_type prob, value_type w = 0.05, std::size_t M = 1, bool clamp = false)
	: prob_(prob),
	  w_(w),
	  clamp_(clamp),
	  data_(),
	  sample_buf_(M),
	  sn_(std::numeric_limits<value_type>::quiet_NaN()),
	  fn_(std::numeric_limits<value_type>::quiet_NaN()),
	  rn_(std::numeric_limits<value_type>::quiet_NaN()),
	  cn_(std::numeric_limits<value_type>::quiet_NaN()),
	  qmin_(+std::numeric_limits<value_type>::quiet_NaN()),
	  qmax_(-std::numeric_limits<value_type>::quiet_NaN()),
	  init_(true)
	{
	}

	private: void do_collect(data_container const& data)
	{
		typedef typename data_container::const_iterator data_iterator;

		const data_iterator data_end_it = data.end();
		for (data_iterator data_it = data.begin(); data_it != data_end_it; ++data_it)
		{
			const value_type val = *data_it;

			data_.push_back(val);
			sample_buf_.push_back(val);

			if (clamp_)
			{
				if (val < qmin_)
				{
					qmin_ = val;
				}
				if (val > qmax_)
				{
					qmax_ = val;
				}
			}
		}
	}

	private: value_type do_estimate() const
	{
		const std::size_t m = data_.size();

		if (m > 0)
		{
			if (init_)
			{
				const std::size_t M = std::max(m, sample_buf_.size());
				const value_type p25_75_x[] = {0.25, 0.75, prob_};
				const std::vector<value_type> q25_75_x = (m > sample_buf_.size()) ? detail::quantile<value_type>(data_.begin(), data_.end(), p25_75_x, p25_75_x+(sizeof p25_75_x/sizeof(value_type)), false)
																				  : detail::quantile<value_type>(sample_buf_.begin(), sample_buf_.end(), p25_75_x, p25_75_x+(sizeof p25_75_x/sizeof(value_type)), false);

				// Set the initial estimate S_0^* equal to the q^\text{th} sample quantile
				// \hat{Q}_n of X_{01},\ldots,X_{0M}
				//sn_ = (m > sample_buf_.size()) ? detail::quantile(data_.begin(), data_.end(), prob_, false)
				//								 : detail::quantile(sample_buf_.begin(), sample_buf_.end(), prob_, false);
				sn_ = q25_75_x[2];
				sn25_ = q25_75_x[0];
				sn75_ = q25_75_x[1];
				// Estimate the scale r_0^* of f_0^* by the interquantile range of
				// X_{01},\ldots,X_{0M}; i.e., by the difference of the .75 and .25 sample
				// quantiles
				rn_ = q25_75_x[1]-q25_75_x[0];
				// Then take c_0^* = r_0^* M^{-1} \sum_{i=1}^M i^{-1/2}
				cn_ = 1;
				for (std::size_t i = 2; i <= M; ++i)
				{
					cn_ += 1.0/std::sqrt(i);
				}
				// FIXME: unlike the Chen's paper, we deal with the case of rn_ ~= 0
				cn_ *= std::max(rn_, 0.01)/M;
				// Take f_0^* = (2 c_0^* M)^{-1} \max\{\#\{|X_{0i}-S_0^*| \le c_0^*\},1\}
				// which is the density of observations in a neighborhood of width 2c_0^* of
				// S_0^*, unless the fraction of neighborhood is zero
				std::size_t cnt = 0;
				std::size_t cnt25 = 0;
				std::size_t cnt75 = 0;
				if (m > sample_buf_.size())
				{
					for (std::size_t i = 0; i < M; ++i)
					{
						if (m > sample_buf_.size())
						{
							if (std::abs(data_[i]-sn_) <= cn_)
							{
								++cnt;
							}
							if (std::abs(data_[i]-sn25_) <= cn_)
							{
								++cnt25;
							}
							if (std::abs(data_[i]-sn75_) <= cn_)
							{
								++cnt75;
							}
						}
						else
						{
							if (std::abs(sample_buf_[i]-sn_) <= cn_)
							{
								++cnt;
							}
							if (std::abs(sample_buf_[i]-sn25_) <= cn_)
							{
								++cnt25;
							}
							if (std::abs(sample_buf_[i]-sn75_) <= cn_)
							{
								++cnt75;
							}
						}
					}
				}
				fn_ = 1.0/(2.0*cn_*M)*std::max(cnt, static_cast<std::size_t>(1U));
				fn25_ = 1.0/(2.0*cn_*M)*std::max(cnt25, static_cast<std::size_t>(1U));
				fn75_ = 1.0/(2.0*cn_*M)*std::max(cnt75, static_cast<std::size_t>(1U));

				if (sample_buf_.full())
				{
					init_ = false;
				}
			}
			else
			{
				const std::size_t M =  std::max(m, sample_buf_.size());

				// S_n^* = S_{n-1}^*+\frac{w}{f_{n-1}^*}(p-\frac{\#\{X_{ni} \le S_{n-1}^*\}}{M})
				// f_n^* = (1-w)f_{n-1}^*+\frac{w}{2c_{n-1}^*M}\#\{|X_{ni}-S_{n-1}^*| \le c_{n-1}^*\}
				std::size_t scnt = 0;
				std::size_t scnt25 = 0;
				std::size_t scnt75 = 0;
				std::size_t fcnt = 0;
				std::size_t fcnt25 = 0;
				std::size_t fcnt75 = 0;
				for (std::size_t i = 0; i < M; ++i)
				{
					if (m > sample_buf_.size())
					{
						if (data_[i] <= sn_)
						{
							++scnt;
						}
						if (std::abs(data_[i]-sn_) <= cn_)
						{
							++fcnt;
						}
						if (data_[i] <= sn25_)
						{
							++scnt25;
						}
						if (std::abs(data_[i]-sn25_) <= cn_)
						{
							++fcnt25;
						}
						if (data_[i] <= sn75_)
						{
							++scnt75;
						}
						if (std::abs(data_[i]-sn75_) <= cn_)
						{
							++fcnt75;
						}
					}
					else
					{
						if (sample_buf_[i] <= sn_)
						{
							++scnt;
						}
						if (std::abs(sample_buf_[i]-sn_) <= cn_)
						{
							++fcnt;
						}
						if (sample_buf_[i] <= sn25_)
						{
							++scnt25;
						}
						if (std::abs(sample_buf_[i]-sn25_) <= cn_)
						{
							++fcnt25;
						}
						if (sample_buf_[i] <= sn75_)
						{
							++scnt75;
						}
						if (std::abs(sample_buf_[i]-sn75_) <= cn_)
						{
							++fcnt75;
						}
					}
				}
				//const value_type q25 = sn_ + (w_/fn_)*(0.25-scnt/static_cast<value_type>(m));
				//const value_type q75 = sn_ + (w_/fn_)*(0.75-scnt/static_cast<value_type>(m));
				sn_ += (w_/fn_)*(prob_-scnt/static_cast<value_type>(M));
				sn25_ += (w_/fn25_)*(0.25-scnt25/static_cast<value_type>(M));
				sn75_ += (w_/fn75_)*(0.75-scnt75/static_cast<value_type>(M));
				fn_ = (1-w_)*fn_ + (w_/(2.0*cn_*M))*fcnt;
				fn25_ = (1-w_)*fn25_ + (w_/(2.0*cn_*M))*fcnt25;
				fn75_ = (1-w_)*fn75_ + (w_/(2.0*cn_*M))*fcnt75;
				// Take r_n^∗ to be the difference of the current EWSA estimates for the
				// .75 and .25 quantiles, and define the neighborhood size for the next
				// updating step to be c_n^* = r_n^∗ c, with c = M^{-1} \sum_{i=M+1}^{2M} i^{-1/2}.
				////rn_ = q25_75[1]-q25_75[0];
				//rn_ = q75-q25;
				rn_ = sn75_-sn25_;
				const std::size_t M2 = 2*M;
				cn_ = 0;
				for (std::size_t i = M+1; i <= M2; ++i)
				{
					cn_ += 1.0/std::sqrt(i);
				}
				cn_ /= M;
				// FIXME: unlike the Chen's paper, we handle the case of rn_ ~= 0
				cn_ *= std::max(rn_, 0.01);
			}

			if (clamp_)
			{
				sn_ = dcs::math::clamp(sn_, qmin_, qmax_);
			}

			data_.clear();
		}

		return sn_;
	}

	private: void do_reset()
	{
		init_ = true;
		data_.clear();
		sample_buf_.clear();
		sn_ = fn_
			= rn_
			= cn_
			= std::numeric_limits<value_type>::quiet_NaN();
		sn25_ = sn75_
			  = fn25_
			  = fn75_
			  = std::numeric_limits<value_type>::quiet_NaN();
		qmin_ = +std::numeric_limits<value_type>::infinity();
		qmax_ = -std::numeric_limits<value_type>::infinity();
	}

//	private: std::size_t do_count() const
//	{
//		return data_.size();
//	}


	private: value_type prob_; //< The probability of the quantile we want to estimate
	private: value_type w_; //< Smoothing constant.
	private: bool clamp_; //< Tells if the current estimate must be constrained in the range of [qmin_,qmax_].
	private: mutable std::vector<value_type> data_; //< Last collected sample values.
	private: boost::circular_buffer<value_type> sample_buf_; //< Sample window used in case there are too few samples to update quantile estimate.
	private: mutable value_type sn_; //< The current EWSA estimate S_n^*.
	private: mutable value_type fn_; //< The current estimate f_n^* of the population density at the wanted quantile.
	private: mutable value_type rn_; //< The current estimate r_n^* of the scale of the population density f_n^*.
	private: mutable value_type cn_; //< The current neighborhood size c_n^*
	private: mutable value_type sn25_; //< The current EWSA estimate for the .25th quantile.
	private: mutable value_type sn75_; //< The current EWSA estimate for the .75th quantile.
	private: mutable value_type fn25_; //< The current estimate of the population density at the .25th quantile.
	private: mutable value_type fn75_; //< The current estimate of the population density at the .75th quantile.
	private: value_type qmin_; //< The minimum quantile value estimated so far.
	private: value_type qmax_; //< The maximum quantile value estimated so far.
	private: mutable bool init_; //< Tells if the estimate has been already initialized or not.
}; // chen2000_ewsa_quantile_estimator


/**
 * The SA-based incremental quantile estimation method from (Chen,2000).
 *
 * \tparam ValueT Type for sample values.
 *
 * References:
 * - Fei Chen and Diane Lambert and José C. Pinheiro
 *   "Incremental Quantile Estimation for Massive Tracking",
 *   In Proc. of the 6th ACM SIGKDD International Conference on Knowledge Discovery and Data Mining (KDD'00), 2000
 * .
 *
 * \author Marco Guazzone (marco.guazzone@gmail)
 */
template <typename ValueT>
class chen2000_sa_quantile_estimator: public base_estimator<ValueT>
{
	private: typedef base_estimator<ValueT> base_type;
	public: typedef typename base_type::value_type value_type;
	private: typedef typename base_type::data_container data_container;


	/**
	 * \brief Constructs a new data estimator.
	 *
	 * \param prob The probability value for which we want to estimate the
	 *  quantile value.
	 * \param clamp Tells if the quantile estimate must be clamped between the
	 *  minimum and maximum values seen so far.
	 */
	public: explicit chen2000_sa_quantile_estimator(value_type prob, std::size_t M = 1, bool clamp = false)
	: prob_(prob),
	  clamp_(clamp),
	  data_(),
	  sample_buf_(M),
	  sn_(std::numeric_limits<value_type>::quiet_NaN()),
	  fn_(std::numeric_limits<value_type>::quiet_NaN()),
	  f0_(std::numeric_limits<value_type>::quiet_NaN()),
	  n_(0),
	  qmin_(+std::numeric_limits<value_type>::quiet_NaN()),
	  qmax_(-std::numeric_limits<value_type>::quiet_NaN()),
	  init_(true)
	{
	}

	private: void do_collect(data_container const& data)
	{
		typedef typename data_container::const_iterator data_iterator;

		data_iterator data_end_it(data.end());
		for (data_iterator data_it = data.begin(); data_it != data_end_it; ++data_it)
		{
			const value_type val = *data_it;

			data_.push_back(val);
			sample_buf_.push_back(val);

			if (clamp_)
			{
				if (val < qmin_)
				{
					qmin_ = val;
				}
				if (val > qmax_)
				{
					qmax_ = val;
				}
			}
		}
	}

	private: value_type do_estimate() const
	{
		const std::size_t m = data_.size();

		if (m > 0)
		{
			if (init_)
			{
/*
				// FIXME: the initialization step is not clear in the Chen's paper, so we propose an our one
				// - f0_ and fn_ are initialized to the IQR if m > 1, or to 1 otherwise
				// - sn_ is initialized with the true sample quantile of the first sample

				if (m > 1)
				{
					const value_type p25_75[] = {0.25, 0.75};
					const std::vector<value_type> q25_75 = detail::quantile<value_type>(data_.begin(), data_.end(), p25_75, p25_75+2, false);

					fn_ = f0_ = q25_75[1]-q25_75[0]; // Estimate f_0 with IQR
				}
				else
				{
					fn_ = f0_ = 1;
				}

				// NOTE: Unlike Chen's paper, for S0 we use the true sample quantile of current data.
				// This seems to produce better results.
				//sn_ = (1.0/f0_)*prob_;
				sn_ = detail::quantile(data_.begin(), data_.end(), prob_, false);
				init_ = false;
*/

				// FIXME: the initialization step for SA is not clear in the Chen's paper, so we use the same of EWSA

				const std::size_t M = std::max(m, sample_buf_.size());
				const value_type p25_75_x[] = {0.25, 0.75, prob_};
				const std::vector<value_type> q25_75_x = (m > sample_buf_.size()) ? detail::quantile<value_type>(data_.begin(), data_.end(), p25_75_x, p25_75_x+(sizeof p25_75_x/sizeof(value_type)), false)
																				  : detail::quantile<value_type>(sample_buf_.begin(), sample_buf_.end(), p25_75_x, p25_75_x+(sizeof p25_75_x/sizeof(value_type)), false);

				// Set the initial estimate S_0^* equal to the q^\text{th} sample quantile
				// \hat{Q}_n of X_{01},\ldots,X_{0M}
				sn_ = q25_75_x[2];
				// Estimate the scale r_0^* of f_0^* by the interquantile range of
				// X_{01},\ldots,X_{0M}; i.e., by the difference of the .75 and .25 sample
				// quantiles
				const value_type r0 = q25_75_x[1]-q25_75_x[0];
				// Then take c_0^* = r_0^* M^{-1} \sum_{i=1}^M i^{-1/2}
				value_type c0 = 1;
				for (std::size_t i = 2; i <= M; ++i)
				{
					c0 += 1.0/std::sqrt(i);
				}
				// FIXME: unlike the Chen's paper, we deal with the case of r0 ~= 0
				c0 *= std::max(r0, 0.01)/M;
				// Take f_0^* = (2 c_0^* M)^{-1} \max\{\#\{|X_{0i}-S_0^*| \le c_0^*\},1\}
				// which is the density of observations in a neighborhood of width 2c_0^* of
				// S_0^*, unless the fraction of neighborhood is zero
				std::size_t cnt = 0;
				if (m > sample_buf_.size())
				{
					for (std::size_t i = 0; i < M; ++i)
					{
						if (m > sample_buf_.size())
						{
							if (std::abs(data_[i]-sn_) <= c0)
							{
								++cnt;
							}
						}
						else
						{
							if (std::abs(sample_buf_[i]-sn_) <= c0)
							{
								++cnt;
							}
						}
					}
				}
				fn_ = f0_
					= 1.0/(2.0*c0*M)*std::max(cnt, static_cast<std::size_t>(1U));

				if (sample_buf_.full())
				{
					init_ = false;
				}
			}
			else
			{
				const std::size_t M =  std::max(m, sample_buf_.size());

				++n_;

				const value_type wn = 1.0/n_; // w_n
				const value_type cn = std::sqrt(wn); // c_n

				std::size_t cncnt = 0; // #_{i=1}^M \{|X_{ni}-S_{ni}| \le c_{ni}\}
				std::size_t sncnt = 0; // #_{i=1}^M \{X_{ni} \le S_{n-1}\}
				for (std::size_t i = 0; i < M; ++i)
				{
					if (m > sample_buf_.size())
					{
						if (std::abs(data_[i]- sn_) <= cn)
						{
							++cncnt;
						}
						if (data_[i] <= sn_)
						{
							++sncnt;
						}
					}
					else
					{
						if (std::abs(sample_buf_[i]- sn_) <= cn)
						{
							++cncnt;
						}
						if (sample_buf_[i] <= sn_)
						{
							++sncnt;
						}
					}
				}

				//FIXME: Unlike Chen's paper, we deal with the case of f_0/\sqrt{n-1} \approx \infty when n=1
				const value_type en = n_ > 1 ? std::max(fn_, f0_/std::sqrt(n_-1))
											 : f0_; // e_n

				sn_ += (wn/en)*(prob_-sncnt/static_cast<value_type>(M)); // S_n

				fn_ = (1-wn)*fn_ + wn*cncnt/(2.0*cn*M); // f_n
			}

			if (clamp_)
			{
				sn_ = dcs::math::clamp(sn_, qmin_, qmax_);
			}

			data_.clear();
		}

		return sn_;
	}

	private: void do_reset()
	{
		init_ = true;
		data_.clear();
		sample_buf_.clear();
		sn_ = fn_
			= f0_
			= std::numeric_limits<value_type>::quiet_NaN();
		qmin_ = +std::numeric_limits<value_type>::infinity();
		qmax_ = -std::numeric_limits<value_type>::infinity();
		n_ = 0;
	}

//	private: std::size_t do_count() const
//	{
//		return data_.size();
//	}


	private: value_type prob_; //< The probability of the quantile we want to estimate
	private: bool clamp_; //< Tells if the current estimate must be constrained in the range of [qmin_,qmax_].
	private: mutable std::vector<value_type> data_; //< Last collected sample values.
	private: boost::circular_buffer<value_type> sample_buf_; //< Sample window used in case there are too few samples to update quantile estimate.
	private: mutable value_type sn_; //< The current SA estimate S_n.
	private: mutable value_type fn_; //< The current estimate f_n of the population density at the wanted quantile.
	private: mutable value_type f0_; //< The initial estimate f_0 of the population density at the wanted quantile.
	private: mutable std::size_t n_; //< The number of times the estimate has been updated.
	private: value_type qmin_; //< The minimum quantile value estimated so far.
	private: value_type qmax_; //< The maximum quantile value estimated so far.
	private: mutable bool init_; //< Tells if the estimate has been already initialized or not.
}; // chen2000_sa_quantile_estimator


/**
 * \brief The t-digest algorithm for incremental quantile estimation.
 *
 * \tparam ValueT Type for sample values.
 *
 * References:
 * - Ted Dunning and Otmar Ertl
 *   "Computing Extremely Accurate Quantiles Using t-digests"
 *   Technical Report, 2013.
 *   Available online: https://github.com/tdunning/t-digest
 * .
 *
 * \author Marco Guazzone (marco.guazzone@gmail.com)
 */
template <typename ValueT>
class dunning2013_tdigest_quantile_estimator: public base_estimator<ValueT>
{
	private: typedef base_estimator<ValueT> base_type;
	public: typedef typename base_type::value_type value_type;
	private: typedef typename base_type::data_container data_container;


	public: enum digest_category {
				array_digest_category,
				avltree_digest_category,
				default_digest_category,
				tree_digest_category
			};


	public: explicit dunning2013_tdigest_quantile_estimator(value_type prob, digest_category digest = default_digest_category, double compression = 100)
	: prob_(prob),
	  digest_(digest),
	  compression_(compression),
	  p_impl_(new tdigestx::TDigestProxy(detail::jni::jni_helper::get().java_vm()))
	{
		this->init();
	}

	public: ~dunning2013_tdigest_quantile_estimator()
	{
		if (p_impl_)
		{
			delete p_impl_;
		}
	}

	private: void do_collect(data_container const& data)
	{
		typedef typename data_container::const_iterator data_iterator;

		data_iterator data_end_it(data.end());
		for (data_iterator data_it = data.begin(); data_it != data_end_it; ++data_it)
		{
			const value_type val = *data_it;

			p_impl_->add(val);
#ifdef DCS_TESTBED_JNI_ENABLE_DIAGNOSTICS
			detail::jni::check_exception(detail::jni::jni_helper::get().env());
#endif // DCS_TESTBED_JNI_ENABLE_DIAGNOSTICS
		}
	}

	private: value_type do_estimate() const
	{
#ifdef DCS_TESTBED_JNI_ENABLE_DIAGNOSTICS
		value_type q = p_impl_->quantile(prob_);
		detail::jni::check_exception(detail::jni::jni_helper::get().env());
		return q;
#else
		return p_impl_->quantile(prob_);
#endif // DCS_TESTBED_JNI_ENABLE_DIAGNOSTICS
	}

	private: void do_reset()
	{
		this->init();
	}

//	private: std::size_t do_count() const
//	{
//		return ::boost::accumulators::count(acc_);
//	}

	private: void init()
	{
		if (p_impl_)
		{
			int digest_type;

			switch (digest_)
			{
				case array_digest_category:
					digest_type = tdigestx::TDigestProxy::getArrayDigestType(detail::jni::jni_helper::get().java_vm());
					break;
				case avltree_digest_category:
					digest_type = tdigestx::TDigestProxy::getAvlTreeDigestType(detail::jni::jni_helper::get().java_vm());
					break;
				case default_digest_category:
					digest_type = tdigestx::TDigestProxy::getDefaultDigestType(detail::jni::jni_helper::get().java_vm());
					break;
				case tree_digest_category:
					digest_type = tdigestx::TDigestProxy::getTreeDigestType(detail::jni::jni_helper::get().java_vm());
					break;
				default:
					throw std::runtime_error("Unknown digest category");
			}
			p_impl_->init(digest_type, compression_);
#ifdef DCS_TESTBED_JNI_ENABLE_DIAGNOSTICS
			detail::jni::check_exception(detail::jni::jni_helper::get().env());
#endif // DCS_TESTBED_JNI_ENABLE_DIAGNOSTICS
		}
	}


	private: value_type prob_;
	private: digest_category digest_;
	private: double compression_;
	//private: JavaVM* p_jvm_;
	private: tdigestx::TDigestProxy* p_impl_;
}; // dunning2013_tdigest_quantile_estimator



/**
 * \brief The P^2 Algorithm for incremental quantile estimation.
 *
 * \tparam ValueT Type for sample values.
 *
 * References:
 * - Raj Jain and Imrich Chlamtac,
 *   "The P^2 Algorithm for Dynamic Calculation of Quantiles and Histograms Without Storing Observations"
 *   Communications of the ACM, 28(10):1076-1085 1985
 * .
 *
 * \author Marco Guazzone (marco.guazzone@gmail.com)
 */
template <typename ValueT>
class jain1985_p2_algorithm_quantile_estimator: public base_estimator<ValueT>
{
	private: typedef base_estimator<ValueT> base_type;
	public: typedef typename base_type::value_type value_type;
	private: typedef typename base_type::data_container data_container;
	private: typedef ::boost::accumulators::accumulator_set<value_type,
															::boost::accumulators::stats< ::boost::accumulators::tag::p_square_quantile > > accumulator_type;


	public: explicit jain1985_p2_algorithm_quantile_estimator(value_type prob)
	: prob_(prob),
	  acc_(::boost::accumulators::quantile_probability = prob)
	{
	}

	private: void do_collect(data_container const& data)
	{
		typedef typename data_container::const_iterator data_iterator;

		data_iterator data_end_it(data.end());
		for (data_iterator data_it = data.begin(); data_it != data_end_it; ++data_it)
		{
			value_type val(*data_it);

			acc_(val);
		}
	}

	private: value_type do_estimate() const
	{
		return ::boost::accumulators::p_square_quantile(acc_);
	}

	private: void do_reset()
	{
		acc_ = accumulator_type(::boost::accumulators::quantile_probability = prob_);
	}

//	private: std::size_t do_count() const
//	{
//		return ::boost::accumulators::count(acc_);
//	}


	private: value_type prob_;
	private: accumulator_type acc_;
}; // jain1985_p2_algorithm_quantile_estimator


/*
 * \brief Incremental sample mean estimation.
 *
 * \tparam ValueT Type for sample values.
 *
 * \author Marco Guazzone (marco.guazzone@gmail.com)
 */
template <typename ValueT>
class mean_estimator: public base_estimator<ValueT>
{
	private: typedef base_estimator<ValueT> base_type;
	public: typedef typename base_type::value_type value_type;
	private: typedef typename base_type::data_container data_container;
	private: typedef ::boost::accumulators::accumulator_set<value_type,
															::boost::accumulators::stats< ::boost::accumulators::tag::mean > > accumulator_type;


	public: mean_estimator()
	: acc_()
	{
	}

	private: void do_collect(data_container const& data)
	{
		typedef typename data_container::const_iterator data_iterator;

		data_iterator data_end_it(data.end());
		for (data_iterator data_it = data.begin(); data_it != data_end_it; ++data_it)
		{
			value_type val(*data_it);

			acc_(val);
		}
	}

	private: value_type do_estimate() const
	{
		return ::boost::accumulators::mean(acc_);
	}

	private: void do_reset()
	{
		acc_ = accumulator_type();
	}

//	private: std::size_t do_count() const
//	{
//		return ::boost::accumulators::count(acc_);
//	}


	private: accumulator_type acc_;
}; // mean_estimator


/**
 * \brief Data estimator that returns the most recently observed value.
 *
 * \tparam ValueT Type for sample values.
 *
 * \author Marco Guazzone (marco.guazzone@gmail.com)
 */
template <typename ValueT>
class most_recently_observed_estimator: public base_estimator<ValueT>
{
	private: typedef base_estimator<ValueT> base_type;
	public: typedef typename base_type::value_type value_type;
	private: typedef typename base_type::data_container data_container;
	private: typedef ::boost::accumulators::accumulator_set<value_type,
															::boost::accumulators::stats< ::boost::accumulators::tag::mean > > accumulator_type;


	public: most_recently_observed_estimator()
	: mro_(std::numeric_limits<value_type>::quiet_NaN())
	{
	}

	private: void do_collect(data_container const& data)
	{
		mro_ = data.back();
	}

	private: value_type do_estimate() const
	{
		return mro_;
	}

	private: void do_reset()
	{
		mro_ = std::numeric_limits<value_type>::quiet_NaN();
	}

//	private: std::size_t do_count() const
//	{
//		return ::boost::accumulators::count(acc_);
//	}


	private: value_type mro_;
}; // most_recently_observed_estimator


/**
 * \brief Incremental quantile estimation method found at
 *  https://stackoverflow.com/a/2144754 .
 *
 * \tparam ValueT Type for sample values.
 *
 * Here is a brief description of the method:
 * "I use these incremental/recursive mean and median estimators, which both use
 *  constant storage:
 *
 *   mean += eta * (sample - mean)
 *   median += eta * sgn(sample - median)
 *
 *  where eta is a small learning rate parameter (e.g. 0.001), and sgn() is the
 *  signum function which returns one of {-1, 0, 1}.
 *  Use a constant eta if the data is non-stationary and you want to track
 *  changes over time; otherwise, for stationary sources you can use something
 *  like eta=1/n for the mean estimator, where n is the number of samples seen
 *  so far... unfortunately, this does not appear to work for the median
 *  estimator.
 *
 *  This type of incremental mean estimator seems to be used all over the place,
 *  e.g. in unsupervised neural network learning rules, but the median version
 *  seems much less common, despite its benefits (robustness to outliers).
 *  It seems that the median version could be used as a replacement for the
 *  mean estimator in many applications.
 *
 *  The above incremental median estimator can be generalized to estimate
 *  arbitrary quantiles.
 *  In general, a quantile function (http://en.wikipedia.org/wiki/Quantile_function)
 *  tells you the value that divides the data into two fractions: p and 1-p.
 *  The following estimates this value incrementally:
 *
 *   quantile += eta * (sgn(sample - quantile) + 2.0 * p - 1.0)
 *
 *  The value p should be within [0,1].
 *  This essentially shifts the sgn() function's symmetrical output {-1,0,1} to
 *  lean toward one side, partitioning the data samples into two unequally-sized
 *  bins (fractions p and 1-p of the data are less than/greater than the
 *  quantile estimate, respectively).
 *  Note that for p=0.5, this reduces to the median estimator.
 *
 *  ADDEDUM: Imagine that the numbers have units, e.g., millimeters.
 *  Then it's clear eta (for the estimate of the median) has to have the same
 *  units as the measurements, and so a generic value like 0.001 simply doesn't
 *  make any sense.
 *  A seemingly-better approach is to set eta from a running estimate of the
 *  absolute deviation: for each new value sample, update
 *  cumadev += abs(sample-median).
 *  Then set eta = 1.5*cumadev/(k*k), where k is the number of samples seen so
 *  far.
 *
 *  This method is inspired by the work described in (Byron,1997).
 *
 * References:
 * - Bylander, T.; Rosen, B.,
 *   "A perceptron-like online algorithm for tracking the median,"
 *   In Proc. of the International Conference on Neural Networks,
 *   vol.4, no., pp.2219-2224 vol.4, 9-12 Jun 1997.
 *   doi: 10.1109/ICNN.1997.614292
 * .
 *
 * \author Marco Guazzone (marco.guazzone@gmail.com)
 */
template <typename ValueT>
class recursive_quantile_estimator: public base_estimator<ValueT>
{
	private: typedef base_estimator<ValueT> base_type;
	public: typedef typename base_type::value_type value_type;
	private: typedef typename base_type::data_container data_container;


	public: explicit recursive_quantile_estimator(value_type prob, value_type w = 0.05)
	: prob_(prob),
	  w_(w),
	  k_(0),
	  cumadev_(0),
	  m_(std::numeric_limits<value_type>::quiet_NaN()),
	  q_(std::numeric_limits<value_type>::quiet_NaN()),
	  init_(true)
	{
	}

	private: void do_collect(data_container const& data)
	{
		typedef typename data_container::const_iterator data_iterator;

		data_iterator data_end_it(data.end());
		for (data_iterator data_it = data.begin(); data_it != data_end_it; ++data_it)
		{
			value_type val(*data_it);

			data_.push_back(val);
		}
	}

	private: value_type do_estimate() const
	{
		const std::size_t n = data_.size();

		if (n > 0)
		{
			if (init_)
			{
				const value_type p[2] = {prob_, 0.5};

				const std::vector<value_type> q = detail::quantile<value_type>(data_.begin(), data_.end(), p, p+2, false);
				q_ = q[0];
				m_ = q[1];
				init_ = false;
				k_ += n;
			}
			else
			{
				// This essentially shifts the sgn() function's symmetrical output {-1,0,1}
				// to lean toward one side, partitioning the data samples into two
				// unequally-sized bins (fractions p and 1-p of the data are less
				// than/greater than the quantile estimate, respectively).
				// Note that for p=0.5, this reduces to the median estimator.
				//
				// Use a constant w_ if the data is non-stationary and you want to track
				// changes over time; otherwise, for stationary sources you can use
				// something like w=1/n

				for (std::size_t i = 0; i < n; ++i)
				{
					++k_;
					m_ += w_*::dcs::math::sign(data_[i]-m_);

					// A generic value like 0.001 simply doesn't make any sense.
					// A seemingly-better approach is to set eta from a running estimate of the absolute deviation
					cumadev_ += std::abs(data_[i]-m_);
					const value_type w = 1.5*cumadev_/(k_*k_);
					q_ += w*(::dcs::math::sign(data_[i]-q_) + 2.0*prob_ - 1);
				}
			}

			data_.clear();
		}

		return q_;
	}

	private: void do_reset()
	{
		data_.clear();
		k_ = 0;
		m_ = q_
		   = std::numeric_limits<value_type>::quiet_NaN();
		cumadev_ = 0;
		init_ = true;
	}

//	private: std::size_t do_count() const
//	{
//		return ::boost::accumulators::count(acc_);
//	}


	private: value_type prob_;
	private: value_type w_;
	private: mutable data_container data_;
	private: mutable std::size_t k_;
	private: mutable value_type cumadev_;
	private: mutable value_type m_;
	private: mutable value_type q_;
	private: mutable bool init_;
}; // recursive_quantile_estimator


/**
 * \brief Incremental quantile estimation based on the true sample quantile
 *  formula.
 *
 * \tparam ValueT Type for sample values.
 *
 * \author Marco Guazzone (marco.guazzone@gmail.com)
 */
template <typename ValueT>
class true_quantile_estimator: public base_estimator<ValueT>
{
	private: typedef base_estimator<ValueT> base_type;
	public: typedef typename base_type::value_type value_type;
	private: typedef typename base_type::data_container data_container;


	public: explicit true_quantile_estimator(value_type prob)
	: prob_(prob),
	  type_(detail::type7_quantile),
	  data_()
	{
	}

	private: void do_collect(data_container const& data)
	{
#if 1 /* This performs insertion sort. By using the std::list data structure, ordered insertion takes just linear time */
		typedef typename data_container::const_iterator data_iterator;
		typedef typename std::list<value_type>::iterator impl_iterator;

		const data_iterator data_end_it = data.end();
		for (data_iterator data_it = data.begin(); data_it != data_end_it; ++data_it)
		{
			value_type val = *data_it;

			const impl_iterator impl_end_it = data_.end();
			impl_iterator impl_it = data_.begin();
			while (impl_it != impl_end_it && *impl_it < val)
			{
				++impl_it;
			}
			data_.insert(impl_it, val);
		}
//TODO: use a different data structure (e.g., a vector) for the case of unordered insertions
#else /* This performs unorderd insertion */
		typedef typename data_container::const_iterator data_iterator;

		const data_iterator data_end_it = data.end();
		for (data_iterator data_it = data.begin(); data_it != data_end_it; ++data_it)
		{
			const value_type val = *data_it;

			data_.push_back(val);
		}
#endif
	}

	private: value_type do_estimate() const
	{
#if 1 /* This assumes that 'do_collect' inserted data in order */
		return detail::quantile(data_.begin(), data_.end(), prob_, true, type_);
#else /* This assumes that 'do_collect' didn't insert data in order */
		return detail::quantile(data_.begin(), data_.end(), prob_, false, type_);
#endif
	}

	private: void do_reset()
	{
		data_.clear();
	}


	private: value_type prob_;
	private: detail::quantile_category type_;
	private: std::list<value_type> data_;
}; // true_quantile_estimator


/**
 * \brief The EWMA-based incremental quantile estimation method from
 *  (Welsh,2003).
 *
 * \tparam ValueT Type for sample values.
 *
 * The extended version extends the original one, by computing the quantile with
 * a linear interpolation.
 *
 * References:
 * - Matt Welsh and David Culler,
 *   "Adaptive Overload Control for Busy Internet Servers",
 *   In Proc. of the 4th Conference on USENIX Symposium on Internet Technologies and Systems (USITS'03), 2003
 *
 * \author Marco Guazzone (marco.guazzone@gmail.com)
 */
template <typename ValueT>
class welsh2003_ewma_quantile_estimator: public base_estimator<ValueT>
{
	private: typedef base_estimator<ValueT> base_type;
	public: typedef typename base_type::value_type value_type;
	private: typedef typename base_type::data_container data_container;


	public: explicit welsh2003_ewma_quantile_estimator(value_type prob, value_type alpha = 0.7, bool extended = false)
	: prob_(prob),
	  alpha_(alpha),
	  data_(),
	  ewma_(0),
	  init_(true),
	  ext_(extended)
	{
	}

	private: void do_collect(data_container const& data)
	{
		typedef typename data_container::const_iterator data_iterator;

		data_iterator data_end_it(data.end());
		for (data_iterator data_it = data.begin(); data_it != data_end_it; ++data_it)
		{
			value_type val(*data_it);

			data_.push_back(val);
		}
	}

	private: value_type do_estimate() const
	{
		const std::size_t m = data_.size();

		if (m > 0)
		{
			value_type q = 0;

			if (ext_)
			{
				q = detail::quantile(data_.begin(), data_.end(), prob_, false);
			}
			else
			{
				std::size_t np = std::min(static_cast< std::size_t >(std::ceil(prob_*data_.size())), data_.size());
				//std::partial_sort(data_.begin(), data_.begin()+np, data_.end());
				std::sort(data_.begin(), data_.end());
				q = data_[np-1];
			}

			if (init_)
			{
				ewma_ = q;
				init_ = false;
			}
			else
			{
				ewma_ = alpha_*ewma_+(1-alpha_)*q;
			}
			data_.clear();
		}

		return ewma_;
	}

	private: void do_reset()
	{
		init_ = true;
		data_.clear();
		ewma_ = 0;
	}

//	private: std::size_t do_count() const
//	{
//		return data_.size();
//	}


	private: value_type prob_;
	private: value_type alpha_;
	private: mutable data_container data_;
	private: mutable value_type ewma_;
	private: mutable bool init_;
	private: bool ext_;
}; // welsh2003_ewma_quantile_estimator

}} // Namespace dcs::testbed

#endif // DCS_TESTBED_DATA_ESTIMATORS_HPP
