#ifndef DCS_TESTBED_SYSTEM_IDENTIFICATION_HPP
#define DCS_TESTBED_SYSTEM_IDENTIFICATION_HPP


#ifndef NDEBUG
# include <boost/numeric/ublas/io.hpp>
#endif // NDEBUG
#include <boost/smart_ptr.hpp>
#include <cstddef>
#include <dcs/testbed/base_signal_generator.hpp>
#include <dcs/testbed/base_virtual_machine.hpp>
#ifndef NDEBUG
# include <iostream>
#endif // NDEBUG


namespace dcs { namespace testbed {

template <typename RealT>
class system_identification
{
	public: typedef RealT real_type;
	public: typedef base_virtual_machine<real_type> vm_type;
	public: typedef ::boost::shared_ptr<vm_type> vm_pointer;
	public: typedef base_signal_generator<real_type> signal_generator_type;
	public: typedef ::boost::shared_ptr<signal_generator_type> signal_generator_pointer;

	public: system_identification(vm_pointer const& p_oliodb_vm, vm_pointer const& p_olioweb_vm, signal_generator_pointer const& p_sig_gen)
	: p_oliodb_vm_(p_oliodb_vm),
	  p_olioweb_vm_(p_olioweb_vm),
	  p_sig_gen_(p_sig_gen)
	{
	}


	public: void run()
	{
		typedef typename signal_generator_type::vector_type vector_type;

		const ::std::size_t nt(10);//FIXME: parameterize

		for (::std::size_t t = 0; t < nt; ++t)
		{
			vector_type share((*p_sig_gen_)());

#ifndef NDEBUG
			::std::clog << "-- Time " << t << ::std::endl;
			::std::clog << "  OlioDB :: Old CPU share: " << p_oliodb_vm_->cpu_share() << ::std::endl;
			::std::clog << "  OlioWeb :: Old CPU share: " << p_olioweb_vm_->cpu_share() << ::std::endl;
			::std::clog << "  Generated shares: " << share << ::std::endl;
#endif // NDEBUG

			p_oliodb_vm_->cpu_share(share(0));
			p_olioweb_vm_->cpu_share(share(1));

#ifndef NDEBUG
			::std::clog << "  OlioDB :: New CPU share: " << p_oliodb_vm_->cpu_share() << ::std::endl;
			::std::clog << "  OlioWeb :: New CPU share: " << p_olioweb_vm_->cpu_share() << ::std::endl;
#endif // NDEBUG
		}

		p_oliodb_vm_->cpu_share(1);
		p_olioweb_vm_->cpu_share(1);
	}


	private: vm_pointer p_oliodb_vm_; ///< Ptr to the OlioDB VM
	private: vm_pointer p_olioweb_vm_; ///< Ptr to the OlioWeb VM
	private: signal_generator_pointer p_sig_gen_; ///< Ptr to signal generator used to excite VMs
}; // system_identification

}} // Namespace dcs::testbed

#endif // DCS_TESTBED_SYSTEM_IDENTIFICATION_HPP
