#ifndef DCS_TESTBED_BASE_WORKLOAD_DRIVER_HPP
#define DCS_TESTBED_BASE_WORKLOAD_DRIVER_HPP


#include <string>
#include <vector>


namespace dcs { namespace testbed {

class base_workload_driver
{
	public: virtual ~base_workload_driver()
	{
	}

	public: void start(bool asynch = true)
	{
	}

	public: void stop()
	{
	}

	private: virtual ::std::string do_command() const = 0;

	private: virtual ::std::vector< ::std::string > do_args() const = 0;

}; // base_workload_driver

}} // Namespace dcs::testbed

#endif // DCS_TESTBED_BASE_WORKLOAD_DRIVER_HPP
