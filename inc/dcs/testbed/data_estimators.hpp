/**
 * \file dcs/testbed/data_estimators.hpp
 *
 * \brief Classes to estimate data.
 *
 * \author Marco Guazzone (marco.guazzone@gmail.com)
 *
 * <hr/>
 *
 * Copyright (C) 2013       Marco Guazzone (marco.guazzone@gmail.com)
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

#ifndef DCS_TESTBED_DATA_ESTIMATORS_HPP
#define DCS_TESTBED_DATA_ESTIMATORS_HPP


#include <algorithm>
#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics/mean.hpp>
#include <boost/accumulators/statistics/p_square_quantile.hpp>
#include <boost/accumulators/statistics/stats.hpp>
#include <cmath>
#include <cstddef>
#include <dcs/debug.hpp>
#include <dcs/macro.hpp>
#include <dcs/math/function/sign.hpp>
#include <dcs/testbed/detail/quantile.hpp>
#include <limits>
#include <list>
#include <vector>


namespace dcs { namespace testbed {

template <typename ValueT>
class base_estimator
{
	public: typedef ValueT value_type;
	protected: typedef ::std::vector<value_type> data_container;


	public: base_estimator()
	: n_(0)
	{
	}

	public: void collect(value_type val)
	{
		this->do_collect(::std::vector<value_type>(1, val));

		++n_;
	}

	public: template <typename IterT>
			void collect(IterT first, IterT last)
	{
		const ::std::vector<value_type> data(first, last);

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
//				   DCS_EXCEPTION_THROW(::std::logic_error,
//									   "Number of collected observations greater than zero after reset"));

		n_ = 0;
	}

	// Return the number of collected observations after the last reset
	public: ::std::size_t count() const
	{
//		return this->do_count();
		return n_;
	}

	private: virtual void do_collect(data_container const& data) = 0; 
	private: virtual value_type do_estimate() const = 0;
	private: virtual void do_reset() = 0;
//	private: virtual ::std::size_t do_count() const = 0;


	private: ::std::size_t n_;
};

template <typename ValueT>
class most_recently_observed_estimator: public base_estimator<ValueT>
{
	private: typedef base_estimator<ValueT> base_type;
	public: typedef typename base_type::value_type value_type;
	private: typedef typename base_type::data_container data_container;
	private: typedef ::boost::accumulators::accumulator_set<value_type,
															::boost::accumulators::stats< ::boost::accumulators::tag::mean > > accumulator_type;


	public: most_recently_observed_estimator()
	: mro_(::std::numeric_limits<value_type>::quiet_NaN())
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
		mro_ = ::std::numeric_limits<value_type>::quiet_NaN();
	}

//	private: ::std::size_t do_count() const
//	{
//		return ::boost::accumulators::count(acc_);
//	}


	private: value_type mro_;
}; // most_recently_observed_estimator

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

//	private: ::std::size_t do_count() const
//	{
//		return ::boost::accumulators::count(acc_);
//	}


	private: accumulator_type acc_;
}; // mean_estimator


/**
 * The P^2 Algorithm for incremental quantile estimation.
 *
 * From:
 *  Raj Jain and Imrich Chlamtac,
 *  "The P^2 Algorithm for Dynamic Calculation of Quantiles and Histograms Without Storing Observations"
 *  Communications of the ACM, 28(10):1076-1085 1985
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

//	private: ::std::size_t do_count() const
//	{
//		return ::boost::accumulators::count(acc_);
//	}


	private: value_type prob_;
	private: accumulator_type acc_;
}; // jain1985_p2_algorithm_quantile_estimator


// Method found at https://stackoverflow.com/a/2144754
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
	  m_(::std::numeric_limits<value_type>::quiet_NaN()),
	  q_(::std::numeric_limits<value_type>::quiet_NaN()),
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
		const ::std::size_t n = data_.size();

		if (n > 0)
		{
			if (init_)
			{
				const value_type p[2] = {prob_, 0.5};

				const ::std::vector<value_type> q = detail::quantile<value_type>(data_.begin(), data_.end(), p, p+2, false);
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

				for (::std::size_t i = 0; i < n; ++i)
				{
					++k_;
					m_ += w_*::dcs::math::sign(data_[i]-m_);

					// A generic value like 0.001 simply doesn't make any sense.
					// A seemingly-better approach is to set eta from a running estimate of the absolute deviation
					cumadev_ += ::std::abs(data_[i]-m_);
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
		m_ = q_ = ::std::numeric_limits<value_type>::quiet_NaN();
		cumadev_ = 0;
	}

//	private: ::std::size_t do_count() const
//	{
//		return ::boost::accumulators::count(acc_);
//	}


	private: value_type prob_;
	private: value_type w_;
	private: mutable data_container data_;
	private: mutable ::std::size_t k_;
	private: mutable value_type cumadev_;
	private: mutable value_type m_;
	private: mutable value_type q_;
	private: mutable bool init_;
}; // recursive_quantile_estimator


/**
 * The EWMA-based incremental quantile estimation method from (Welsh,2003).
 *
 * The extended version extends the original one, by computing the quantile with a linear interpolation.
 *
 * From:
 *  Matt Welsh and David Culler,
 *  "Adaptive Overload Control for Busy Internet Servers",
 *  In Proc. of the 4th Conference on USENIX Symposium on Internet Technologies and Systems (USITS'03), 2003
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
		const ::std::size_t m = data_.size();

		if (m > 0)
		{
			value_type q = 0;

			if (ext_)
			{
				q = detail::quantile(data_.begin(), data_.end(), prob_, false);
			}
			else
			{
				::std::size_t np = ::std::min(static_cast< ::std::size_t >(::std::ceil(prob_*data_.size())), data_.size());
				//::std::partial_sort(data_.begin(), data_.begin()+np, data_.end());
				::std::sort(data_.begin(), data_.end());
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

//	private: ::std::size_t do_count() const
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


/**
 * The EWMA-based incremental quantile estimation method from (Chen,2000).
 *
 * From:
 *   Fei Chen and Diane Lambert and José C. Pinheiro
 *   "Incremental Quantile Estimation for Massive Tracking",
 *   In Proc. of the 6th ACM SIGKDD International Conference on Knowledge Discovery and Data Mining (KDD'00), 2000
 */
template <typename ValueT>
class chen2000_ewma_quantile_estimator: public base_estimator<ValueT>
{
	private: typedef base_estimator<ValueT> base_type;
	public: typedef typename base_type::value_type value_type;
	private: typedef typename base_type::data_container data_container;


	public: explicit chen2000_ewma_quantile_estimator(value_type prob, value_type w = 0.05)
	: prob_(prob),
	  w_(w),
	  data_(),
	  ewma_(0),
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
		const ::std::size_t m = data_.size();

		if (m > 0)
		{
			const value_type q = detail::quantile(data_.begin(), data_.end(), prob_, false);

			if (init_)
			{
				ewma_ = q;
				init_ = false;
			}
			else
			{
				ewma_ = (1-w_)*ewma_+w_*q;
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

//	private: ::std::size_t do_count() const
//	{
//		return data_.size();
//	}


	private: value_type prob_;
	private: value_type w_;
	private: mutable data_container data_;
	private: mutable value_type ewma_;
	private: mutable bool init_;
}; // chen2000_ewma_quantile_estimator


/**
 * The SA-based incremental quantile estimation method from (Chen,2000).
 *
 * From:
 *   Fei Chen and Diane Lambert and José C. Pinheiro
 *   "Incremental Quantile Estimation for Massive Tracking",
 *   In Proc. of the 6th ACM SIGKDD International Conference on Knowledge Discovery and Data Mining (KDD'00), 2000
 */
template <typename ValueT>
class chen2000_sa_quantile_estimator: public base_estimator<ValueT>
{
	private: typedef base_estimator<ValueT> base_type;
	public: typedef typename base_type::value_type value_type;
	private: typedef typename base_type::data_container data_container;


	public: explicit chen2000_sa_quantile_estimator(value_type prob)
	: prob_(prob),
	  sn_(::std::numeric_limits<value_type>::quiet_NaN()),
	  fn_(::std::numeric_limits<value_type>::quiet_NaN()),
	  f0_(::std::numeric_limits<value_type>::quiet_NaN()),
	  n_(0),
	  data_(),
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
		const ::std::size_t m = data_.size();

		if (m > 0)
		{
			if (init_)
			{
				// FIXME: the initialization step is not clear in the Chen's paper, so we propose an our one
				// - f0_ and fn_ are initialized to the IQR if m > 1, or to 1 otherwise
				// - sn_ is initialized with the true sample quantile of the first sample

				if (m > 1)
				{
					const value_type p25_75[] = {0.25, 0.75};
					const ::std::vector<value_type> q25_75 = detail::quantile<value_type>(data_.begin(), data_.end(), p25_75, p25_75+2, false);

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
			}
			else
			{
				++n_;

				const value_type wn = 1.0/n_;
				const value_type cn = ::std::sqrt(wn);

				::std::size_t cncnt = 0;
				for (::std::size_t i = 0; i < m; ++i)
				{
					if (::std::abs(data_[i]- sn_) <= cn)
					{
						++cncnt;
					}
				}
				fn_ = (1-wn)*fn_ + wn*cncnt/(2.0*cn*m);

				const value_type en = ::std::max(fn_,f0_*cn);

				::std::size_t sncnt = 0;
				for (::std::size_t i = 0; i < m; ++i)
				{
					if (data_[i] <= sn_)
					{
						++sncnt;
					}
				}
				sn_ += (wn/en)*(prob_-sncnt/static_cast<value_type>(m));
			}
			data_.clear();
		}

		return sn_;
	}

	private: void do_reset()
	{
		init_ = true;
		data_.clear();
		sn_ = fn_ = f0_ = ::std::numeric_limits<value_type>::quiet_NaN();
	}

//	private: ::std::size_t do_count() const
//	{
//		return data_.size();
//	}


	private: value_type prob_;
	private: mutable value_type sn_;
	private: mutable value_type fn_;
	private: mutable value_type f0_;
	private: mutable ::std::size_t n_;
	private: mutable data_container data_;
	private: mutable bool init_;
}; // chen2000_sa_quantile_estimator


/**
 * The EWSA-based incremental quantile estimation method from (Chen,2000).
 *
 * From:
 *  Fei Chen and Diane Lambert and José C. Pinheiro
 *  "Incremental Quantile Estimation for Massive Tracking",
 *  In Proc. of the 6th ACM SIGKDD International Conference on Knowledge Discovery and Data Mining (KDD'00), 2000
 */
template <typename ValueT>
class chen2000_ewsa_quantile_estimator: public base_estimator<ValueT>
{
	private: typedef base_estimator<ValueT> base_type;
	public: typedef typename base_type::value_type value_type;
	private: typedef typename base_type::data_container data_container;


	public: explicit chen2000_ewsa_quantile_estimator(value_type prob, value_type w = 0.05)
	: prob_(prob),
	  w_(w),
	  data_(),
	  sn_(::std::numeric_limits<value_type>::quiet_NaN()),
	  fn_(::std::numeric_limits<value_type>::quiet_NaN()),
	  rn_(::std::numeric_limits<value_type>::quiet_NaN()),
	  cn_(::std::numeric_limits<value_type>::quiet_NaN()),
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
		}
	}

	private: value_type do_estimate() const
	{
		const ::std::size_t m = data_.size();

		if (m > 0)
		{
			const value_type p25_75[] = {0.25, 0.75};
			const ::std::vector<value_type> q25_75 = detail::quantile<value_type>(data_.begin(), data_.end(), p25_75, p25_75+2, false);

			if (init_)
			{
				// Set the initial estimate S_0^* equal to the q^\text{th} sample quantile
				// \hat{Q}_n of X_{01},\ldots,X_{0M}
				sn_ = detail::quantile(data_.begin(), data_.end(), prob_, false);
				// Estimate the scale r_0^* of f_0^* by the interquantile range of
				// X_{01},\ldots,X_{0M}; i.e., by the difference of the .75 and .25 sample
				// quantiles
				rn_ = q25_75[1]-q25_75[0];
				// Then take c_0^* = r_0^* M^{-1} \sum_{i=1}^M i^{-1/2}
				value_type c = 1;
				for (::std::size_t i = 2; i <= m; ++i)
				{
					c += 1.0/::std::sqrt(i);
				}
				// FIXME: unlike the Chen's paper, we deal with the case of rn_ ~= zero
				if (rn_ > 0)
				{
					cn_ = rn_*c/m;
				}
				else
				{
					cn_ = c/m;
				}
				// Take f_0^* = (2 c_0^* M)^{-1} \max\{\#\{|X_{0i}-S_0^*| \le c_0^*\},1\}
				// which is the density of observations in a neighborhood of width 2c_0^* of
				// S_0^*, unless the fraction of neighborhood is zero
				::std::size_t cnt = 0;
				for (::std::size_t i = 0; i < m; ++i)
				{
					if (::std::abs(data_[i]-sn_) <= cn_)
					{
						++cnt;
					}
				}
				fn_ = 1.0/(2.0*cn_*m)*::std::max(cnt,::std::size_t(1));
				init_ = false;
			}
			else
			{
				// S_n^* = S_{n-1}^*+\frac{w}{f_{n-1}^*}(p-\frac{\#\{X_{ni} \le S_{n-1}^*\}}{M})
				// f_n^* = (1-w)f_{n-1}^*+\frac{w}{2c_{n-1}^*M}\#\{|X_{ni}-S_{n-1}^*| \le c_{n-1}^*\}
				::std::size_t scnt = 0;
				::std::size_t fcnt = 0;
				for (::std::size_t i = 0; i < m; ++i)
				{
					if (data_[i] <= sn_)
					{
						++scnt;
					}
					if (::std::abs(data_[i]-sn_) <= cn_)
					{
						++fcnt;
					}
				}
				sn_ += (w_/fn_)*(prob_-scnt/static_cast<value_type>(m));
				fn_ = (1-w_)*fn_ + (w_/(2.0*cn_*m))*fcnt;
				// Take r_n^∗ to be the difference of the current EWSA estimates for the
				// .75 and .25 quantiles, and define the neighborhood size for the next
				// updating step to be c_n^* = r_n^∗ c, with c = M^{-1} \sum_{i=M+1}^{2M} i^{-1/2}.
				rn_ = q25_75[1]-q25_75[0];
				value_type c = 0;
				const ::std::size_t m2 = 2*m;
				for (::std::size_t i = m+1; i <= m2; ++i)
				{
					c += 1.0/::std::sqrt(i);
				}
				c /= m;
				// FIXME: unlike the Chen's paper, we handle the case of rn_ ~= zero
				if (rn_ > 0)
				{
					cn_ = rn_*c;
				}
				else
				{
					cn_ = c;
				}
			}
			data_.clear();
		}

		return sn_;
	}

	private: void do_reset()
	{
		init_ = true;
		data_.clear();
		sn_ = fn_ = rn_ = cn_ = ::std::numeric_limits<value_type>::quiet_NaN();
	}

//	private: ::std::size_t do_count() const
//	{
//		return data_.size();
//	}


	private: value_type prob_;
	private: value_type w_;
	private: mutable data_container data_;
	private: mutable value_type sn_;
	private: mutable value_type fn_;
	private: mutable value_type rn_;
	private: mutable value_type cn_;
	private: mutable bool init_;
}; // chen2000_ewsa_quantile_estimator


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
	}

	private: value_type do_estimate() const
	{
		return detail::quantile(data_.begin(), data_.end(), prob_, true, type_);
	}

	private: void do_reset()
	{
		data_.clear();
	}


	private: value_type prob_;
	private: detail::quantile_category type_;
	private: ::std::list<value_type> data_;
}; // true_quantile_estimator

}} // Namespace dcs::testbed

#endif // DCS_TESTBED_DATA_ESTIMATORS_HPP
