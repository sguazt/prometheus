#ifndef DCS_TESTBED_RAIN_WORKLOAD_DRIVER_HPP
#define DCS_TESTBED_RAIN_WORKLOAD_DRIVER_HPP


#include <dcs/testbed/base_workload_driver.hpp>
#include <string>
#include <vector>


namespace dcs { namespace testbed {

class rain_workload_driver: public base_workload_driver
{
	private: ::std::string do_command() const
	{
		return cmd_;
	}

	private: ::std::vector< ::std::string > do_args() const
	{
		return args_;
	}


	private: ::std::string cmd_;
	private: ::std::vector< ::std::string > args_;
}; // rain_workload_driver

}} // Namespace dcs::testbed

#endif // DCS_TESTBED_RAIN_WORKLOAD_DRIVER_HPP
