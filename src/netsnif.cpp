/**
 * \file netsnif_engine.hpp
 *
 * \brief Network packet-level sniffer engine.
 *
 * \author Marco Guazzone (marco.guazzone@gmail.com)
 *
 * <hr/>
 *
 * Copyright (C) 2012       Marco Guazzone (marco.guazzone@gmail.com)
 *                          [Distributed Computing System (DCS) Group,
 *                           Computer Science Institute,
 *                           Department of Science and Technological Innovation,
 *                           University of Piemonte Orientale,
 *                           Alessandria (Italy)]
 *
 * This file is part of dcsxx-testbed (below referred to as "this program").
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <boost/smart_ptr.hpp>
#include <dcs/cli.hpp>
#include <dcs/debug.hpp>
#include <dcs/exception.hpp>
#include <dcs/logging.hpp>
#include <dcs/macro.hpp>
#include <dcs/network/ethernet.hpp>
#include <dcs/network/ip.hpp>
#include <dcs/network/pcap.hpp>
#include <dcs/network/tcp.hpp>
#include <exception>
#include <iostream>
#include <netdb.h>
#include <sqlite3.h>
#include <string>
#include <stdexcept>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>


namespace detail {

enum connection_status_category
{
	unknown_connection_status = -1,
	wait_connection_status = 0,
	active_connection_status,
	closed_connection_status,
};


struct network_connection
{
	network_connection()
	: server_port(0),
	  client_port(0),
	  status(unknown_connection_status)
	{
	}


	::std::string server_address;
	::boost::uint16_t server_port;
	::std::string client_address;
	::boost::uint16_t client_port;
	connection_status_category status;
	::std::string last_update_datetime;
}; // network_connection


struct base_data_store
{
	virtual void open() = 0;

	virtual void clear() = 0;

	virtual ::boost::shared_ptr<network_connection> load(::std::string const& server_address,
														 ::boost::uint16_t server_port,
														 ::std::string const& client_address,
														 ::boost::uint16_t client_port) = 0;

	virtual void save(network_connection const& conn) = 0;

	virtual void erase(::std::string const& server_address,
					   ::boost::uint16_t server_port,
					   ::std::string const& client_address,
					   ::boost::uint16_t client_port) = 0;

	virtual void erase(network_connection const& conn) = 0;

	virtual unsigned long num_connections(::std::string const& server_address,
										  ::boost::uint16_t server_port,
										  connection_status_category status) = 0;

	virtual void begin_transaction() = 0;

	virtual void commit_transaction() = 0;

	virtual void rollback_transaction() = 0;

	virtual bool is_open() const = 0;

	virtual void close() = 0;
}; // base_data_store


class sqlite3_data_store: public base_data_store
{
	private: static const ::std::string tbl_connection;
	private: static const ::std::string stmt_create_tbl_connection;
	private: static const ::std::string stmt_delete_all_tbl_connection;
	private: static const ::std::string stmt_delete_tbl_connection;
	private: static const ::std::string stmt_replace_tbl_connection;
	private: static const ::std::string stmt_select_tbl_connection;
	private: static const ::std::string stmt_count_status_tbl_connection;


	public: sqlite3_data_store()
	: p_db_(0)
	{
	}

	public: sqlite3_data_store(::std::string const& db_name)
	: name_(db_name),
	  p_db_(0)
	{
	}

	public: ~sqlite3_data_store()
	{
		this->close();
	}

	public: void open()
	{
		int ret(SQLITE_OK);

		this->close();

		// Open the DB
		ret = ::sqlite3_open(name_.c_str(), &p_db_);
		if (ret != SQLITE_OK)
		{
			::std::ostringstream oss;
			oss << "Unable to open DB: " << ::sqlite3_errmsg(p_db_);

			DCS_EXCEPTION_THROW(::std::runtime_error, oss.str());
		}

		// Enable the extended result codes
		::sqlite3_extended_result_codes(p_db_, 1);

		// Create tables (if needed)
		ret = sqlite3_exec(p_db_, stmt_create_tbl_connection.c_str(), 0, 0, 0);
		if (ret != SQLITE_OK)
		{
			::std::ostringstream oss;
			oss << "Unable to create table '" << tbl_connection << "': " << ::sqlite3_errmsg(p_db_);

			DCS_EXCEPTION_THROW(::std::runtime_error, oss.str());
		}
	}

	public: void clear()
	{
		DCS_ASSERT(this->is_open(),
				   DCS_EXCEPTION_THROW(::std::logic_error,
									   "DB is not open"));

		int ret(SQLITE_OK);
		ret = sqlite3_exec(p_db_, stmt_delete_all_tbl_connection.c_str(), 0, 0, 0);
		if (ret != SQLITE_OK)
		{
			::std::ostringstream oss;
			oss << "Unable to clear table '" << tbl_connection << "': " << ::sqlite3_errmsg(p_db_);

			DCS_EXCEPTION_THROW(::std::runtime_error, oss.str());
		}
	}

	public: ::boost::shared_ptr<network_connection> load(::std::string const& server_address,
														 ::boost::uint16_t server_port,
														 ::std::string const& client_address,
														 ::boost::uint16_t client_port)
	{
		DCS_ASSERT(this->is_open(),
				   DCS_EXCEPTION_THROW(::std::logic_error,
									   "DB is not open"));

		char* sql = ::sqlite3_mprintf(stmt_select_tbl_connection.c_str(),
									  server_address.c_str(),
									  server_port,
									  client_address.c_str(),
									  client_port);

		DCS_DEBUG_TRACE("-- SQL: " << sql);//XXX

		network_connection* p_conn = new network_connection();

		int ret(SQLITE_OK);
		ret = sqlite3_exec(p_db_, sql, &load_callback, p_conn, 0);
		if (ret != SQLITE_OK)
		{
			::std::ostringstream oss;
			oss << "Unable to load (" << server_address << ":" << server_port << "," << client_address << ":" << client_port << ") from table '" << tbl_connection << "': " << ::sqlite3_errmsg(p_db_);
			::sqlite3_free(sql);

			DCS_EXCEPTION_THROW(::std::runtime_error, oss.str());
		}

		::sqlite3_free(sql);

		p_conn->server_address = server_address;
		p_conn->server_port = server_port;
		p_conn->client_address = client_address;
		p_conn->client_port = client_port;

		return ::boost::shared_ptr<network_connection>(p_conn);
	}

	public: void save(network_connection const& conn)
	{
		DCS_ASSERT(this->is_open(),
				   DCS_EXCEPTION_THROW(::std::logic_error,
									   "DB is not open"));

		char* sql = ::sqlite3_mprintf(stmt_replace_tbl_connection.c_str(),
									  conn.server_address.c_str(),
									  conn.server_port,
									  conn.client_address.c_str(),
									  conn.client_port,
									  conn.status);

		DCS_DEBUG_TRACE("-- SQL: " << sql);//XXX

		int ret(SQLITE_OK);
		ret = sqlite3_exec(p_db_, sql, 0, 0, 0);
		if (ret != SQLITE_OK)
		{
			::std::ostringstream oss;
			oss << "Unable to save (" << conn.server_address << ":" << conn.server_port << "," << conn.client_address << ":" << conn.client_port << ") into table '" << tbl_connection << "': " << ::sqlite3_errmsg(p_db_);
			::sqlite3_free(sql);

			DCS_EXCEPTION_THROW(::std::runtime_error, oss.str());
		}

		::sqlite3_free(sql);
	}

	public: void erase(::std::string const& server_address,
					   ::boost::uint16_t server_port,
					   ::std::string const& client_address,
					   ::boost::uint16_t client_port)
	{
		DCS_ASSERT(this->is_open(),
				   DCS_EXCEPTION_THROW(::std::logic_error,
									   "DB is not open"));

		char* sql = ::sqlite3_mprintf(stmt_delete_tbl_connection.c_str(),
									  server_address.c_str(),
									  server_port,
									  client_address.c_str(),
									  client_port);

		DCS_DEBUG_TRACE("-- SQL: " << sql);//XXX

		int ret(SQLITE_OK);
		ret = sqlite3_exec(p_db_, sql, 0, 0, 0);
		if (ret != SQLITE_OK)
		{
			::std::ostringstream oss;
			oss << "Unable to erase (" << server_address << ":" << server_port << "," << client_address << ":" << client_port << ") from table '" << tbl_connection << "': " << ::sqlite3_errmsg(p_db_);
			::sqlite3_free(sql);

			DCS_EXCEPTION_THROW(::std::runtime_error, oss.str());
		}

		::sqlite3_free(sql);
	}

	public: void erase(network_connection const& conn)
	{
		this->erase(conn.server_address,
					conn.server_port,
					conn.client_address,
					conn.client_port);
	}

	public: void begin_transaction()
	{
		DCS_ASSERT(this->is_open(),
				   DCS_EXCEPTION_THROW(::std::logic_error,
									   "DB is not open"));

		int ret(SQLITE_OK);
		ret = sqlite3_exec(p_db_, "BEGIN TRANSACTION", 0, 0, 0);
		if (ret != SQLITE_OK)
		{
			::std::ostringstream oss;
			oss << "Unable to begin a new transaction: " << ::sqlite3_errmsg(p_db_);

			DCS_EXCEPTION_THROW(::std::runtime_error, oss.str());
		}
	}

	public: void commit_transaction()
	{
		DCS_ASSERT(this->is_open(),
				   DCS_EXCEPTION_THROW(::std::logic_error,
									   "DB is not open"));

		int ret(SQLITE_OK);
		ret = sqlite3_exec(p_db_, "COMMIT TRANSACTION", 0, 0, 0);
		if (ret != SQLITE_OK)
		{
			::std::ostringstream oss;
			oss << "Unable to commit current transaction: " << ::sqlite3_errmsg(p_db_);

			DCS_EXCEPTION_THROW(::std::runtime_error, oss.str());
		}
	}

	public: void rollback_transaction()
	{
		DCS_ASSERT(this->is_open(),
				   DCS_EXCEPTION_THROW(::std::logic_error,
									   "DB is not open"));

		int ret(SQLITE_OK);
		ret = sqlite3_exec(p_db_, "ROLLBACK TRANSACTION", 0, 0, 0);
		if (ret != SQLITE_OK)
		{
			::std::ostringstream oss;
			oss << "Unable to rollback current transaction: " << ::sqlite3_errmsg(p_db_);

			DCS_EXCEPTION_THROW(::std::runtime_error, oss.str());
		}
	}

	public: void close()
	{
		if (this->is_open())
		{
			::sqlite3_close(p_db_);
			p_db_ = 0;
		}
	}

	public: bool is_open() const
	{
		return p_db_ ? true : false;
	}

	public: unsigned long num_connections(::std::string const& server_address,
										  ::boost::uint16_t server_port,
										  connection_status_category status)
	{
		DCS_ASSERT(this->is_open(),
				   DCS_EXCEPTION_THROW(::std::logic_error,
									   "DB is not open"));

		int ret(SQLITE_OK);
		char* sql = ::sqlite3_mprintf(stmt_count_status_tbl_connection.c_str(),
									  server_address.c_str(),
									  server_port,
									  status);

		DCS_DEBUG_TRACE("-- SQL: " << sql);//XXX

		unsigned long count(0);
		ret = sqlite3_exec(p_db_, sql, &num_connections_callback, &count, 0);
		if (ret != SQLITE_OK)
		{
			::std::ostringstream oss;
			oss << "Unable to count connections (" << server_address << ":" << server_port << ") from table '" << tbl_connection << "': " << ::sqlite3_errmsg(p_db_);
			::sqlite3_free(sql);

			DCS_EXCEPTION_THROW(::std::runtime_error, oss.str());
		}

		::sqlite3_free(sql);

		return count;
	}

	private: static int load_callback(void* user_data, int num_cols, char** col_values, char** col_names)
	{
		DCS_DEBUG_ASSERT( user_data );

		network_connection* p_conn = static_cast<network_connection*>(user_data);
		::std::istringstream iss;
		iss.str(col_values[0]);
		int status;
		iss >> status;
		p_conn->status = static_cast<detail::connection_status_category>(status);
//		p_conn->last_update_timestamp = col_values[1];

		return 0;
	}

	private: static int num_connections_callback(void* user_data, int num_cols, char** col_values, char** col_names)
	{
		DCS_DEBUG_ASSERT( user_data );

		unsigned long* p_count = static_cast<unsigned long*>(user_data);
		::std::istringstream iss;
		iss.str(col_values[0]);
		iss >> *p_count;

		return 0;
	}

	private: ::std::string name_;
	private: ::sqlite3* p_db_;
}; // sqlite3_data_store

const ::std::string sqlite3_data_store::tbl_connection = "network_connection";

const ::std::string sqlite3_data_store::stmt_create_tbl_connection = "CREATE TABLE IF NOT EXISTS " + tbl_connection + " ("
																	 "  server_addr TEXT DEFAULT ''"
																	 ", server_port INTEGER DEFAULT 0"
																	 ", client_addr TEXT DEFAULT ''"
																	 ", client_port INTEGER DEFAULT 0"
																	 ", status INTEGER DEFAULT 0"
																	 ", last_update TEXT DEFAULT (datetime('now'))"
																	 ", CONSTRAINT pk_addr_port PRIMARY KEY (server_addr,server_port,client_addr,client_port)"
																	 ")";

const ::std::string sqlite3_data_store::stmt_delete_all_tbl_connection = "DELETE FROM " + tbl_connection;

const ::std::string sqlite3_data_store::stmt_delete_tbl_connection = "DELETE FROM " + tbl_connection + " WHERE server_addr=%Q AND server_port=%u AND client_addr=%Q AND client_port=%u";

const ::std::string sqlite3_data_store::stmt_replace_tbl_connection = "REPLACE INTO " + tbl_connection +
																	   " (server_addr,server_port,client_addr,client_port,status,last_update)"
																	   " VALUES (%Q,%u,%Q,%u,%lu,(datetime('now')))";

const ::std::string sqlite3_data_store::stmt_select_tbl_connection = "SELECT status,last_update FROM " + tbl_connection +
																	 " WHERE server_addr=%Q"
																	 " AND   server_port=%u"
																	 " AND   client_addr=%Q"
																	 " AND   client_port=%u";

const ::std::string sqlite3_data_store::stmt_count_status_tbl_connection = "SELECT COUNT(*) FROM " + tbl_connection +
																		   " WHERE server_addr=%Q"
																		   " AND   server_port=%u"
																		   " GROUP BY status"
																		   " HAVING status=%d";


//class in_memory_data_store: public base_data_store
//{
//}; // in_memory_data_store


class network_connection_manager
{
	public: typedef ::boost::shared_ptr<base_data_store> data_store_pointer;


	public: network_connection_manager(data_store_pointer const& p_ds)
	: p_ds_(p_ds)
	{
		if (!p_ds_->is_open())
		{
			p_ds_->open();
		}
	}

	public: void begin_connection_establishment(::std::string const& server_address,
												::boost::uint16_t server_port,
												::std::string const& client_address,
												::boost::uint16_t client_port)
	{
		bool in_trans(false);

		try
		{
			p_ds_->begin_transaction();

			in_trans = true;

			::boost::shared_ptr<network_connection> p_conn = p_ds_->load(server_address,
																		 server_port,
																		 client_address,
																		 client_port);

			//p_conn->wait_count += 1;
			p_conn->status = wait_connection_status;

			p_ds_->save(*p_conn);

			p_ds_->commit_transaction();

			in_trans = false;
		}
		catch (::std::exception const& e)
		{
			if (in_trans)
			{
				p_ds_->rollback_transaction();
			}
			DCS_EXCEPTION_RETHROW(e);
		}
		catch (...)
		{
			if (in_trans)
			{
				p_ds_->rollback_transaction();
			}
			DCS_EXCEPTION_THROW(::std::runtime_error, "Unknown error");
		}
	}

	public: void end_connection_establishment(::std::string const& server_address,
											  ::boost::uint16_t server_port,
											  ::std::string const& client_address,
											  ::boost::uint16_t client_port)
	{
		bool in_trans(false);

		try
		{
			p_ds_->begin_transaction();

			in_trans = true;

			::boost::shared_ptr<network_connection> p_conn = p_ds_->load(server_address,
																		 server_port,
																		 client_address,
																		 client_port);

			if (p_conn->status == wait_connection_status)
			{
				p_conn->status = active_connection_status;

				p_ds_->save(*p_conn);
			}
			else
			{
				// Treat this entry as garbage and delete it

				::std::ostringstream oss;
				oss << "Found connection status is '" << p_conn->status << ": expected '" << wait_connection_status << "'";
				::dcs::log_warn(DCS_LOGGING_AT, oss.str());

				p_ds_->erase(*p_conn);
			}

			p_ds_->commit_transaction();

			in_trans = false;
		}
		catch (::std::exception const& e)
		{
			if (in_trans)
			{
				p_ds_->rollback_transaction();
			}
			DCS_EXCEPTION_RETHROW(e);
		}
		catch (...)
		{
			if (in_trans)
			{
				p_ds_->rollback_transaction();
			}
			DCS_EXCEPTION_THROW(::std::runtime_error, "Unknown error");
		}
	}

	public: void begin_connection_termination(::std::string const& server_address,
											  ::boost::uint16_t server_port,
											  ::std::string const& client_address,
											  ::boost::uint16_t client_port)
	{
		DCS_MACRO_SUPPRESS_UNUSED_VARIABLE_WARNING( server_address );
		DCS_MACRO_SUPPRESS_UNUSED_VARIABLE_WARNING( server_port );
		DCS_MACRO_SUPPRESS_UNUSED_VARIABLE_WARNING( client_address );
		DCS_MACRO_SUPPRESS_UNUSED_VARIABLE_WARNING( client_port );

		// Nothing to do here.
		// We will decrement active count once we are sure the connection is terminated
	}

	public: void end_connection_termination(::std::string const& server_address,
											::boost::uint16_t server_port,
											::std::string const& client_address,
											::boost::uint16_t client_port)
	{
		bool in_trans(false);

		try
		{
			p_ds_->begin_transaction();

			in_trans = true;

			::boost::shared_ptr<network_connection> p_conn = p_ds_->load(server_address,
																		 server_port,
																		 client_address,
																		 client_port);

			if (p_conn->status == active_connection_status)
			{
				p_conn->status = closed_connection_status;

				p_ds_->save(*p_conn);
			}
			else
			{
				// Treat this entry as garbage and delete it

				::std::ostringstream oss;
				oss << "Found connection status is '" << p_conn->status << ": expected '" << active_connection_status << "'";
				::dcs::log_warn(DCS_LOGGING_AT, oss.str());

				p_ds_->erase(*p_conn);
			}

			p_ds_->commit_transaction();

			in_trans = false;
		}
		catch (::std::exception const& e)
		{
			if (in_trans)
			{
				p_ds_->rollback_transaction();
			}
			DCS_EXCEPTION_RETHROW(e);
		}
		catch (...)
		{
			if (in_trans)
			{
				p_ds_->rollback_transaction();
			}
			DCS_EXCEPTION_THROW(::std::runtime_error, "Unknown error");
		}
	}

	public: unsigned long num_connections(::std::string const& server_address,
										  ::boost::uint16_t server_port,
										  connection_status_category status) const
	{
		return p_ds_->num_connections(server_address, server_port, status);
	}


	private: data_store_pointer p_ds_;
}; // network_connection_manager

} // Namespace detail


namespace detail { namespace /*<unnamed>*/ {

static const ::std::string default_server_address = "127.0.0.1";
static const ::boost::uint16_t default_server_port = 9999;
static const ::std::string default_db_file = "./sniffer.db";
static const ::std::string default_device = "lo";


void usage(char const* progname)
{
	::std::cerr << "Usage: " << progname << " [options]" << ::std::endl
				<< " --db <file path>" << ::std::endl
				<< "   The path to the database where packet information is stored." << ::std::endl
				<< "   [default: '" << default_db_file << "']." << ::std::endl
				<< " --dev <device name>" << ::std::endl
				<< "   The name of the capture device (e.g., eth0, lo, ...)." << ::std::endl
				<< "   [default: the first available device]." << ::std::endl
				<< " --addr <IP address or host name>" << ::std::endl
				<< "   The IP address or host name of the host to monitor." << ::std::endl
				<< "   [default: '" << default_server_address << "']." << ::std::endl
				<< " --port <port number>" << ::std::endl
				<< "   The port number of the host to monitor." << ::std::endl
				<< "   [default: '" << default_server_port << "']." << ::std::endl
				<< " --help" << ::std::endl
				<< "   Show this message." << ::std::endl
				<< ::std::endl;
}

::std::string host_address(::std::string const& name)
{
	::addrinfo hints;
	::std::memset(&hints, 0, sizeof(::addrinfo));
	hints.ai_family = AF_UNSPEC;    // Allow IPv4 or IPv6
	hints.ai_socktype = SOCK_DGRAM; // Datagram socket
	hints.ai_flags = AI_PASSIVE;    // For wildcard IP address
	hints.ai_protocol = 0;          // Any protocol
	hints.ai_canonname = 0;
	hints.ai_addr = 0;
	hints.ai_next = 0;

	int ret(0);
	::addrinfo* addrs(0);
	ret = ::getaddrinfo(name.c_str(), 0, &hints, &addrs);
	if (ret != 0)
	{
		::std::ostringstream oss;
		oss << "Error on getting address information: " << gai_strerror(ret);

		throw ::std::runtime_error(oss.str());
	}

	::std::string host_addr;
	char hbuf[NI_MAXHOST];
	for (::addrinfo* addr = addrs; addr != 0; addr = addr->ai_next)
	{
		ret = ::getnameinfo(addr->ai_addr,
							addr->ai_addrlen,
							hbuf,
							sizeof(hbuf),
							0,
							0,
							NI_NUMERICHOST);
		if (ret == 0)
		{
			host_addr = hbuf;
			break;
		}
	}

	::freeaddrinfo(addrs);

	return host_addr;
}

}} // Namespace detail::<unnamed>


int main(int argc, char* argv[])
{
	std::string db_file; // path to the DB file
	std::string dev; // capture device
	bool help(false);
	std::string srv_address; // server address
	boost::uint16_t srv_port; // server port

	std::string default_device;
	try
	{
		default_device = dcs::network::pcap::lookup_device();
	}
	catch (...)
	{
		// No suitable device found, try with "lo"
		default_device = detail::default_device;
	}

	// Parse command line options
	try
	{
		db_file = dcs::cli::simple::get_option<std::string>(argv, argv+argc, "--db", detail::default_db_file);
		dev = dcs::cli::simple::get_option<std::string>(argv, argv+argc, "--dev", default_device);
		help = dcs::cli::simple::get_option(argv, argv+argc, "--help");
		srv_address = dcs::cli::simple::get_option<std::string>(argv, argv+argc, "--addr", detail::default_server_address);
		srv_port = dcs::cli::simple::get_option<boost::uint16_t>(argv, argv+argc, "--port", detail::default_server_port);
	}
	catch (std::exception const& e)
	{
		std::ostringstream oss;
		oss << "Error while parsing command-line options: " << e.what();
		dcs::log_error(DCS_LOGGING_AT, oss.str());

		detail::usage(argv[0]);
		return EXIT_FAILURE;
	}

	if (help)
	{
		detail::usage(argv[0]);
		return EXIT_SUCCESS;
	}

	srv_address = detail::host_address(srv_address);

	detail::network_connection_manager stats_mgr(boost::make_shared<detail::sqlite3_data_store>(db_file));

	// Open the device for sniffing
	dcs::network::pcap::live_packet_sniffer sniffer(dev);

	sniffer.snapshot_length(65535);
	sniffer.promiscuous_mode(true);
	sniffer.timeout(1000);

	//std::string filter_expr = "tcp[tcpflags] & (tcp-syn|tcp-ack|tcp-fin) != 0";
	//std::string filter_expr = "tcp[tcpflags]";
	std::string filter_expr;
	{
		std::ostringstream oss;
		//oss << "tcp[tcpflags] host " << srv_address << " port " << srv_port;
		oss << "tcp and host " << srv_address << " and port " << srv_port;
		filter_expr = oss.str();
	}

	sniffer.filter(filter_expr);

	boost::shared_ptr<dcs::network::pcap::raw_packet> p_pkt;

	const unsigned int num_trials(5);
	const unsigned int zzz_time(2);
	unsigned int trial(0);
	do
	{
		++trial;

		unsigned long count(0);
		while (p_pkt = sniffer.capture())
		{
			++count;

			//DCS_DEBUG_TRACE( "-[" << count << "] Grabbed new packet: " << *p_pkt );

			boost::shared_ptr<dcs::network::ethernet_frame> p_eth = dcs::network::pcap::make_ethernet_frame(*p_pkt);
			DCS_DEBUG_TRACE( "-[" << count << "] -> Ethernet frame: " << *p_eth );

			if (p_eth->ethertype_field() == ::dcs::network::ethernet_frame::ethertype_ipv4)
			{
				boost::shared_ptr<dcs::network::ip4_packet> p_ip = ::boost::make_shared<dcs::network::ip4_packet>(p_eth->payload(), p_eth->payload_size());
				DCS_DEBUG_TRACE( "-[" << count << "] -> IP packet: " << *p_ip );

				if (p_ip->protocol_field() == ::dcs::network::ip4_packet::proto_tcp)
				{
					boost::shared_ptr<dcs::network::tcp_segment> p_tcp = ::boost::make_shared<dcs::network::tcp_segment>(p_ip->payload(), p_ip->payload_size());
					DCS_DEBUG_TRACE( "-[" << count << "] -> TCP segment: " << *p_tcp );
#ifdef DCS_DEBUG
					if (p_tcp->payload_size() > 0)
					{
						char const* payload = reinterpret_cast<char const*>(p_tcp->payload());
						std::size_t payload_sz = p_tcp->payload_size();
						std::size_t i = 0;
						while (i < payload_sz && std::isprint(payload[i]))
						{
							++i;
						}
						if (i == payload_sz)
						{
							DCS_DEBUG_TRACE( "-[" << count << "] -> TCP payload: " << std::string(payload, payload_sz) );
						}
						else
						{
							DCS_DEBUG_TRACE( "-[" << count << "] -> TCP payload: <binary data>" );
						}
					}
#endif // DCS_DEBUG

					//::std::string srv_address;
					//::boost::uint16_t srv_port(0);
					::std::string cli_address;
					::boost::uint16_t cli_port(0);

					std::string src_addr = p_ip->source_address();
					std::string dst_addr = p_ip->destination_address();

					if (src_addr == srv_address && p_tcp->source_port_field() == srv_port)
					{
						// SERVER --> CLIENT

						if (p_tcp->payload_size() > 0)
						{
							DCS_DEBUG_TRACE( "TCP have PAYLOAD" );

							// The server sends back data to the client
							//srv_address = src_addr;
							//srv_port = p_tcp->source_port_field();
							cli_address = dst_addr;
							cli_port = p_tcp->destination_port_field();

							try
							{
								stats_mgr.end_connection_establishment(srv_address, srv_port, cli_address, cli_port);
							}
							catch (std::exception const& e)
							{
								std::ostringstream oss;
								oss << "Stats update for end of connection establishment: " << e.what();
								dcs::log_error(DCS_LOGGING_AT, oss.str());
							}
						}
						else if (p_tcp->have_flags(dcs::network::tcp_segment::flags_fin))
						{
							// We are in the four-way handshake procedure (connection termination)

							if (p_tcp->have_flags(dcs::network::tcp_segment::flags_ack))
							{
								DCS_DEBUG_TRACE( "TCP have FIN-ACK" );

								// The server sends back FIN-ACK to the client
								//srv_address = src_addr;
								//srv_port = p_tcp->source_port_field();
								cli_address = dst_addr;
								cli_port = p_tcp->destination_port_field();

								try
								{
									stats_mgr.end_connection_termination(srv_address, srv_port, cli_address, cli_port);
								}
								catch (std::exception const& e)
								{
									std::ostringstream oss;
									oss << "Stats update for end of connection termination: " << e.what();
									dcs::log_error(DCS_LOGGING_AT, oss.str());
								}
							}
						}
					}
					else if (dst_addr == srv_address && p_tcp->destination_port_field() == srv_port)
					{
						// CLIENT --> SERVER

						if (p_tcp->have_flags(dcs::network::tcp_segment::flags_syn))
						{
							DCS_DEBUG_TRACE( "TCP have SYN" );

							// The client begins the 3-way handshake by sending SYN to the server
							cli_address = src_addr;
							cli_port = p_tcp->source_port_field();
							//srv_address = dst_addr;
							//srv_port = p_tcp->destination_port_field();

							try
							{
								stats_mgr.begin_connection_establishment(srv_address, srv_port, cli_address, cli_port);
							}
							catch (std::exception const& e)
							{
								std::ostringstream oss;
								oss << "Stats update for begin of connection establishment: " << e.what();
								dcs::log_error(DCS_LOGGING_AT, oss.str());
							}
						}
						else if (p_tcp->have_flags(dcs::network::tcp_segment::flags_fin))
						{
							// The client begins the 4-way handshake by sending FIN to the server
							cli_address = src_addr;
							cli_port = p_tcp->source_port_field();
							//srv_address = dst_addr;
							//srv_port = p_tcp->destination_port_field();

							try
							{
								stats_mgr.begin_connection_termination(srv_address, srv_port, cli_address, cli_port);
							}
							catch (std::exception const& e)
							{
								std::ostringstream oss;
								oss << "Stats update for begin of connection termination: " << e.what();
								dcs::log_error(DCS_LOGGING_AT, oss.str());
							}
						}
//						std::cout << ":: Num Waiting Connections for (" << srv_address << ":" << srv_port << "): " << stats_mgr.num_connections(srv_address, srv_port, detail::wait_connection_status) << std::endl;
					}
					std::cout << ":: Num Waiting Connections for (" << srv_address << ":" << srv_port << "): " << stats_mgr.num_connections(srv_address, srv_port, detail::wait_connection_status) << std::endl;
				}
				std::cout << "--------------------------------------------" << std::endl;
			}
		}

		//::sleep(zzz_time);
	}
	//while (trial < num_trials);
	while (true);
}
