#ifndef DCS_TESTBED_SINUSOIDAL_SIGNAL_GENERATOR_HPP
#define DCS_TESTBED_SINUSOIDAL_SIGNAL_GENERATOR_HPP


#include <boost/numeric/ublas/operation/size.hpp>
#include <boost/numeric/ublas/vector.hpp>
#include <dcs/testbed/base_signal_generator.hpp>
#include <cmath>
#include <cstddef>
#include <stdexcept>


namespace dcs { namespace testbed {

/**
 * \brief Generate a sinusoidal wave according to the sample-based mode.
 *
 * Sample-based mode uses the following formula to compute the output of the sine wave:
 * \f[
 *   y = A\sin(2\pi(k+o)/p) + b
 * \f]
 * where
 * - A is the amplitude of the sine wave (i.e., the peak deviation of the sine
 *   function from its center position).
 * - p is the number of time samples per sine wave period.
 * - k is a repeating integer value that ranges from 0 to p–1.
 * - o is the offset (phase shift) of the signal in number of sample times.
 * - b is the signal bias (i.e., constant value added to the sine to produce the
 *   output).
 * .
 *
 * See:
 * http://www.mathworks.com/help/toolbox/simulink/slref/sinewave.html
 */

namespace detail { namespace /*<unnamed>*/ {

template <typename ValueT>
class base_sinusoidal_signal_generator: public base_signal_generator<ValueT>
{
	private: typedef base_signal_generator<ValueT> base_type;
	public: typedef ::std::size_t size_type;
	public: typedef ValueT value_type;
	public: typedef typename base_type::vector_type vector_type;
	public: typedef ::boost::numeric::ublas::vector<size_type> size_vector_type;


	protected: static const value_type double_pi_;


	public: base_sinusoidal_signal_generator(vector_type const& a, size_vector_type const& p)
	: a_(a),
	  p_(p),
	  o_(::boost::numeric::ublas::zero_vector<size_type>(::boost::numeric::ublas::size(a))),
	  b_(::boost::numeric::ublas::zero_vector<value_type>(::boost::numeric::ublas::size(a))),
	  k_(::boost::numeric::ublas::zero_vector<size_type>(::boost::numeric::ublas::size(a)))
	{
		namespace ublas = ::boost::numeric::ublas;

		// pre: size(a) == size(p)
		if (ublas::size(a_) != ublas::size(p_))
		{
			throw ::std::invalid_argument("[sinusoidal_signal_generator::ctor] Invalid vector size.");
		}
	}

	public: base_sinusoidal_signal_generator(vector_type const& a, size_vector_type const& p, size_vector_type const& o, vector_type const& b)
	: a_(a),
	  p_(p),
	  o_(o),
	  b_(b),
	  k_(::boost::numeric::ublas::zero_vector<size_type>(::boost::numeric::ublas::size(a)))
	{
		namespace ublas = ::boost::numeric::ublas;

		// pre: size(a) == size(p)
		if (ublas::size(a_) != ublas::size(p_))
		{
			throw ::std::invalid_argument("[sinusoidal_signal_generator::ctor] Invalid vector size between 'a' and 'p'.");
		}
		// pre: size(a) == size(o)
		if (ublas::size(a_) != ublas::size(o_))
		{
			throw ::std::invalid_argument("[sinusoidal_signal_generator::ctor] Invalid vector size between 'a' and 'o'.");
		}
		// pre: size(a) == size(b)
		if (ublas::size(a_) != ublas::size(b_))
		{
			throw ::std::invalid_argument("[sinusoidal_signal_generator::ctor] Invalid vector size between 'a' and 'b'.");
		}
	}

	public: void offset(size_vector_type o)
	{
		namespace ublas = ::boost::numeric::ublas;

		// pre: size(o) == size(a_)
		if (ublas::size(o) == ublas::size(a_))
		{
			throw ::std::invalid_argument("[sinusoidal_signal_generator::offset] Invalid vector size.");
		}

		o_ = o;
	}

	public: void bias(vector_type b)
	{
		namespace ublas = ::boost::numeric::ublas;

		// pre: size(b) == size(a_)
		if (ublas::size(b) == ublas::size(a_))
		{
			throw ::std::invalid_argument("[sinusoidal_signal_generator::bias] Invalid vector size.");
		}

		b_ = b;
	}

	private: vector_type do_generate()
	{
		namespace ublas = ::boost::numeric::ublas;

		::std::size_t n(ublas::size(a_));
		vector_type u(n);

		for (::std::size_t i = 0; i < n; ++i)
		{
			u(i) = a_(i)*::std::sin(double_pi_*(k_(i)+o_(i))/p_(i))+b_(i);

//          DCS_DEBUG_TRACE("Generated: k(" << i << ")=" << k_(i) << " ==> u(" << i << ")=" << u(i));//XXX

			k_(i) += 1;
		}

		return u;
	}


	protected: vector_type a_; ///< The amplitude (the peak deviation of the function from its center position).
	protected: size_vector_type p_; ///< The phase (specifies where in its cycle the oscillation begins at t = 0).
	protected: size_vector_type o_; ///< The DC offset (a non-zero center amplitude).
	protected: vector_type b_;
	protected: vector_type k_;
}; // base_sinusoidal_signal_generator

template <typename VT>
const VT base_sinusoidal_signal_generator<VT>::double_pi_ = static_cast<VT>(3.1415926535897932384626433832795029L);

}} // Namespace detail::<unnamed>


template <typename ValueT>
class sinusoidal_signal_generator: public detail::base_sinusoidal_signal_generator<ValueT>
{
	private: typedef detail::base_sinusoidal_signal_generator<ValueT> base_type;
	public: typedef ::std::size_t size_type;
	public: typedef ValueT value_type;
	public: typedef typename base_type::vector_type vector_type;
	public: typedef ::boost::numeric::ublas::vector<size_type> size_vector_type;


	public: sinusoidal_signal_generator(vector_type const& a, size_vector_type const& p)
	: base_type(a,p)
	{
	}

	public: sinusoidal_signal_generator(vector_type const& a, size_vector_type const& p, size_vector_type const& o, vector_type const& b)
	: base_type(a,p,o,b)
	{
	}

	private: vector_type do_generate()
	{
		namespace ublas = ::boost::numeric::ublas;

		::std::size_t n(ublas::size(this->a_));
		vector_type u(n);

		for (::std::size_t i = 0; i < n; ++i)
		{
			u(i) = this->a_(i)*::std::sin(base_type::double_pi_*(this->k_(i)+this->o_(i))/this->p_(i))+this->b_(i);

//          DCS_DEBUG_TRACE("Generated: k(" << i << ")=" << k_(i) << " ==> u(" << i << ")=" << u(i));//XXX

			this->k_(i) += 1;
		}

		return u;
	}

	private: void do_reset()
	{
		namespace ublas = ::boost::numeric::ublas;

		this->k_ = ublas::zero_vector<size_type>(ublas::size(this->a_));
	}
}; // sinusoidal_signal_generator

template <typename ValueT>
class sinusoidal_mesh_signal_generator: public detail::base_sinusoidal_signal_generator<ValueT>
{
	private: typedef detail::base_sinusoidal_signal_generator<ValueT> base_type;
	public: typedef ::std::size_t size_type;
	public: typedef ValueT value_type;
	public: typedef typename base_type::vector_type vector_type;
	public: typedef ::boost::numeric::ublas::vector<size_type> size_vector_type;


	public: sinusoidal_mesh_signal_generator(vector_type const& a, size_vector_type const& p)
	: base_type(a,p),
	  c_(0)
	{
	}

	public: sinusoidal_mesh_signal_generator(vector_type const& a, size_vector_type const& p, size_vector_type const& o, vector_type const& b)
	: base_type(a,p,o,b),
	  c_(0)
	{
	}

	private: vector_type do_generate()
	{
		namespace ublas = ::boost::numeric::ublas;

		::std::size_t n(ublas::size(this->a_));
		vector_type u(n);

		if (this->k_(c_) == this->p_(c_))
		{
			this->k_(c_) = 0;
			c_ = (c_+1) % n;
		}
		for (::std::size_t i = 0; i < n; ++i)
		{
			u(i) = this->a_(i)*::std::sin(base_type::double_pi_*(this->k_(i)+this->o_(i))/this->p_(i))+this->b_(i);

			if (i == c_)
			{
				this->k_(i) += 1;
			}
		}

		return u;
	}

	private: void do_reset()
	{
		namespace ublas = ::boost::numeric::ublas;

		this->k_ = ublas::zero_vector<size_type>(ublas::size(this->a_));
		c_ = 0;
	}


	private: size_type c_;
}; // sinusoidal_mesh_signal_generator

}} // Namespace dcs::testbed


#endif // DCS_TESTBED_SINUSOIDAL_SIGNAL_GENERATOR_HPP
