#ifndef DCS_TESTBED_LIBVIRT_VIRTUAL_MACHINE_HPP
#define DCS_TESTBED_LIBVIRT_VIRTUAL_MACHINE_HPP


#include <dcs/testbed/base_virtual_machine.hpp>
#include <dcs/testbed/detail/libvirt.hpp>
#include <iostream>
#include <libvirt/libvirt.h>
#include <stdexcept>
#include <string>


namespace dcs { namespace testbed {

template <typename RealT>
class libvirt_virtual_machine: public base_virtual_machine<RealT>
{
	private: typedef base_virtual_machine<RealT> base_type;
	public: typedef RealT real_type;


	public: libvirt_virtual_machine(::std::string const& vmm_uri, ::std::string const& name)
	: uri_(vmm_uri),
	  name_(name),
	  conn_(0)
	{
		init();
	}

	public: ~libvirt_virtual_machine()
	{
		// According to http://www.stlport.org/doc/exception_safety.html we avoid to throw any exception inside the destructor
		try
		{
			detail::libvirt::disconnect(conn_);
		}
		catch (::std::exception const& e)
		{
			::std::cerr << "[E] Failed to disconnect from hypervisor '" << uri_ << "': " << e.what();
		}
		catch (...)
		{
			::std::cerr << "[E] Failed to disconnect from hypervisor '" << uri_ << "': Unknown error";
		}
	}

	private: int do_num_vcpus() const
	{
		assert( 0 != conn_ );

		virDomainPtr dom = detail::libvirt::connect_domain(conn_, name_);

		int nvcpus = detail::libvirt::num_vcpus(conn_, dom, VIR_DOMAIN_AFFECT_CURRENT);

		detail::libvirt::disconnect_domain(conn_, dom);

		return nvcpus;
	}

	private: void do_cpu_share(real_type share)
	{
		assert( 0 != conn_ );

		virDomainPtr dom = detail::libvirt::connect_domain(conn_, name_);

		int nvcpus(0);
		nvcpus = detail::libvirt::num_vcpus(conn_, dom, VIR_DOMAIN_AFFECT_CURRENT);

		//FIXME: This is a Xen-related stuff. What for other hypervisors?
		//FIXME: Actually we assume that weight is 256 (its default value)
		int cap(share < 1.0 ? share*nvcpus*100 : 0); //Note: cap == 0 ==> No upper cap
		detail::libvirt::sched_param<int>(conn_, dom, "cap", cap, VIR_DOMAIN_AFFECT_CURRENT);

		detail::libvirt::disconnect_domain(conn_, dom);
	}

	private: real_type do_cpu_share() const
	{
		assert( 0 != conn_ );

		virDomainPtr dom = detail::libvirt::connect_domain(conn_, name_);

		int cap(0);
		cap = detail::libvirt::sched_param<int>(conn_, dom, "cap", VIR_DOMAIN_AFFECT_CURRENT);

		int nvcpus(0);
		nvcpus = detail::libvirt::num_vcpus(conn_, dom, VIR_DOMAIN_AFFECT_CURRENT);

		detail::libvirt::disconnect_domain(conn_, dom);

		//FIXME: This is a Xen-related stuff. What for other hypervisors?
		//FIXME: Actually we assume that weight is 256 (its default value)
		real_type share(cap/(nvcpus*100.0));

		return share > 0 ? share : 1; //Note: cap == 0 ==> No upper cap
	}

	private: void init()
	{
		// Connect to libvirtd daemon
		conn_ = detail::libvirt::connect(uri_);
	}


	private: ::std::string uri_;
	private: ::std::string name_;
	private: virConnectPtr conn_;
}; // libvirt_virtual_machine

}} // Namespace dcs::testbed

#endif // DCS_TESTBED_LIBVIRT_VIRTUAL_MACHINE_HPP
