/**
 * \file dcs/testbed/detail/quantile.hpp
 *
 * \brief Sample quantile.
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

#ifndef DCS_TESTBED_DETAIL_QUANTILE_HPP
#define DCS_TESTBED_DETAIL_QUANTILE_HPP


#include <algorithm>
#include <cmath>
#include <cstddef>
#include <iterator>
#include <limits>
#include <stdexcept>
#include <vector>


namespace dcs { namespace testbed { namespace detail {

enum quantile_category
{
    type1_quantile,
    type2_quantile,
    type3_quantile,
    type4_quantile,
    type5_quantile,
    type6_quantile,
    type7_quantile,
    type8_quantile,
    type9_quantile
};

template <typename T, typename DataIterT, typename ProbIterT>
inline
::std::vector<T> quantile(DataIterT data_first, DataIterT data_last, ProbIterT prob_first, ProbIterT prob_last, bool sorted = false, quantile_category cat = type7_quantile)
{
	const ::std::vector<T> probs(prob_first, prob_last);
	const ::std::size_t np = probs.size();

	::std::vector<T> qs(np, ::std::numeric_limits<T>::quiet_NaN());

	if (data_first == data_last)
	{
		return qs;
	}

	if (!sorted)
	{
		std::vector<T> data(data_first, data_last);
		std::sort(data.begin(), data.end());

		return quantile<T>(data.begin(), data.end(), prob_first, prob_last, true, cat);
	}
	else
	{
		switch (cat)
		{
			case type7_quantile:
				{
					const ::std::size_t n = ::std::distance(data_first, data_last);
					::std::vector<T> index(np);
					::std::vector< ::std::size_t > lo(np);
					::std::vector< ::std::size_t > hi(np);
					::std::vector< ::std::size_t > ix;
					DataIterT it = data_first;
					for (::std::size_t i = 0; i < np; ++i)
					{
						index[i] = (n-1)*probs[i];
						lo[i] = static_cast< ::std::size_t >(::std::floor(index[i]));
						hi[i] = static_cast< ::std::size_t >(::std::ceil(index[i]));
						if (index[i] > lo[i])
						{
							ix.push_back(i);
						}
						if (i > 0)
						{
							::std::advance(it, lo[i]-lo[i-1]);
						}
						else
						{
							::std::advance(it, lo[i]);
						}
						qs[i] = *it;
					}
					const ::std::size_t nh = ix.size();
					::std::vector<T> h(nh);
					it = data_first;
					for (::std::size_t i = 0; i < nh; ++i)
					{
						h[i] = index[ix[i]]-lo[ix[i]];
						if (i > 0)
						{
							::std::advance(it, hi[i]);
						}
						else
						{
							::std::advance(it, hi[i]-hi[i-1]);
						}
						qs[i] = (1-h[i])*qs[i] + h[i]*(*it);
					}
				}
				break;
			default:
				throw ::std::runtime_error("Quantile estimators other than type-7 are to be implemented");
		}
	}
#if 0
	else
	{
		::std::vector<T> data(data_first, data_last);
		const ::std::size_t n = data.size();

		switch (cat)
		{
			case type7_quantile:
				{
					::std::vector<T> index(np);
					::std::vector< ::std::size_t > lo(np);
					::std::vector< ::std::size_t > hi(np);
					::std::vector< ::std::size_t > ix;
//					::std::size_t max_hi = 0;
					for (::std::size_t i = 0; i < np; ++i)
					{
						index[i] = (n-1)*probs[i];
						lo[i] = static_cast< ::std::size_t >(::std::floor(index[i]));
						hi[i] = static_cast< ::std::size_t >(::std::ceil(index[i]));
						if (index[i] > lo[i])
						{
							ix.push_back(i);
						}
//						if (hi[i] > max_hi)
//						{
//							max_hi = hi[i];
//						}
					}
					//::std::partial_sort(data.begin(), data.begin()+max_hi, data.end());//FIXME: doesn't work
					::std::sort(data.begin(), data.end());
					for (::std::size_t i = 0; i < np; ++i)
					{
						qs[i] = data[lo[i]];
					}
					const ::std::size_t nh = ix.size();
					::std::vector<T> h(nh);
					for (::std::size_t i = 0; i < nh; ++i)
					{
						h[i] = index[ix[i]]-lo[ix[i]];
						qs[i] = (1-h[i])*qs[i] + h[i]*data[hi[i]];
					}
				}
				break;
			default:
/*TODO
			if (cat == type1_quantile || cat == type2_quantile || cat == type3_quantile)
			{
				// Types 1, 2 and 3 are discontinuous sample quantiles
				T nppm = (cat == type3_quantile) ? (n*prob-.5) : (n*prob);
				::std::size_t j = static_cast< ::std::size_t >(::std::floor(nppm));
				::std::size_t h;
				switch (cat)
				{
					case type1_quantile:
						h = nppm > j;
						break;
					case type2_quantile:
						h = ((nppm > j)+1)/2;
						break;
					case type3_quantile:
						h = (nppm != j) | ((j % 2) == 1);
						break;
					default:
						break;
				}
			}
			else
			{
				// Types 4 through 9 are continuous sample quantiles
				T a;
				T b;

				switch (cat)
				{
					case type4_quantile:
						a = 0;
						b = 1;
						break;
					case type5_quantile:
						a = b = 0.5;
						break;
					case type6_quantile:
						a = b = 0;
						break;
					case type7_quantile:
						a = b = 1;
						break;
					case type8_quantile:
						a = b = 1/3.0;
						break;
					case type9_quantile:
						a = b = 3/8.0;
						break;
					default:
						break;
				}
				// need to watch for rounding errors here
				T fuzz = 4.0*::std::numeric_limits<T>::epsilon();
				nppm = a+prob*(n+1-a-b);
				j = ::std::floor(nppm+fuzz);
				h = nppm-j;
				if (::std::abs(h) < fuzz)
				{
					h = 0;
				}
			}
			::std::partial_sort(data.begin(), data.end());
			::std::vector<T> x;
			x.push_back(data.front());
			x.push_back(data.front());
			x.insert(x.end(), data.begin(), data.end());
			x.push_back(data.back());
			q = x[j+2];
			if (h == 1)
			{
				q = x[j+3];
			}
			other = (0<h) & (h<1);
			if (other)
			{
				q = ((1-h)*x[j+2] + h*x[j+3]);
			}
*/
			throw ::std::runtime_error("Quantile estimators other than type-7 are to be implemented");
		}
	}
#endif

    return qs;
}

template <typename T, typename IterT>
inline
T quantile(IterT first, IterT last, T prob, bool sorted = false, quantile_category cat = type7_quantile)
{
	T q = ::std::numeric_limits<T>::quiet_NaN();

	if (first == last)
	{
		return q;
	}

	if (sorted)
	{
		switch (cat)
		{
			case type7_quantile:
				{
					const ::std::size_t n = ::std::distance(first, last);
					const T index = (n-1)*prob;
					const ::std::size_t lo = static_cast< ::std::size_t >(::std::floor(index));
					const ::std::size_t hi = static_cast< ::std::size_t >(::std::ceil(index));
					const T h = index-lo;

					IterT it_lo = first;
					::std::advance(it_lo, lo);
					IterT it_hi = first;
					::std::advance(it_hi, hi);
//DCS_DEBUG_TRACE("INDEX: " << index);
//DCS_DEBUG_TRACE("LO: " << lo);
//DCS_DEBUG_TRACE("HI: " << hi);
//DCS_DEBUG_TRACE("X[LO]: " << *it_lo);
//DCS_DEBUG_TRACE("H: " << h);
//DCS_DEBUG_TRACE("X[HI]: " << *it_hi);

					q = (1-h)*(*it_lo) + h*(*it_hi);
				}
				break;
			default:
				throw ::std::runtime_error("Quantile estimators other than type-7 are to be implemented");
		}
	}
	else
	{
		::std::vector<T> data(first, last);
		const ::std::size_t n = data.size();

		switch (cat)
		{
			case type7_quantile:
				{
/*
					T index = 1+(n-1)*prob;
					::std::size_t lo = static_cast< ::std::size_t >(::std::floor(index));
					::std::size_t hi = static_cast< ::std::size_t >(::std::ceil(index));
					::std::partial_sort(data.begin(), data.begin()+hi, data.end());
					//::std::sort(data.begin(), data.end());
					q = data[lo-1];
					T h = index-lo; // h > 0, by construction
					q = (1-h)*q+h*data[hi-1];
*/
					const T index = (n-1)*prob;
					const ::std::size_t lo = static_cast< ::std::size_t >(::std::floor(index));
					const ::std::size_t hi = static_cast< ::std::size_t >(::std::ceil(index));
					const T h = index-lo; // h > 0, by construction
					//::std::partial_sort(data.begin(), data.begin()+hi, data.end());//FIXME: doesn't work
					::std::sort(data.begin(), data.end());
//DCS_DEBUG_TRACE("INDEX: " << index);
//DCS_DEBUG_TRACE("LO: " << lo);
//DCS_DEBUG_TRACE("HI: " << hi);
//DCS_DEBUG_TRACE("X[LO]: " << data[lo]);
//DCS_DEBUG_TRACE("H: " << h);
//DCS_DEBUG_TRACE("X[HI]: " << data[hi]);
					q = (1-h)*data[lo]+h*data[hi];
				}
				break;
			default:
/*TODO
			if (cat == type1_quantile || cat == type2_quantile || cat == type3_quantile)
			{
				// Types 1, 2 and 3 are discontinuous sample quantiles
				T nppm = (cat == type3_quantile) ? (n*prob-.5) : (n*prob);
				::std::size_t j = static_cast< ::std::size_t >(::std::floor(nppm));
				::std::size_t h;
				switch (cat)
				{
					case type1_quantile:
						h = nppm > j;
						break;
					case type2_quantile:
						h = ((nppm > j)+1)/2;
						break;
					case type3_quantile:
						h = (nppm != j) | ((j % 2) == 1);
						break;
					default:
						break;
				}
			}
			else
			{
				// Types 4 through 9 are continuous sample quantiles
				T a;
				T b;

				switch (cat)
				{
					case type4_quantile:
						a = 0;
						b = 1;
						break;
					case type5_quantile:
						a = b = 0.5;
						break;
					case type6_quantile:
						a = b = 0;
						break;
					case type7_quantile:
						a = b = 1;
						break;
					case type8_quantile:
						a = b = 1/3.0;
						break;
					case type9_quantile:
						a = b = 3/8.0;
						break;
					default:
						break;
				}
				// need to watch for rounding errors here
				T fuzz = 4.0*::std::numeric_limits<T>::epsilon();
				nppm = a+prob*(n+1-a-b);
				j = ::std::floor(nppm+fuzz);
				h = nppm-j;
				if (::std::abs(h) < fuzz)
				{
					h = 0;
				}
			}
			::std::partial_sort(data.begin(), data.end());
			::std::vector<T> x;
			x.push_back(data.front());
			x.push_back(data.front());
			x.insert(x.end(), data.begin(), data.end());
			x.push_back(data.back());
			q = x[j+2];
			if (h == 1)
			{
				q = x[j+3];
			}
			other = (0<h) & (h<1);
			if (other)
			{
				q = ((1-h)*x[j+2] + h*x[j+3]);
			}
*/
			throw ::std::runtime_error("Quantile estimators other than type-7 are to be implemented");
		}
	}

    return q;
}

}}} // Namespace dcs::testbed::detail

#endif // DCS_TESTBED_DETAIL_QUANTILE_HPP
