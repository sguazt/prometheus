/**
 * \file dcs/testbed/sinusoidal_signal_generator.hpp
 *
 * \brief Generates sinusoidal signals.
 *
 * Generate a sinusoidal wave according to the sample-based mode.
 * Sample-based mode uses the following formula to compute the output of the sine wave:
 * \f[
 *   y = A\sin(2\pi(k+o)/p) + b
 * \f]
 * where
 * - A is the amplitude of the sine wave (i.e., the peak deviation of the sine
 *   function from its center position).
 * - p is the number of time samples per sine wave period.
 * - k is a repeating integer value that ranges from 0 to p-1.
 * - o is the offset (phase shift) of the signal in number of sample times.
 * - b is the signal bias (i.e., constant value added to the sine to produce the
 *   output).
 * .
 *
 * See:
 * http://www.mathworks.com/help/toolbox/simulink/slref/sinewave.html
 *
 * \author Marco Guazzone (marco.guazzone@gmail.com)
 *
 * <hr/>
 *
 * Copyright 2012 Marco Guazzone (marco.guazzone@gmail.com)
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

#ifndef DCS_TESTBED_SINUSOIDAL_SIGNAL_GENERATOR_HPP
#define DCS_TESTBED_SINUSOIDAL_SIGNAL_GENERATOR_HPP


#include <dcs/assert.hpp>
#include <dcs/exception.hpp>
#include <dcs/testbed/base_signal_generator.hpp>
#include <cmath>
#include <cstddef>
#include <limits>
#include <stdexcept>
#include <vector>


namespace dcs { namespace testbed {

namespace detail { namespace /*<unnamed>*/ {

template <typename ValueT, typename UIntT>
class base_sinusoidal_signal_generator: public base_signal_generator<ValueT>
{
	private: typedef base_signal_generator<ValueT> base_type;
	public: typedef ValueT value_type;
	public: typedef UIntT uint_type;
	public: typedef typename base_type::vector_type vector_type;
	public: typedef ::std::vector<uint_type> uint_vector_type;


	protected: static const value_type double_pi_; ///< The constant \f$2\pi\f$


	public: base_sinusoidal_signal_generator(vector_type const& a, uint_vector_type const& p)
	: a_(a),
	  p_(p),
	  o_(a.size(),0),
	  b_(a.size(),0),
	  k_(a.size(),0),
	  ub_(a.size(), ::std::numeric_limits<value_type>::infinity()),
	  lb_(a.size(),-::std::numeric_limits<value_type>::infinity())
	{
		// pre: size(a) == size(p)
		DCS_ASSERT(a_.size() == p_.size(),
				   DCS_EXCEPTION_THROW(::std::invalid_argument,
									   "Size of input vectors 'a' and 'p' does not match"));
	}

	public: base_sinusoidal_signal_generator(vector_type const& a, uint_vector_type const& p, uint_vector_type const& o, vector_type const& b)
	: a_(a),
	  p_(p),
	  o_(o),
	  b_(b),
	  k_(a.size(),0),
	  ub_(a.size(), ::std::numeric_limits<value_type>::infinity()),
	  lb_(a.size(),-::std::numeric_limits<value_type>::infinity())
	{
		// pre: size(a) == size(p)
		DCS_ASSERT(a_.size() == p_.size(),
				   DCS_EXCEPTION_THROW(::std::invalid_argument,
									   "Size of input vectors 'a' and 'p' does not match"));
		// pre: size(a) == size(o)
		DCS_ASSERT(a_.size() == o_.size(),
				   DCS_EXCEPTION_THROW(::std::invalid_argument,
									   "Size of input vectors 'a' and 'o' does not match"));
		// pre: size(a) == size(b)
		DCS_ASSERT(a_.size() == b_.size(),
				   DCS_EXCEPTION_THROW(::std::invalid_argument,
									   "Size of input vectors 'a' and 'b' does not match"));
	}

	public: void offset(uint_vector_type o)
	{
		// pre: size(o) == size(a_)
		DCS_ASSERT(o.size() == a_.size(),
				   DCS_EXCEPTION_THROW(::std::invalid_argument,
									   "Invalid vector size"));

		o_ = o;
	}

	public: void bias(vector_type b)
	{
		// pre: size(b) == size(a_)
		DCS_ASSERT(b.size() == a_.size(),
				   DCS_EXCEPTION_THROW(::std::invalid_argument,
									   "Invalid vector size"));

		b_ = b;
	}

	private: void do_upper_bound(value_type val)
	{
		ub_ = vector_type(a_.size(), val);
	}

	private: void do_lower_bound(value_type val)
	{
		lb_ = vector_type(a_.size(), val);
	}

	protected: vector_type a_; ///< The amplitude of the signal (i.e., the peak deviation of the signal from its center position)
	protected: uint_vector_type p_; ///< The frequency of the signal (i.e., the number of time samples per sine wave period)
	protected: uint_vector_type o_; ///< The offset (phase-shift) of the signal (i.e., specifies where in signal cycle the oscillation begins at t = 0)
	protected: vector_type b_; ///< The bias (DC offset) of the signal (i.e., the constant value added to the sine amplitude)
	protected: uint_vector_type k_; ///< A repeating integer ranging from 0 to p_-1
	protected: vector_type ub_; ///< The upper bound value for the signal
	protected: vector_type lb_; ///< The lower bound value for the signal
}; // base_sinusoidal_signal_generator

template <typename VT, typename UT>
const VT base_sinusoidal_signal_generator<VT,UT>::double_pi_ = 2.0*static_cast<VT>(3.1415926535897932384626433832795029L);

}} // Namespace detail::<unnamed>


template <typename ValueT, typename UIntT>
class sinusoidal_signal_generator: public detail::base_sinusoidal_signal_generator<ValueT,UIntT>
{
	private: typedef detail::base_sinusoidal_signal_generator<ValueT,UIntT> base_type;
	public: typedef typename base_type::value_type value_type;
	public: typedef typename base_type::vector_type vector_type;
	public: typedef typename base_type::uint_type uint_type;
	public: typedef typename base_type::uint_vector_type uint_vector_type;


	public: sinusoidal_signal_generator(vector_type const& a, uint_vector_type const& p)
	: base_type(a,p)
	{
	}

	public: sinusoidal_signal_generator(vector_type const& a, uint_vector_type const& p, uint_vector_type const& o, vector_type const& b)
	: base_type(a,p,o,b)
	{
	}

	private: vector_type do_generate()
	{
		const ::std::size_t n = this->a_.size();
		vector_type u(n);

		for (::std::size_t i = 0; i < n; ++i)
		{
			// Compute new signal value
			u[i] = this->a_[i]*::std::sin(base_type::double_pi_*(this->k_[i]+this->o_[i])/this->p_[i])+this->b_[i];
			// Bound the signal
			u[i] = ::std::min(::std::max(u[i], this->lb_[i]), this->ub_[i]);

			// Increment k_
			//this->k_[i] = (this->k_[i]+1) % (2*this->p_[i]);
			this->k_[i] = (this->k_[i]+1) % this->p_[i];
		}

		return u;
	}

	private: void do_reset()
	{
		this->k_ = uint_vector_type(this->a_.size(), 0);
	}
}; // sinusoidal_signal_generator

template <typename ValueT, typename UIntT>
class half_sinusoidal_signal_generator: public detail::base_sinusoidal_signal_generator<ValueT,UIntT>
{
	private: typedef detail::base_sinusoidal_signal_generator<ValueT,UIntT> base_type;
	public: typedef typename base_type::value_type value_type;
	public: typedef typename base_type::vector_type vector_type;
	public: typedef typename base_type::uint_type uint_type;
	public: typedef typename base_type::uint_vector_type uint_vector_type;


	public: half_sinusoidal_signal_generator(vector_type const& a, uint_vector_type const& p)
	: base_type(a,p)
	{
	}

	public: half_sinusoidal_signal_generator(vector_type const& a, uint_vector_type const& p, uint_vector_type const& o, vector_type const& b)
	: base_type(a,p,o,b)
	{
	}

	private: vector_type do_generate()
	{
		const ::std::size_t n = this->a_.size();
		vector_type u(n);

		for (::std::size_t i = 0; i < n; ++i)
		{
			// Compute new signal value
			u[i] = this->a_[i]*::std::sin(base_type::double_pi_*(this->k_[i]+this->o_[i])/this->p_[i])+this->b_[i];
			// Bound the signal
			u[i] = ::std::min(::std::max(u[i], this->lb_[i]), this->ub_[i]);

			// Increment k_
			this->k_[i] = (this->k_[i]+1) % this->p_[i];
		}

		return u;
	}

	private: void do_reset()
	{
		this->k_ = uint_vector_type(this->a_.size(), 0);
	}
}; // half_sinusoidal_signal_generator

template <typename ValueT, typename UIntT>
class sinusoidal_mesh_signal_generator: public detail::base_sinusoidal_signal_generator<ValueT,UIntT>
{
	private: typedef detail::base_sinusoidal_signal_generator<ValueT,UIntT> base_type;
	public: typedef typename base_type::value_type value_type;
	public: typedef typename base_type::vector_type vector_type;
	public: typedef typename base_type::uint_type uint_type;
	public: typedef typename base_type::uint_vector_type uint_vector_type;


	public: sinusoidal_mesh_signal_generator(vector_type const& a, uint_vector_type const& p)
	: base_type(a,p)
	{
	}

	public: sinusoidal_mesh_signal_generator(vector_type const& a, uint_vector_type const& p, uint_vector_type const& o, vector_type const& b)
	: base_type(a,p,o,b)
	{
	}

	private: vector_type do_generate()
	{
		const ::std::size_t n = this->a_.size();
		vector_type u(n);

		// Compute signal value
		for (::std::size_t i = 0; i < n; ++i)
		{
			// Compute new signal value
			u[i] = this->a_[i]*::std::sin(base_type::double_pi_*(this->k_[i]+this->o_[i])/this->p_[i])+this->b_[i];
			// Bound the signal
			u[i] = ::std::min(::std::max(u[i], this->lb_[i]), this->ub_[i]);
		}

		// Increment k_
		for (::std::size_t i = 0; i < n; ++i)
		{
			//this->k_[i] = (this->k_[i]+1) % (2*this->p_[i]);
			this->k_[i] = (this->k_[i]+1) % this->p_[i];
			if (this->k_[i] > 0)
			{
				break;
			}
		}

		return u;
	}

	private: void do_reset()
	{
		this->k_ = uint_vector_type(this->a_.size(), 0);
	}
}; // sinusoidal_mesh_signal_generator

template <typename ValueT, typename UIntT>
class half_sinusoidal_mesh_signal_generator: public detail::base_sinusoidal_signal_generator<ValueT,UIntT>
{
	private: typedef detail::base_sinusoidal_signal_generator<ValueT,UIntT> base_type;
	public: typedef typename base_type::value_type value_type;
	public: typedef typename base_type::vector_type vector_type;
	public: typedef typename base_type::uint_type uint_type;
	public: typedef typename base_type::uint_vector_type uint_vector_type;


	public: half_sinusoidal_mesh_signal_generator(vector_type const& a, uint_vector_type const& p)
	: base_type(a,p)
	{
	}

	public: half_sinusoidal_mesh_signal_generator(vector_type const& a, uint_vector_type const& p, uint_vector_type const& o, vector_type const& b)
	: base_type(a,p,o,b)
	{
	}

	private: vector_type do_generate()
	{
		const ::std::size_t n = this->a_.size();
		vector_type u(n);

		// Compute signal value
		for (::std::size_t i = 0; i < n; ++i)
		{
			// Compute new signal value
			u[i] = this->a_[i]*::std::sin(base_type::double_pi_*(this->k_[i]+this->o_[i])/this->p_[i])+this->b_[i];
			// Bound the signal
			u[i] = ::std::min(::std::max(u[i], this->lb_[i]), this->ub_[i]);
		}

		// Increment k_
		for (::std::size_t i = 0; i < n; ++i)
		{
			this->k_[i] = (this->k_[i]+1) % this->p_[i];
			if (this->k_[i] > 0)
			{
				break;
			}
		}

		return u;
	}

	private: void do_reset()
	{
		this->k_ = uint_vector_type(this->a_.size(), 0);
	}
}; // half_sinusoidal_mesh_signal_generator

}} // Namespace dcs::testbed


#endif // DCS_TESTBED_SINUSOIDAL_SIGNAL_GENERATOR_HPP
