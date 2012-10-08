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

	public: void start()
	{
		this->do_start();
	}

	public: void stop()
	{
		this->do_stop();
	}

	private: virtual void do_start() = 0;

	private: virtual void do_stop() = 0;

}; // base_workload_driver

}} // Namespace dcs::testbed

#endif // DCS_TESTBED_BASE_WORKLOAD_DRIVER_HPP
