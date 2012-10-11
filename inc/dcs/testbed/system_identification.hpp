/**
 * \file dcs/testbed/system_identification.hpp
 *
 * \brief Class to perform a system identification experiment.
 *
 * \author Marco Guazzone (marco.guazzone@gmail.com)
 *
 * <hr/>
 *
 * Copyright (C) 2012       Marco Guazzone
 *                          [Distributed Computing System (DCS) Group,
 *                           Computer Science Institute,
 *                           Department of Science and Technological Innovation,
 *                           University of Piemonte Orientale,
 *                           Alessandria (Italy)]
 *
 * This file is part of dcsxx-testbed.
 *
 * dcsxx-testbed is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * dcsxx-testbed is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with dcsxx-testbed.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef DCS_TESTBED_SYSTEM_IDENTIFICATION_HPP
#define DCS_TESTBED_SYSTEM_IDENTIFICATION_HPP


#include <boost/smart_ptr.hpp>
#include <cstddef>
#include <dcs/assert.hpp>
#include <dcs/debug.hpp>
#include <dcs/exception.hpp>
#include <dcs/testbed/base_signal_generator.hpp>
#include <dcs/testbed/base_virtual_machine.hpp>
#include <dcs/testbed/base_workload_driver.hpp>
#include <fstream>
#include <iterator>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unistd.h>
#include <vector>

#ifdef DCS_DEBUG
# include <boost/numeric/ublas/io.hpp>
#endif // DCS_DEBUG


namespace dcs { namespace testbed {

template <typename RealT>
class system_identification
{
	public: typedef RealT real_type;
	public: typedef base_virtual_machine<real_type> vm_type;
	public: typedef ::boost::shared_ptr<vm_type> vm_pointer;
	public: typedef base_signal_generator<real_type> signal_generator_type;
	public: typedef ::boost::shared_ptr<signal_generator_type> signal_generator_pointer;
	public: typedef base_workload_driver workload_driver_type;
	public: typedef ::boost::shared_ptr<workload_driver_type> workload_driver_pointer;
	private: typedef ::std::vector<vm_pointer> vm_container;


	private: static const unsigned int default_sampling_time = 10;
	private: static const ::std::string default_output_data_file_path;


	public: system_identification()
	: ts_(default_sampling_time),
	  out_dat_file_(default_output_data_file_path)
	{
	}

	public: template <typename FwdIterT>
			system_identification(FwdIterT vm_first, FwdIterT vm_last, workload_driver_pointer const& p_wkl_driver, signal_generator_pointer const& p_sig_gen)
	: vms_(vm_first, vm_last),
	  p_wkl_driver_(p_wkl_driver),
	  p_sig_gen_(p_sig_gen),
	  ts_(default_sampling_time),
	  out_dat_file_(default_output_data_file_path)
	{
	}

	/// Set the path of the output data file.
	public: void output_data_file(::std::string const& s)
	{
		out_dat_file_ = s;
	}

	/**
	 * \brief Perform system identification by using as initial shares the
	 *  100% of resource.
	 */
	public: void run()
	{
		::std::vector<real_type> init_shares(vms_.size(), 1);

		this->run(init_shares.begin(), init_shares.end());
	}

	/**
	 * \brief Perform system identification with the given initial shares.
	 */
	public: template <typename FwdIterT>
			void run(FwdIterT share_first, FwdIterT share_end)
	{
		// distance(share_first,share_end) == size(vms_)
		DCS_ASSERT(static_cast< ::std::size_t >(::std::distance(share_first, share_end)) == vms_.size(),
				   DCS_EXCEPTION_THROW(::std::invalid_argument,
									   "Share container size does not match"));

		typedef typename vm_container::const_iterator vm_iterator;
		typedef typename signal_generator_type::vector_type share_container;

		//const ::std::size_t nt(10);//FIXME: parameterize

		// Open output data file
		::std::ofstream ofs(out_dat_file_.c_str());
		if (!ofs.good())
		{
			::std::ostringstream oss;
			oss << "Cannot open output data file '" << out_dat_file_ << "'";

			DCS_EXCEPTION_THROW(::std::runtime_error, oss.str());
		}

		// Set initial shares
		vm_iterator vm_end_it(vms_.end());
		vm_iterator vm_beg_it(vms_.begin());
		for (vm_iterator vm_it = vm_beg_it;
			 vm_it != vm_end_it;
			 ++vm_it)
		{
			vm_pointer p_vm(*vm_it);

			p_vm->cpu_share(*share_first);
			++share_first;
		}

		// Start the workload driver
		p_wkl_driver_->start();

		// Set shares according to the given signal
		::std::size_t t(0);
		while (p_wkl_driver_->alive())
		{
			if (p_wkl_driver_->ready() && p_wkl_driver_->has_observation())
			{
				share_container share((*p_sig_gen_)());

				// check: consistency
				DCS_DEBUG_ASSERT( share.size() == vms_.size() );

				double rt = p_wkl_driver_->observation();

				DCS_DEBUG_TRACE( "-- Time " << (t*ts_) );
				DCS_DEBUG_TRACE( "   Generated shares: " << dcs::debug::to_string(share.begin(), share.end()) );
				DCS_DEBUG_TRACE( "   Current Response Time: " << rt );

				ofs << t*ts_ << " " << rt;

				::std::size_t ix(0);
				for (vm_iterator vm_it = vm_beg_it;
					 vm_it != vm_end_it;
					 ++vm_it)
				{
					vm_pointer p_vm(*vm_it);

					// check: not null
					DCS_DEBUG_ASSERT( p_vm );

					DCS_DEBUG_TRACE( "   VM '" << p_vm->name() << "' :: Old CPU share: " << p_vm->cpu_share() << " :: New CPU share: " << share[ix] );

					ofs << " " << p_vm->cpu_share();

					p_vm->cpu_share(share[ix]);

					++ix;
				}

				ofs << ::std::endl;

				// Wait until the next sampling time
				::sleep(ts_);
			}

			++t;
		}

		// Stop the workload driver
		p_wkl_driver_->stop();

		// Close output data file
		ofs.close();
	}


	private: vm_container vms_; ///< VMs container
	private: workload_driver_pointer p_wkl_driver_; ///< Ptr to workload driver
	private: signal_generator_pointer p_sig_gen_; ///< Ptr to signal generator used to excite VMs
	private: unsigned int ts_; ///< The sampling time
	private: ::std::string out_dat_file_; ///< The path to the output data file
}; // system_identification

template <typename RealT>
const ::std::string system_identification<RealT>::default_output_data_file_path("./sysid_out.dat");

}} // Namespace dcs::testbed

#endif // DCS_TESTBED_SYSTEM_IDENTIFICATION_HPP
