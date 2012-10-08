#ifndef DCS_TESTBED_CONSTANT_SIGNAL_GENERATOR_HPP
#define DCS_TESTBED_CONSTANT_SIGNAL_GENERATOR_HPP


#include <dcs/testbed/base_signal_generator.hpp>


namespace dcs { namespace testbed {

template <typename ValueT>
class constant_signal_generator: public base_signal_generator<ValueT>
{
	private: typedef base_signal_generator<ValueT> base_type;
	public: typedef ValueT value_type;
	public: typedef typename base_type::vector_type vector_type;


	public: constant_signal_generator(vector_type const& u0)
	: u_(u0)
	{
	}

	private: vector_type do_generate()
	{
		return u_;
	}

	private: void do_reset()
	{
		// do nothing: the signal is constant.
	}


	private: vector_type u_;
};

}} // Namespace dcs::testbed


#endif // DCS_TESTBED_CONSTANT_SIGNAL_GENERATOR_HPP
