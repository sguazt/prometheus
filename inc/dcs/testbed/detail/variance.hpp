#ifndef DCS_TESTBED_DETAIL_VARIANCE_HPP
#define DCS_TESTBED_DETAIL_VARIANCE_HPP


#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics.hpp>
#include <cmath>
#include <cstddef>


namespace dcs { namespace testbed { namespace detail {

// See: https://en.wikipedia.org/wiki/Algorithms_for_calculating_variance
template <typename ValueT, typename IterT>
ValueT compensated_variance(IterT first, IterT last)
{
    std::size_t n = 0;
    ValueT mean = 0;

	for (IterT it = first; it != last; ++it)
	{
		const ValueT x = *it;
		++n;
		mean += x;
	}
	mean /= n;

    ValueT sum1 = 0;
    ValueT sum2 = 0;
	for (IterT it = first; it != last; ++it)
	{
		const ValueT x = *it;
		const ValueT dev = x-mean;

        sum1 += dev*dev;
        sum2 += dev;
	}
    return (sum1 - sum2*sum2/n)/(n - 1);
}

template <typename ValueT, typename IterT>
ValueT boost_variance(IterT first, IterT last)
{
	boost::accumulators::accumulator_set<ValueT, boost::accumulators::stats<boost::accumulators::tag::variance> > acc;
	std::size_t n = 0;

	while (first != last)
	{
		acc(*first);
		++first;
		++n;
	}

	return boost::accumulators::variance(acc)*n/static_cast<ValueT>(n-1);
}

template <typename ValueT, typename IterT>
ValueT variance(IterT first, IterT last)
{
	return boost_variance<ValueT>(first, last);
}

template <typename ValueT, typename IterT>
ValueT stdev(IterT first, IterT last)
{
	return std::sqrt(variance<ValueT>(first, last));
}

}}} // Namespace dcs::testbed::detail


#endif // DCS_TESTBED_DETAIL_VARIANCE_HPP
