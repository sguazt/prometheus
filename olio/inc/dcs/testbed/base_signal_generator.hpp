#ifndef DCS_TESTBED_BASE_SIGNAL_GENERATOR_HPP
#define DCS_TESTBED_BASE_SIGNAL_GENERATOR_HPP


#include <boost/numeric/ublas/vector.hpp>


namespace dcs { namespace testbed {

template <typename ValueT>
class base_signal_generator
{
	public: typedef ValueT value_type;
	public: typedef ::boost::numeric::ublas::vector<value_type> vector_type;


	public: vector_type operator()()
	{
		return do_generate();
	}

	public: void reset()
	{
		do_reset();
	}

	public: virtual ~base_signal_generator()
	{
		// empty
	}

	private: virtual vector_type do_generate() = 0;

	private: virtual void do_reset() = 0;
}; // base_signal_generator

}} // Namespace dcs::testbed


#endif // DCS_TESTBED_SIGNAL_GENERATOR_HPP
