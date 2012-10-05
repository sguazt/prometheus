#ifndef DCS_TESTBED_SAWTOOTH_SIGNAL_GENERATOR_HPP
#define DCS_TESTBED_SAWTOOTH_SIGNAL_GENERATOR_HPP


#include <boost/numeric/ublas/operation/size.hpp>
#include <dcs/testbed/base_signal_generator.hpp>
#include <cstddef>


namespace dcs { namespace testbed {

template <typename ValueT>
class sawtooth_signal_generator: public base_signal_generator<ValueT>
{
	private: typedef base_signal_generator<ValueT> base_type;
	public: typedef ValueT value_type;
	public: typedef typename base_type::vector_type vector_type;


	public: sawtooth_signal_generator(vector_type const& ul, vector_type const& uh, vector_type const& incr)
	: ul_(ul),
	  uh_(uh),
	  u_(ul),
	  h_(incr)
	{
	}

	private: vector_type do_generate()
	{
		::std::size_t n(::boost::numeric::ublas::size(u_));
		for (::std::size_t i = 0; i < n; ++i)
		{
			u_(i) += h_(i);
			if (u_(i) > uh_(i))
			{
				u_(i) = ul_(i);
			}
		}

		return u_;
	}

	private: void do_reset()
	{
		u_ = ul_;
	}


	private: vector_type ul_; ///< Lower bounds.
	private: vector_type uh_; ///< Upper bounds.
	private: vector_type u_;
	private: vector_type h_;
};

}} // Namespace dcs::testbed


#endif // DCS_TESTBED_SAWTOOTH_SIGNAL_GENERATOR_HPP
