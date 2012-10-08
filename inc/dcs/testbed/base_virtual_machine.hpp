#ifndef DCS_TESTBED_BASE_VIRTUAL_MACHINE_HPP
#define DCS_TESTBED_BASE_VIRTUAL_MACHINE_HPP

namespace dcs { namespace testbed {

template <typename RealT>
class base_virtual_machine
{
	public: typedef RealT real_type;


	public: virtual ~base_virtual_machine()
	{
		// empty
	}

	/// Get the CPU share
	public: real_type cpu_share() const
	{
		return do_cpu_share();
	}

	/// Set the CPU share
	public: void cpu_share(real_type value)
	{
		do_cpu_share(value);
	}

	private: virtual real_type do_cpu_share() const = 0;

	private: virtual void do_cpu_share(real_type value) = 0;
}; // base_virtual_machine

}} // Namespace dcs::testbed

#endif // DCS_TESTBED_BASE_VIRTUAL_MACHINE_HPP
