#ifndef DCS_TESTBED_GAUSSIAN_SIGNAL_GENERATOR_HPP
#define DCS_TESTBED_GAUSSIAN_SIGNAL_GENERATOR_HPP


#include <boost/numeric/ublas/operation/size.hpp>
#include <boost/random/normal_distribution.hpp>
#include <cstddef>
#include <dcs/testbed/base_signal_generator.hpp>
#include <stdexcept>
#include <vector>


namespace dcs { namespace testbed {

//FIXME: I need a way to pass a reference to a generic RNG in the constructor. But HOW?!?!?
template <typename ValueT>
class gaussian_signal_generator: public base_signal_generator<ValueT>
{
	private: typedef base_signal_generator<ValueT> base_type;
	public: typedef ValueT value_type;
	public: typedef typename base_type::vector_type vector_type;
	private: typedef ::boost::random::normal_distribution<value_type> normal_distribution_type;
	private: typedef ::std::vector<normal_distribution_type> normal_distribution_container;


	public: gaussian_signal_generator(vector_type const& mu0, vector_type const& sigma0)
	{
		namespace ublas = ::boost::numeric::ublas;

		// pre: size(mu0) == size(sigma0)
		if (ublas::size(mu0) != ublas::size(sigma0))
		{
				throw ::std::invalid_argument("Invalid vector size for Gaussian signal generator");
		}

		::std::size_t n(ublas::size(mu0));
		for (::std::size_t i = 0; i < n; ++i)
		{
			distrs_.push_back(normal_distribution_type(mu0(i), sigma0(i)));
		}
	}

//TODO: Commented until the FIXME above has not been solved
//	private: vector_type do_generate()
//	{
//		random_generator_pointer ptr_rng(::dcs::eesim::registry<traits_type>::instance().uniform_random_generator_ptr());
//
//		::std::size_t n(distrs_.size());
//		vector_type u(n);
//		for (::std::size_t i = 0; i < n; ++i)
//		{
//			u(i) = distrs_[i](*ptr_rng);
//		}
//
//		return u;
//	}

	private: void do_reset()
	{
		// do nothing: the generator is reset by resetting the random number generator, which should be made elsewhere.
	}


	private: normal_distribution_container distrs_;
};

}} // Namespace dcs::testbed


#endif // DCS_TESTBED_GAUSSIAN_SIGNAL_GENERATOR_HPP
