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


#if    !defined(DCS_TESTBED_NETSNIF_USE_BOOST_THREAD_SYNC_PACKET_QUEUE) \
	&& !defined(DCS_TESTBED_NETSNIF_USE_BOOST_LOCKFREE_MWMR_PACKET_QUEUE) \
	&& !defined(DCS_TESTBED_NETSNIF_USE_BOOST_LOCKFREE_SPSC_PACKET_QUEUE) \
	&& !defined(DCS_TESTBED_NETSNIF_USE_DCS_CONCURRENT_BLOCKING_PACKET_QUEUE)
# error "Don't know what type of packet queue to use."
#endif // DCS_TESTBED_USE_*_PACKET_QUEUE

#if    !defined(DCS_TESTBED_NETSNIF_USE_RAM_DATA_STORE) \
	&& !defined(DCS_TESTBED_NETSNIF_USE_MYSQL_DATA_STORE) \
	&& !defined(DCS_TESTBED_NETSNIF_USE_SQLITE_DATA_STORE)
# error "Don't know what type of data store to use."
#endif // DCS_TESTBED_NETSNIF_USE_*_DATA_STORE


#include <boost/atomic.hpp>
#include <boost/ref.hpp>
#include <boost/smart_ptr.hpp>
#include <boost/thread.hpp>
#ifdef DCS_TESTBED_NETSNIF_USE_BOOST_THREAD_SYNC_PACKET_QUEUE
# include <boost/thread/sync_queue.hpp>
#elif DCS_TESTBED_NETSNIF_USE_BOOST_LOCKFREE_MWMR_PACKET_QUEUE
# include <boost/lockfree/queue.hpp>
#elif DCS_TESTBED_NETSNIF_USE_BOOST_LOCKFREE_SPSC_PACKET_QUEUE
# include <boost/lockfree/spsc_queue.hpp>
#elif DCS_TESTBED_NETSNIF_USE_DCS_CONCURRENT_BLOCKING_PACKET_QUEUE
# include <dcs/concurrent/blocking_queue.hpp>
#endif // DCS_TESTBED_USE_*_PACKET_QUEUE
#ifdef DCS_TESTBED_NETSNIF_USE_MYSQL_DATA_STORE
# include <cppconn/connection.h>
# include <cppconn/driver.h>
# include <cppconn/exception.h>
# include <cppconn/resultset.h>
# include <cppconn/statement.h>
#endif // DCS_TESTBED_NETSNIF_USE_MYSQL_DATA_STORE
#ifdef DCS_TESTBED_NETSNIF_USE_RAM_DATA_STORE
# include <cstring>
#endif // DCS_TESTBED_NETSNIF_USE_RAM_DATA_STORE
#include <dcs/cli.hpp>
#include <dcs/debug.hpp>
#ifdef DCS_TESTBED_NETSNIF_USE_RAM_DATA_STORE
# include <dcs/digest/md5.hpp>
#endif // DCS_TESTBED_NETSNIF_USE_RAM_DATA_STORE
#include <dcs/exception.hpp>
#include <dcs/logging.hpp>
#include <dcs/macro.hpp>
#include <dcs/network/ethernet.hpp>
#include <dcs/network/ip.hpp>
#include <dcs/network/pcap.hpp>
#include <dcs/network/tcp.hpp>
#include <dcs/uri.hpp>
#include <exception>
#include <iostream>
#ifdef DCS_TESTBED_NETSNIF_USE_RAM_DATA_STORE
# include <map>
#endif // DCS_TESTBED_NETSNIF_USE_RAM_DATA_STORE
#ifdef DCS_TESTBED_NETSNIF_USE_MYSQL_DATA_STORE
# include <mysql_connection.h>
# include <mysql_driver.h>
#endif // DCS_TESTBED_NETSNIF_USE_MYSQL_DATA_STORE
#include <netdb.h>
#ifdef DCS_TESTBED_NETSNIF_USE_SQLITE_DATA_STORE
# include <sqlite3.h>
#endif // DCS_TESTBED_NETSNIF_USE_SQLITE_DATA_STORE
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
	closed_connection_status
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
										  ::boost::uint16_t server_port) = 0;

	virtual unsigned long num_connections(::std::string const& server_address,
										  ::boost::uint16_t server_port,
										  connection_status_category status) = 0;

	virtual void begin_transaction() = 0;

	virtual void commit_transaction() = 0;

	virtual void rollback_transaction() = 0;

	virtual bool is_open() const = 0;

	virtual void close() = 0;
}; // base_data_store


#ifdef DCS_TESTBED_NETSNIF_USE_SQLITE_DATA_STORE

class sqlite_data_store: public base_data_store
{
	public: sqlite_data_store()
	: p_db_(0)
	{
	}

	public: sqlite_data_store(::std::string const& db_name)
	: name_(db_name),
	  p_db_(0)
	{
	}

	public: ~sqlite_data_store()
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

		::std::ostringstream sql_oss;

		// Create tables (if needed)
		sql_oss << "CREATE TABLE IF NOT EXISTS network_connection ("
				<< "  server_addr TEXT DEFAULT ''"
				<< ", server_port INTEGER DEFAULT 0"
				<< ", client_addr TEXT DEFAULT ''"
				<< ", client_port INTEGER DEFAULT 0"
				<< ", status INTEGER DEFAULT 0"
				<< ", last_update TEXT DEFAULT (datetime('now'))"
				<< ", CONSTRAINT pk_nc_srv_cli PRIMARY KEY (server_addr,server_port,client_addr,client_port)"
				<< ")";
		ret = sqlite3_exec(p_db_, sql_oss.str().c_str(), 0, 0, 0);
		if (ret != SQLITE_OK)
		{
			::std::ostringstream oss;
			oss << "Unable to create table 'network_connection': " << ::sqlite3_errmsg(p_db_);

			DCS_EXCEPTION_THROW(::std::runtime_error, oss.str());
		}
		sql_oss.str("");
		sql_oss << "CREATE TABLE IF NOT EXISTS network_connection_stat ("
				<< "  server_addr TEXT DEFAULT ''"
				<< ", server_port INTEGER DEFAULT 0"
				<< ", num_arrivals INTEGER DEFAULT 0"
				<< ", num_departures INTEGER DEFAULT 0"
				<< ", last_update TEXT DEFAULT (datetime('now'))"
				<< ", CONSTRAINT pk_ncs_srv PRIMARY KEY (server_addr,server_port)"
				<< ")";
		ret = sqlite3_exec(p_db_, sql_oss.str().c_str(), 0, 0, 0);
		if (ret != SQLITE_OK)
		{
			::std::ostringstream oss;
			oss << "Unable to create table 'network_connection_stat': " << ::sqlite3_errmsg(p_db_);

			DCS_EXCEPTION_THROW(::std::runtime_error, oss.str());
		}
		// Create indices (if needed)
		sql_oss.str("");
		sql_oss << "CREATE INDEX IF NOT EXISTS idx_nc_srv ON network_connection (server_addr,server_port)";
		ret = sqlite3_exec(p_db_, sql_oss.str().c_str(), 0, 0, 0);
		if (ret != SQLITE_OK)
		{
			::std::ostringstream oss;
			oss << "Unable to create index 'idx_nc_srv': " << ::sqlite3_errmsg(p_db_);

			DCS_EXCEPTION_THROW(::std::runtime_error, oss.str());
		}
//		sql_oss.str("");
//		sql_oss << "CREATE INDEX IF NOT EXISTS idx_ncs_srv ON network_connection_stat (server_addr,server_port)";
//		ret = sqlite3_exec(p_db_, sql_oss.str().c_str(), 0, 0, 0);
//		if (ret != SQLITE_OK)
//		{
//			::std::ostringstream oss;
//			oss << "Unable to create index 'idx_ncs_srv': " << ::sqlite3_errmsg(p_db_);
//
//			DCS_EXCEPTION_THROW(::std::runtime_error, oss.str());
//		}
	}

	public: void clear()
	{
		DCS_ASSERT(this->is_open(),
				   DCS_EXCEPTION_THROW(::std::logic_error,
									   "DB is not open"));

		int ret(SQLITE_OK);
		::std::ostringstream sql_oss;

		sql_oss <<  "DELETE FROM network_connection";
		ret = sqlite3_exec(p_db_, sql_oss.str().c_str(), 0, 0, 0);
		if (ret != SQLITE_OK)
		{
			::std::ostringstream oss;
			oss << "Unable to clear table 'network_connection': " << ::sqlite3_errmsg(p_db_);

			DCS_EXCEPTION_THROW(::std::runtime_error, oss.str());
		}
		sql_oss.str("");
		sql_oss <<  "DELETE FROM network_connection_stat";
		ret = sqlite3_exec(p_db_, sql_oss.str().c_str(), 0, 0, 0);
		if (ret != SQLITE_OK)
		{
			::std::ostringstream oss;
			oss << "Unable to clear table 'network_connection_stat': " << ::sqlite3_errmsg(p_db_);

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

		::std::ostringstream sql_oss;
		sql_oss << "SELECT status,last_update"
				<< " FROM network_connection"
				<< " WHERE server_addr=%Q"
				<< " AND   server_port=%u"
				<< " AND   client_addr=%Q"
				<< " AND   client_port=%u";

		char* sql = ::sqlite3_mprintf(sql_oss.str().c_str(),
									  server_address.c_str(),
									  server_port,
									  client_address.c_str(),
									  client_port);

		DCS_DEBUG_TRACE("-- SQL: " << sql);//XXX

		network_connection* p_conn = new network_connection();
		p_conn->status = unknown_connection_status;

		int ret(SQLITE_OK);
		ret = sqlite3_exec(p_db_, sql, &load_callback, p_conn, 0);
		if (ret != SQLITE_OK)
		{
			::std::ostringstream oss;
			oss << "Unable to load (" << server_address << ":" << server_port << "," << client_address << ":" << client_port << ") from table 'network_connection': " << ::sqlite3_errmsg(p_db_);
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

		::std::ostringstream sql_oss;
		sql_oss << "INSERT OR REPLACE INTO network_connection"
				<< " (server_addr,server_port,client_addr,client_port,status)"
				<< " VALUES (%Q,%u,%Q,%u,%lu))";

		char* sql = ::sqlite3_mprintf(sql_oss.str().c_str(),
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
			oss << "Unable to save (" << conn.server_address << ":" << conn.server_port << "," << conn.client_address << ":" << conn.client_port << ") into table 'network_connection': " << ::sqlite3_errmsg(p_db_);
			::sqlite3_free(sql);

			DCS_EXCEPTION_THROW(::std::runtime_error, oss.str());
		}

		::sqlite3_free(sql);

		sql_oss.str("");
		int nr = ::sqlite3_changes(p_db_);
		if (nr == 1 || conn.status == closed_connection_status)
		{
			// The following emulate the 'INSERT INTO ... ON DUPLICATE KEY UPDATE' MySQL extension
			sql_oss << "INSERT OR IGNORE INTO network_connection_stat"
					<< " (server_addr,server_port)"
					<< " VALUES (%Q,%u));"
					<< "UPDATE network_connection_stat"
					<< " SET num_departures=num_departures+1"
					<< " WHERE server_addr=%Q AND server_port=%u";

			sql = ::sqlite3_mprintf(sql_oss.str().c_str(),
									conn.server_address.c_str(),
									conn.server_port,
									conn.server_address.c_str(),
									conn.server_port);
		}
		else
		{
			// The following emulate the 'INSERT INTO ... ON DUPLICATE KEY UPDATE' MySQL extension
			sql_oss << "INSERT OR IGNORE INTO network_connection_stat"
					<< " (server_addr,server_port)"
					<< " VALUES (%Q,%u));"
					<< "UPDATE network_connection_stat"
					<< " SET num_arrivals=num_arrivals+1"
					<< " WHERE server_addr=%Q AND server_port=%u";

			sql = ::sqlite3_mprintf(sql_oss.str().c_str(),
									conn.server_address.c_str(),
									conn.server_port,
									conn.server_address.c_str(),
									conn.server_port);
		}

		DCS_DEBUG_TRACE("-- SQL: " << sql);//XXX

		ret = sqlite3_exec(p_db_, sql, 0, 0, 0);
		if (ret != SQLITE_OK)
		{
			::std::ostringstream oss;
			oss << "Unable to save (" << conn.server_address << ":" << conn.server_port << ") into table 'network_connection_stat': " << ::sqlite3_errmsg(p_db_);
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

		::std::ostringstream sql_oss;
		sql_oss << "DELETE FROM network_connection"
				<< " WHERE server_addr=%Q"
				<< " AND   server_port=%u"
				<< " AND   client_addr=%Q"
				<< " AND   client_port=%u";

		char* sql = ::sqlite3_mprintf(sql_oss.str().c_str(),
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
			oss << "Unable to erase (" << server_address << ":" << server_port << "," << client_address << ":" << client_port << ") from table 'network_connection': " << ::sqlite3_errmsg(p_db_);
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

		::std::ostringstream sql_oss;
		sql_oss << "SELECT COUNT(*) FROM network_connection"
				<< " WHERE server_addr=%Q"
				<< " AND   server_port=%u"
				<< " GROUP BY status"
				<< " HAVING status=%d";

		int ret(SQLITE_OK);
		char* sql = ::sqlite3_mprintf(sql_oss.str().c_str(),
									  server_address.c_str(),
									  server_port,
									  status);

		DCS_DEBUG_TRACE("-- SQL: " << sql);//XXX

		unsigned long count(0);
		ret = sqlite3_exec(p_db_, sql, &num_connections_by_status_callback, &count, 0);
		if (ret != SQLITE_OK)
		{
			::std::ostringstream oss;
			oss << "Unable to count connections (" << server_address << ":" << server_port << ") from table 'network_connection': " << ::sqlite3_errmsg(p_db_);
			::sqlite3_free(sql);

			DCS_EXCEPTION_THROW(::std::runtime_error, oss.str());
		}

		::sqlite3_free(sql);

		return count;
	}

	public: unsigned long num_connections(::std::string const& server_address,
										  ::boost::uint16_t server_port)
	{
		DCS_ASSERT(this->is_open(),
				   DCS_EXCEPTION_THROW(::std::logic_error,
									   "DB is not open"));

		::std::ostringstream sql_oss;
		sql_oss << "SELECT num_arrivals-num_departures"
				<< " FROM network_connection_stat"
				<< " WHERE server_addr=%Q"
				<< " AND   server_port=%u";

		int ret(SQLITE_OK);
		char* sql = ::sqlite3_mprintf(sql_oss.str().c_str(),
									  server_address.c_str(),
									  server_port);

		DCS_DEBUG_TRACE("-- SQL: " << sql);//XXX

		unsigned long count(0);

		ret = sqlite3_exec(p_db_, sql, &num_connections_callback, &count, 0);
		if (ret != SQLITE_OK)
		{
			::std::ostringstream oss;
			oss << "Unable to count connections (" << server_address << ":" << server_port << ") from table 'network_connection_stat': " << ::sqlite3_errmsg(p_db_);
			::sqlite3_free(sql);

			DCS_EXCEPTION_THROW(::std::runtime_error, oss.str());
		}

		::sqlite3_free(sql);

		return count;
	}

	private: static int load_callback(void* user_data, int num_cols, char** col_values, char** col_names)
	{
		DCS_MACRO_SUPPRESS_UNUSED_VARIABLE_WARNING( num_cols );
		DCS_MACRO_SUPPRESS_UNUSED_VARIABLE_WARNING( col_names );

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

	private: static int num_connections_by_status_callback(void* user_data, int num_cols, char** col_values, char** col_names)
	{
		DCS_MACRO_SUPPRESS_UNUSED_VARIABLE_WARNING( num_cols );
		DCS_MACRO_SUPPRESS_UNUSED_VARIABLE_WARNING( col_names );

		DCS_DEBUG_ASSERT( user_data );

		unsigned long* p_count = static_cast<unsigned long*>(user_data);
		::std::istringstream iss;
		iss.str(col_values[0]);
		iss >> *p_count;

		return 0;
	}

	private: static int num_connections_callback(void* user_data, int num_cols, char** col_values, char** col_names)
	{
		DCS_MACRO_SUPPRESS_UNUSED_VARIABLE_WARNING( num_cols );
		DCS_MACRO_SUPPRESS_UNUSED_VARIABLE_WARNING( col_names );

		DCS_DEBUG_ASSERT( user_data );

		unsigned long* p_count = static_cast<unsigned long*>(user_data);
		::std::istringstream iss;
		iss.str(col_values[0]);
		iss >> *p_count;

		return 0;
	}


	private: ::std::string name_;
	private: ::sqlite3* p_db_;
}; // sqlite_data_store

#endif // DCS_TESTBED_NETSNIF_USE_SQLITE_DATA_STORE

#ifdef DCS_TESTBED_NETSNIF_USE_MYSQL_DATA_STORE

class mysql_data_store: public base_data_store
{
	public: mysql_data_store()
	{
	}

	public: mysql_data_store(::std::string const& host_uri, ::std::string const& db_name)
	: uri_(host_uri),
	  db_name_(db_name)
	{
	}

	public: mysql_data_store(::std::string const& host_uri, ::std::string const& db_name, ::std::string const& user, ::std::string const& passwd)
	: uri_(host_uri),
	  db_name_(db_name),
	  user_(user),
	  passwd_(passwd)
	{
	}

	public: ~mysql_data_store()
	{
		this->close();
	}

	public: void open()
	{
		this->close();

		try
		{
			// Open the DB
			::sql::mysql::MySQL_Driver* p_driver = ::sql::mysql::get_driver_instance();
			p_db_ = ::boost::shared_ptr< ::sql::Connection >(p_driver->connect(uri_, user_, passwd_));
			p_db_->setSchema(db_name_);

			::std::ostringstream sql_oss;
			::boost::scoped_ptr< ::sql::Statement > p_stmt;

			// Create tables (if needed)
			sql_oss << "CREATE TABLE IF NOT EXISTS network_connection ("
					<< "  server_addr VARCHAR(255) DEFAULT ''"
					<< ", server_port SMALLINT UNSIGNED DEFAULT 0"
					<< ", client_addr VARCHAR(255) DEFAULT ''"
					<< ", client_port SMALLINT UNSIGNED DEFAULT 0"
					<< ", status TINYINT DEFAULT 0"
					<< ", last_update TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP"
					<< ", CONSTRAINT pk_nc_srv_cli PRIMARY KEY (server_addr,server_port,client_addr,client_port)"
					<< ", INDEX idx_nc_srv (server_addr,server_port)"
					<< ")";
			p_stmt.reset(p_db_->createStatement());
			p_stmt->execute(sql_oss.str());
			sql_oss.str("");
			sql_oss << "CREATE TABLE IF NOT EXISTS network_connection_stat ("
					<< "  server_addr VARCHAR(255) DEFAULT ''"
					<< ", server_port SMALLINT UNSIGNED DEFAULT 0"
					<< ", num_arrivals INTEGER UNSIGNED DEFAULT 0"
					<< ", num_departures INTEGER UNSIGNED DEFAULT 0"
					<< ", last_update TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP"
					<< ", CONSTRAINT pk_ncs_srv PRIMARY KEY (server_addr,server_port)"
					<< ", INDEX idx_ncs_srv (server_addr,server_port)"
					<< ")";
			p_stmt.reset(p_db_->createStatement());
			p_stmt->execute(sql_oss.str());
		
			//// Delete tables
			//sql_oss.str("");
			//sql_oss << "DELETE FROM network_connection";
			//p_stmt.reset(p_db_->createStatement());
			//p_stmt->execute(sql_oss.str());
			//sql_oss.str("");
			//sql_oss << "DELETE FROM network_connection_stat";
			//p_stmt.reset(p_db_->createStatement());
			//p_stmt->execute(sql_oss.str());
		}
		catch (::sql::SQLException const& e)
		{
			::std::ostringstream oss;
			oss << "Unable to open DB: " << e.what() << " (MySQL Code: " << e.getErrorCode() << ", state: " << e.getSQLState() << ")";

			DCS_EXCEPTION_THROW(::std::runtime_error, oss.str());
		}
		catch (::std::runtime_error const& e)
		{
			::std::ostringstream oss;
			oss << "Unable to open DB: " << e.what();

			DCS_EXCEPTION_THROW(::std::runtime_error, oss.str());
		}
	}

	public: void clear()
	{
		DCS_ASSERT(this->is_open(),
				   DCS_EXCEPTION_THROW(::std::logic_error,
									   "DB is not open"));

		try
		{
			::std::ostringstream sql_oss;
			::boost::scoped_ptr< ::sql::Statement > p_stmt;

			sql_oss << "DELETE FROM network_connection";
			p_stmt.reset(p_db_->createStatement());
			p_stmt->execute(sql_oss.str());
			sql_oss.str("");
			sql_oss << "DELETE FROM network_connection_stat";
			p_stmt.reset(p_db_->createStatement());
			p_stmt->execute(sql_oss.str());
		}
		catch (::sql::SQLException const& e)
		{
			::std::ostringstream oss;
			oss << "Unable to clear DB: " << e.what() << " (MySQL Code: " << e.getErrorCode() << ", state: " << e.getSQLState() << ")";

			DCS_EXCEPTION_THROW(::std::runtime_error, oss.str());
		}
		catch (::std::runtime_error const& e)
		{
			::std::ostringstream oss;
			oss << "Unable to clear DB: " << e.what();

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

		::std::ostringstream sql_oss;
		sql_oss << "SELECT status,last_update"
				<< " FROM network_connection"
				<< " WHERE server_addr='" << this->escape_for_db(server_address) << "'"
				<< " AND   server_port=" << server_port
				<< " AND   client_addr='" << this->escape_for_db(client_address) << "'"
				<< " AND   client_port=" << client_port;

		DCS_DEBUG_TRACE("-- SQL: " << sql_oss.str());//XXX

		::boost::shared_ptr<network_connection> p_net_conn;

		try
		{
			::boost::scoped_ptr< ::sql::Statement > p_stmt(p_db_->createStatement());
			::boost::scoped_ptr< ::sql::ResultSet > p_res(p_stmt->executeQuery(sql_oss.str()));

			p_net_conn = ::boost::make_shared<network_connection>();

			if (p_res->rowsCount() > 0)
			{
				if (p_res->rowsCount() != 1)
				{
					::std::ostringstream oss;
					oss << "Expected 1 row, got " << p_res->rowsCount();
					DCS_EXCEPTION_THROW(::std::runtime_error, oss.str());
				}

				p_res->next();

				p_net_conn->status = static_cast<connection_status_category>(p_res->getInt("status"));
			}
			else
			{
				p_net_conn->status = unknown_connection_status;
			}
			p_net_conn->server_address = server_address;
			p_net_conn->server_port = server_port;
			p_net_conn->client_address = client_address;
			p_net_conn->client_port = client_port;
		}
		catch (::sql::SQLException const& e)
		{
			::std::ostringstream oss;
			oss << "Unable to load (" << server_address << ":" << server_port << "," << client_address << ":" << client_port << ") from table 'network_connection': " << e.what() << " (MySQL Code: " << e.getErrorCode() << ", state: " << e.getSQLState() << ")";

			DCS_EXCEPTION_THROW(::std::runtime_error, oss.str());
		}
		catch (::std::runtime_error const& e)
		{
			::std::ostringstream oss;
			oss << "Unable to load (" << server_address << ":" << server_port << "," << client_address << ":" << client_port << ") from table 'network_connection': " << e.what();

			DCS_EXCEPTION_THROW(::std::runtime_error, oss.str());
		}

		return p_net_conn;
	}

	public: void save(network_connection const& conn)
	{
		DCS_ASSERT(this->is_open(),
				   DCS_EXCEPTION_THROW(::std::logic_error,
									   "DB is not open"));

		try
		{
			::std::ostringstream sql_oss;

			this->begin_transaction();

				::boost::scoped_ptr< ::sql::Statement > p_stmt;
				int nr;

				// Add/Update connection status
				sql_oss << "REPLACE INTO network_connection"
						<< " (server_addr,server_port,client_addr,client_port,status)"
						<< " VALUES ('" << this->escape_for_db(conn.server_address) << "'"
						<<          "," << conn.server_port
						<<          ",'" << this->escape_for_db(conn.client_address) << "'"
						<<          "," << conn.client_port
						<<          "," << static_cast<int>(conn.status)
						<<          ")";
				DCS_DEBUG_TRACE("-- SQL: " << sql_oss.str());//XXX
				p_stmt.reset(p_db_->createStatement());
				nr = p_stmt->executeUpdate(sql_oss.str());

				// Add/Update statistics only in case of a new insertion
				if (nr == 1 || conn.status == closed_connection_status)
				{
					sql_oss.str("");
					if (conn.status == closed_connection_status)
					{
						sql_oss << "INSERT INTO network_connection_stat"
								<< " (server_addr,server_port,num_arrivals,num_departures)"
								<< " VALUES ('" << this->escape_for_db(conn.server_address) << "'"
								<<          "," << conn.server_port
								<<          ",0"
								<<          ",1"
								<<          ")"
								<< " ON DUPLICATE KEY UPDATE num_departures=num_departures+1";
					}
					else
					{
						sql_oss << "INSERT INTO network_connection_stat"
								<< " (server_addr,server_port,num_arrivals,num_departures)"
								<< " VALUES ('" << this->escape_for_db(conn.server_address) << "'"
								<<          "," << conn.server_port
								<<          ",1"
								<<          ",0"
								<<          ")"
								<< " ON DUPLICATE KEY UPDATE num_arrivals=num_arrivals+1";
					}
					DCS_DEBUG_TRACE("-- SQL: " << sql_oss.str());//XXX
					p_stmt.reset(p_db_->createStatement());
					p_stmt->executeUpdate(sql_oss.str());
				}

			this->commit_transaction();
		}
		catch (::sql::SQLException const& e)
		{
			this->rollback_transaction();

			::std::ostringstream oss;
			oss << "Unable to save (" << conn.server_address << ":" << conn.server_port << "," << conn.client_address << ":" << conn.client_port << "): " << e.what() << " (MySQL Code: " << e.getErrorCode() << ", state: " << e.getSQLState() << ")";

			DCS_EXCEPTION_THROW(::std::runtime_error, oss.str());
		}
		catch (::std::runtime_error const& e)
		{
			this->rollback_transaction();

			::std::ostringstream oss;
			oss << "Unable to save (" << conn.server_address << ":" << conn.server_port << "," << conn.client_address << ":" << conn.client_port << "): " << e.what();

			DCS_EXCEPTION_THROW(::std::runtime_error, oss.str());
		}
	}

	public: void erase(::std::string const& server_address,
					   ::boost::uint16_t server_port,
					   ::std::string const& client_address,
					   ::boost::uint16_t client_port)
	{
		DCS_ASSERT(this->is_open(),
				   DCS_EXCEPTION_THROW(::std::logic_error,
									   "DB is not open"));

		try
		{
			::std::ostringstream sql_oss;

			this->begin_transaction();

				sql_oss << "DELETE FROM network_connection"
						<< " WHERE server_addr='" << this->escape_for_db(server_address) << "'"
						<< " AND   server_port=" << server_port
						<< " AND   client_addr='" << this->escape_for_db(client_address) << "'"
						<< " AND   client_port=" << client_port;
				DCS_DEBUG_TRACE("-- SQL: " << sql_oss.str());//XXX
				::boost::scoped_ptr< ::sql::Statement > p_stmt(p_db_->createStatement());
				p_stmt->execute(sql_oss.str());

//				// Add/Update statistics
//				sql_oss.str("");
//				sql_oss << "INSERT INTO network_connection_stat"
//						<< " (server_addr,server_port,num_arrivals,num_departures)"
//						<< " VALUES ('" << server_address << "'"
//						<<          "," << server_port
//						<<          ",0"
//						<<          ",1"
//						<<          ")"
//						<< " ON DUPLICATE KEY UPDATE num_departures=num_departures+1";
//				DCS_DEBUG_TRACE("-- SQL: " << sql_oss.str());//XXX
//				p_stmt.reset(p_db_->createStatement());
//				p_stmt->executeUpdate(sql_oss.str());

			this->commit_transaction();
		}
		catch (::sql::SQLException const& e)
		{
			this->rollback_transaction();

			::std::ostringstream oss;
			oss << "Unable to erase (" << server_address << ":" << server_port << "," << client_address << ":" << client_port << ") into table 'network_connection': " << e.what() << " (MySQL Code: " << e.getErrorCode() << ", state: " << e.getSQLState() << ")";

			DCS_EXCEPTION_THROW(::std::runtime_error, oss.str());
		}
		catch (::std::runtime_error const& e)
		{
			this->rollback_transaction();

			::std::ostringstream oss;
			oss << "Unable to erase (" << server_address << ":" << server_port << "," << client_address << ":" << client_port << ") into table 'network_connection': " << e.what();

			DCS_EXCEPTION_THROW(::std::runtime_error, oss.str());
		}
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

		//p_db_->setAutoCommit(false);
		::boost::scoped_ptr< ::sql::Statement > p_stmt(p_db_->createStatement());
		p_stmt->execute("START TRANSACTION");
	}

	public: void commit_transaction()
	{
		DCS_ASSERT(this->is_open(),
				   DCS_EXCEPTION_THROW(::std::logic_error,
									   "DB is not open"));

		//DCS_DEBUG_TRACE("AutoCommit before Commit: " << p_db_->getAutoCommit());
		//p_db_->commit();
		//p_db_->setAutoCommit(true);
		////FIXME: should we re-enable AutoCommit mode?
		//DCS_DEBUG_TRACE("AutoCommit after Commit: " << p_db_->getAutoCommit());
		::boost::scoped_ptr< ::sql::Statement > p_stmt(p_db_->createStatement());
		p_stmt->execute("COMMIT");
	}

	public: void rollback_transaction()
	{
		DCS_ASSERT(this->is_open(),
				   DCS_EXCEPTION_THROW(::std::logic_error,
									   "DB is not open"));

		//DCS_DEBUG_TRACE("AutoCommit before Rollback: " << p_db_->getAutoCommit());
		//p_db_->rollback();
		//p_db_->setAutoCommit(true);
		////FIXME: should we re-enable AutoCommit mode?
		//DCS_DEBUG_TRACE("AutoCommit after Rollback: " << p_db_->getAutoCommit());
		::boost::scoped_ptr< ::sql::Statement > p_stmt(p_db_->createStatement());
		p_stmt->execute("ROLLBACK");
	}

	public: void close()
	{
		if (this->is_open())
		{
			p_db_->close();
			p_db_.reset();
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

		::std::ostringstream sql_oss;
		sql_oss << "SELECT COUNT(*)"
				<< " FROM network_connection"
				<< " WHERE server_addr='" << this->escape_for_db(server_address) << "'"
				<< " AND   server_port=" << server_port
				<< " GROUP BY status"
				<< " HAVING   status=" << static_cast<int>(status);

		DCS_DEBUG_TRACE("-- SQL: " << sql_oss.str());//XXX

		unsigned long count(0);

		try
		{
			::boost::scoped_ptr< ::sql::Statement > p_stmt(p_db_->createStatement());
			::boost::scoped_ptr< ::sql::ResultSet > p_res(p_stmt->executeQuery(sql_oss.str()));

			if (p_res->rowsCount() >= 1)
			{
				if (p_res->rowsCount() != 1)
				{
					::std::ostringstream oss;
					oss << "Expected 1 row, got " << p_res->rowsCount();
					DCS_EXCEPTION_THROW(::std::runtime_error, oss.str());
				}

				p_res->next();

				count = p_res->getUInt(1);
			}
		}
		catch (::sql::SQLException const& e)
		{
			::std::ostringstream oss;
			oss << "Unable to count connections (" << server_address << ":" << server_port << ") with status '" << status << "': " << e.what() << " (MySQL Code: " << e.getErrorCode() << ", state: " << e.getSQLState() << ")";

			DCS_EXCEPTION_THROW(::std::runtime_error, oss.str());
		}
		catch (::std::runtime_error const& e)
		{
			::std::ostringstream oss;
			oss << "Unable to count connections (" << server_address << ":" << server_port << ") with status '" << status << "': " << e.what();

			DCS_EXCEPTION_THROW(::std::runtime_error, oss.str());
		}

		return count;
	}

	public: unsigned long num_connections(::std::string const& server_address,
										  ::boost::uint16_t server_port)
	{
		DCS_ASSERT(this->is_open(),
				   DCS_EXCEPTION_THROW(::std::logic_error,
									   "DB is not open"));

		::std::ostringstream sql_oss;
		sql_oss << "SELECT num_arrivals-num_departures"
				<< " FROM network_connection_stat"
				<< " WHERE server_addr='" << this->escape_for_db(server_address) << "'"
				<< " AND   server_port=" << server_port;

		DCS_DEBUG_TRACE("-- SQL: " << sql_oss.str());//XXX

		unsigned long count(0);

		try
		{
			::boost::scoped_ptr< ::sql::Statement > p_stmt(p_db_->createStatement());
			::boost::scoped_ptr< ::sql::ResultSet > p_res(p_stmt->executeQuery(sql_oss.str()));

			if (p_res->rowsCount() > 1)
			{
				if (p_res->rowsCount() != 1)
				{
					::std::ostringstream oss;
					oss << "Expected 1 row, got " << p_res->rowsCount();
					DCS_EXCEPTION_THROW(::std::runtime_error, oss.str());
				}

				p_res->next();

				count = p_res->getUInt(1);
			}
		}
		catch (::sql::SQLException const& e)
		{
			::std::ostringstream oss;
			oss << "Unable to count connections (" << server_address << ":" << server_port << "): " << e.what() << " (MySQL Code: " << e.getErrorCode() << ", state: " << e.getSQLState() << ")";

			DCS_EXCEPTION_THROW(::std::runtime_error, oss.str());
		}
		catch (::std::runtime_error const& e)
		{
			::std::ostringstream oss;
			oss << "Unable to count connections (" << server_address << ":" << server_port << "): " << e.what();

			DCS_EXCEPTION_THROW(::std::runtime_error, oss.str());
		}

		return count;
	}

	private: ::std::string escape_for_db(::std::string const& s)
	{
		DCS_ASSERT(this->is_open(),
				   DCS_EXCEPTION_THROW(::std::logic_error,
									   "DB is not open"));

		::sql::mysql::MySQL_Connection* p_mysql_conn = dynamic_cast< ::sql::mysql::MySQL_Connection* >(p_db_.get());
		if (!p_mysql_conn)
		{
			DCS_EXCEPTION_THROW(::std::runtime_error,
								"Unable to cast a sql::Connection object into a sql::mysql:MySQL_Connection one");
		}

		return p_mysql_conn->escapeString(s);
	}


	private: ::std::string uri_;
	private: ::std::string db_name_;
	private: ::std::string user_;
	private: ::std::string passwd_;
	private: ::boost::shared_ptr< ::sql::Connection > p_db_;
}; // mysql_data_store

#endif // DCS_TESTBED_NETSNIF_USE_MYSQL_DATA_STORE


#ifdef DCS_TESTBED_NETSNIF_USE_RAM_DATA_STORE

class ram_data_store: public base_data_store
{
	private: typedef ::boost::mutex mutex_type;
	private: typedef ::std::map< ::std::string, ::boost::shared_ptr<network_connection> > client_entry_container;
	private: typedef ::std::map< ::std::string, client_entry_container > server_entry_container;


	public: ram_data_store()
	{
	}

	public: ~ram_data_store()
	{
		this->close();
	}

	public: void open()
	{
		this->close();
	}

	public: void clear()
	{
		DCS_ASSERT(this->is_open(),
				   DCS_EXCEPTION_THROW(::std::logic_error,
									   "DB is not open"));

		store_.clear();
	}

	public: ::boost::shared_ptr<network_connection> load(::std::string const& server_address,
														 ::boost::uint16_t server_port,
														 ::std::string const& client_address,
														 ::boost::uint16_t client_port)
	{
		::boost::shared_ptr<network_connection> p_net_conn;
		p_net_conn = ::boost::make_shared<network_connection>();
		p_net_conn->server_address = server_address;
		p_net_conn->server_port = server_port;
		p_net_conn->client_address = client_address;
		p_net_conn->client_port = client_port;

		::std::string srv_id = make_id(server_address, server_port);
		::std::string cli_id = make_id(client_address, client_port);

		::boost::lock_guard<mutex_type> lock(store_mutex_);

		if (store_.count(srv_id) > 0 && store_.at(srv_id).count(cli_id) > 0)
		{
			p_net_conn->status = store_.at(srv_id).at(cli_id)->status;
		}
		else
		{
			p_net_conn->status = unknown_connection_status;
		}

		return p_net_conn;
	}

	public: void save(network_connection const& conn)
	{
		::std::string srv_id = make_id(conn.server_address, conn.server_port);
		::std::string cli_id = make_id(conn.client_address, conn.client_port);

		::boost::lock_guard<mutex_type> lock(store_mutex_);

		if (store_.count(srv_id) == 0)
		{
			store_[srv_id] = client_entry_container();
		}
		if (store_.at(srv_id).count(cli_id) == 0)
		{
			store_[srv_id][cli_id] = ::boost::make_shared<network_connection>();
		}
		store_[srv_id][cli_id]->server_address = conn.server_address;
		store_[srv_id][cli_id]->server_port = conn.server_port;
		store_[srv_id][cli_id]->client_address = conn.client_address;
		store_[srv_id][cli_id]->client_port = conn.client_port;
		store_[srv_id][cli_id]->status = conn.status;
	}

	public: void erase(::std::string const& server_address,
					   ::boost::uint16_t server_port,
					   ::std::string const& client_address,
					   ::boost::uint16_t client_port)
	{
		::std::string srv_id = make_id(server_address, server_port);
		::std::string cli_id = make_id(client_address, client_port);

		::boost::lock_guard<mutex_type> lock(store_mutex_);

		if (store_.count(srv_id) > 0)
		{
			store_[srv_id].erase(cli_id);
			if (store_[srv_id].size() == 0)
			{
				store_.erase(srv_id);
			}
		}
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
		// Do nothing
	}

	public: void commit_transaction()
	{
		// Do nothing
	}

	public: void rollback_transaction()
	{
		// Do nothing
	}

	public: void close()
	{
		// Do nothing
	}

	public: bool is_open() const
	{
		return true;
	}

	public: unsigned long num_connections(::std::string const& server_address,
										  ::boost::uint16_t server_port,
										  connection_status_category status)
	{
		DCS_ASSERT(this->is_open(),
				   DCS_EXCEPTION_THROW(::std::logic_error,
									   "DB is not open"));

		unsigned long count(0);

		::std::string srv_id = make_id(server_address, server_port);

		::boost::lock_guard<mutex_type> lock(store_mutex_);

		if (store_.count(srv_id) > 0)
		{
			typedef client_entry_container::const_iterator entry_iterator;

			entry_iterator end_it(store_.at(srv_id).end());
			for (entry_iterator it = store_.at(srv_id).begin();
				 it != end_it;
				 ++it)
			{
				if (it->second->status == status)
				{
					++count;
				}
			}
		}

		return count;
	}

	public: unsigned long num_connections(::std::string const& server_address,
										  ::boost::uint16_t server_port)
	{
		throw ::std::runtime_error("Not yet implemented");
	}

	private: static ::std::string make_id(::std::string const& address,
										  ::boost::uint16_t port)
	{
		::std::ostringstream oss;
		oss << "<"
			<< address << ":" << port
			<< ">";
			
		::std::vector< ::dcs::digest::byte_type > digest = ::dcs::digest::md5_digest(oss.str());

		return ::dcs::digest::hex_string(digest.begin(), digest.end());
	}

	private: static ::std::string make_id(::std::string const& server_address,
										  ::boost::uint16_t server_port,
										  ::std::string const& client_address,
										  ::boost::uint16_t client_port)
	{
		::std::ostringstream oss;
		oss << "<"
			<< server_address << ":" << server_port
			<< ","
			<< client_address << ":" << client_port
			<< ">";
			
		::std::vector< ::dcs::digest::byte_type > digest = ::dcs::digest::md5_digest(oss.str());

		return ::dcs::digest::hex_string(digest.begin(), digest.end());
	}


	private: server_entry_container store_;
	private: mutex_type store_mutex_;
}; // ram_data_store

#endif // DCS_TESTBED_NETSNIF_USE_RAM_DATA_STORE


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
				oss << "Found connection status '" << p_conn->status << ": expected '" << wait_connection_status << "'";
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
				oss << "Found connection status '" << p_conn->status << ": expected '" << active_connection_status << "'";
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

	public: connection_status_category connection_status(::std::string const& server_address,
														 ::boost::uint16_t server_port,
														 ::std::string const& client_address,
														 ::boost::uint16_t client_port) const
	{
		::boost::shared_ptr<network_connection> p_conn = p_ds_->load(server_address,
																	 server_port,
																	 client_address,
																	 client_port);

		return p_conn->status;
	}

	public: unsigned long num_connections(::std::string const& server_address,
										  ::boost::uint16_t server_port) const
	{
		return p_ds_->num_connections(server_address, server_port);
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
//static const ::std::string default_db_file = "./sniffer.db";
static const ::std::string default_db_uri = "sniffer_db";
static const ::std::string default_device = "lo";


void usage(char const* progname)
{
	::std::cerr << "Usage: " << progname << " [options]" << ::std::endl
				<< " --db <URI>" << ::std::endl
				<< "   The URI to the database where packet information is stored." << ::std::endl
				<< "   The generic URI format is: protocol://host:port/dbname?param1=value1&param2=value2&...." << ::std::endl
				<< "   Typical parameters are the user name and password, for instance: tcp://127.0.0.1:3306/netsnifdb?user=foo&password=bar" << ::std::endl
				<< "   [default: '" << default_db_uri << "']." << ::std::endl
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


template <typename QueueT>
class batch_packet_handler: public ::dcs::network::pcap::sniffer_batch_packet_handler
{
	public: typedef QueueT packet_queue_type;


	public: batch_packet_handler(::std::string const& srv_address, ::boost::uint16_t srv_port, packet_queue_type* p_pkt_queue)
	:srv_address_(srv_address),
	 srv_port_(srv_port),
	 p_pkt_queue_(p_pkt_queue),
	 count_(0)
	{
	}

	public: void operator()(::boost::shared_ptr< ::dcs::network::pcap::raw_packet > const& p_pkt)
	{
#ifdef DCS_DEBUG
		++count_;
#endif // DCS_DEBUG

		//DCS_DEBUG_TRACE( "-[" << count_ << "] Grabbed new packet: " << *p_pkt );

		boost::shared_ptr<dcs::network::ethernet_frame> p_eth = dcs::network::pcap::make_ethernet_frame(*p_pkt);
		DCS_DEBUG_TRACE( "-[" << count_ << "] -> Ethernet frame: " << *p_eth );

		if (p_eth->ethertype_field() == ::dcs::network::ethernet_frame::ethertype_ipv4)
		{
			boost::shared_ptr<dcs::network::ip4_packet> p_ip = ::boost::make_shared<dcs::network::ip4_packet>(p_eth->payload(), p_eth->payload_size());
			DCS_DEBUG_TRACE( "-[" << count_ << "] -> IP packet: " << *p_ip );

			if (p_ip->protocol_field() == ::dcs::network::ip4_packet::proto_tcp)
			{
				boost::shared_ptr<dcs::network::tcp_segment> p_tcp = ::boost::make_shared<dcs::network::tcp_segment>(p_ip->payload(), p_ip->payload_size());
				DCS_DEBUG_TRACE( "-[" << count_ << "] -> TCP segment: " << *p_tcp );
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
						DCS_DEBUG_TRACE( "-[" << count_ << "] -> TCP payload: " << std::string(payload, payload_sz) );
					}
					else
					{
						DCS_DEBUG_TRACE( "-[" << count_ << "] -> TCP payload: <binary data>" );
					}
				}
#endif // DCS_DEBUG

#if defined(DCS_TESTBED_NETSNIF_USE_BOOST_THREAD_SYNC_PACKET_QUEUE)
				p_pkt_queue_->push(p_pkt);
#elif defined(DCS_TESTBED_NETSNIF_USE_BOOST_LOCKFREE_MWMR_PACKET_QUEUE)
				p_pkt_queue_->push(p_pkt);
#elif defined(DCS_TESTBED_NETSNIF_USE_BOOST_LOCKFREE_SPSC_PACKET_QUEUE)
				p_pkt_queue_->push(p_pkt);
#elif defined(DCS_TESTBED_NETSNIF_USE_DCS_CONCURRENT_BLOCKING_PACKET_QUEUE)
				p_pkt_queue_->push(p_pkt);
#endif // DCS_TESTBED_NETSNIF_USE_*_PACKET_QUEUE
			}
		}

		DCS_DEBUG_TRACE( "--------------------------------------------" );
	}


	private: ::std::string srv_address_;
	private: ::boost::uint16_t srv_port_;
	private: packet_queue_type* p_pkt_queue_;
	private: unsigned long count_;
}; // batch_packet_handler


struct dummy_batch_packet_handler: public ::dcs::network::pcap::sniffer_batch_packet_handler
{
	void operator()(::boost::shared_ptr< ::dcs::network::pcap::raw_packet > const& p_pkt)
	{
		(void)p_pkt;
	}
}; // dummy_batch_packet_handler


template <typename QueueT>
class packet_sniffer_runner
{
	public: typedef QueueT packet_queue_type
;


	public: packet_sniffer_runner(::std::string const& dev,
								  ::std::string const& srv_address,
								  ::boost::uint16_t srv_port,
								  ::boost::atomic<bool>* p_sniffer_done,
								  packet_queue_type* p_pkt_queue)
	: dev_(dev),
	  srv_address_(srv_address),
	  srv_port_(srv_port),
	  p_sniffer_done_(p_sniffer_done),
	  p_pkt_queue_(p_pkt_queue)
	{
	}

	public: void operator()()
	{
		// check: p_sniffer_done_ != null
		DCS_DEBUG_ASSERT( p_sniffer_done_ );
		// check: p_pkt_queue_ != null
		DCS_DEBUG_ASSERT( p_pkt_queue_ );

		// Open the device for sniffing
		::dcs::network::pcap::live_packet_sniffer sniffer(dev_);

		sniffer.snapshot_length(65535);
		sniffer.promiscuous_mode(true);
		sniffer.read_timeout(1000);

		//std::string filter_expr = "tcp[tcpflags] & (tcp-syn|tcp-ack|tcp-fin) != 0";
		//std::string filter_expr = "tcp[tcpflags]";
		::std::string filter_expr;
		{
			::std::ostringstream oss;
			//oss << "tcp[tcpflags] host " << srv_address_ << " port " << srv_port_;
			oss << "tcp and host " << srv_address_ << " and port " << srv_port_;
			filter_expr = oss.str();
		}

		sniffer.filter(filter_expr);

		batch_packet_handler<packet_queue_type> pkt_handler(srv_address_, srv_port_, p_pkt_queue_);
		//detail::dummy_batch_packet_handler pkt_handler;

		try
		{
			sniffer.batch_capture(pkt_handler);
		}
		catch (...)
		{
		}

		*p_sniffer_done_ = true;
	}


	private: ::std::string dev_;
	private: ::std::string srv_address_;
	private: ::boost::uint16_t srv_port_;
	private: ::boost::atomic<bool>* p_sniffer_done_;
	private: packet_queue_type* p_pkt_queue_;
}; // packet_sniffer_runner


template <typename QueueT>
class packet_analyzer_runner
{
	public: typedef QueueT packet_queue_type
;


	public: packet_analyzer_runner(::std::string const& srv_address,
								   ::boost::uint16_t srv_port,
								   network_connection_manager* p_conn_mgr,
								   ::boost::atomic<bool>* p_sniffer_done,
								   packet_queue_type* p_pkt_queue)
	: srv_address_(srv_address),
	  srv_port_(srv_port),
	  p_conn_mgr_(p_conn_mgr),
	  p_sniffer_done_(p_sniffer_done),
	  p_pkt_queue_(p_pkt_queue)
	{
	}

	public: void operator()()
	{
		// check: p_conn_mgr_ != null
		DCS_DEBUG_ASSERT( p_conn_mgr_ );
		// check: p_sniffer_done_ != null
		DCS_DEBUG_ASSERT( p_sniffer_done_ );
		// check: p_pkt_queue_ != null
		DCS_DEBUG_ASSERT( p_pkt_queue_ );

		::boost::shared_ptr< ::dcs::network::pcap::raw_packet > p_pkt;
		bool one_more_time(true);

		do
		{
			while (!*p_sniffer_done_ || !one_more_time)
			{
				if (one_more_time)
				{
#if defined(DCS_TESTBED_NETSNIF_USE_BOOST_THREAD_SYNC_PACKET_QUEUE)
					p_pkt_queue_->pull(p_pkt);
#elif defined(DCS_TESTBED_NETSNIF_USE_BOOST_LOCKFREE_MWMR_PACKET_QUEUE)
					p_pkt_queue_->pop(p_pkt);
#elif defined(DCS_TESTBED_NETSNIF_USE_BOOST_LOCKFREE_SPSC_PACKET_QUEUE)
					bool ok = p_pkt_queue_->pop(p_pkt);
					if (!ok)
					{
						continue;
					}
#elif defined(DCS_TESTBED_NETSNIF_USE_DCS_CONCURRENT_BLOCKING_PACKET_QUEUE)
					p_pkt_queue_->pop(p_pkt);
#endif // DCS_TESTBED_NETSNIF_USE_*_PACKET_QUEUE
				}
				else
				{
#if defined(DCS_TESTBED_NETSNIF_USE_BOOST_THREAD_SYNC_PACKET_QUEUE)
					bool ok = p_pkt_queue_->try_pull(p_pkt);
#elif defined(DCS_TESTBED_NETSNIF_USE_BOOST_LOCKFREE_MWMR_PACKET_QUEUE)
					bool ok = p_pkt_queue_->pop(p_pkt);
#elif defined(DCS_TESTBED_NETSNIF_USE_BOOST_LOCKFREE_SPSC_PACKET_QUEUE)
					bool ok = p_pkt_queue_->pop(p_pkt);
#elif defined(DCS_TESTBED_NETSNIF_USE_DCS_CONCURRENT_BLOCKING_PACKET_QUEUE)
					bool ok = p_pkt_queue_->try_pop(p_pkt);
#endif // DCS_TESTBED_NETSNIF_USE_*_PACKET_QUEUE
					if (!ok)
					{
						break;
					}
				}

				// check: null
				DCS_DEBUG_ASSERT( p_pkt );

				boost::shared_ptr<dcs::network::ip4_packet> p_ip;
				boost::shared_ptr<dcs::network::tcp_segment> p_tcp;
				boost::shared_ptr<dcs::network::ethernet_frame> p_eth = dcs::network::pcap::make_ethernet_frame(*p_pkt);
				if (p_eth->ethertype_field() == ::dcs::network::ethernet_frame::ethertype_ipv4)
				{
					p_ip = ::boost::make_shared<dcs::network::ip4_packet>(p_eth->payload(), p_eth->payload_size());
					p_tcp = ::boost::make_shared<dcs::network::tcp_segment>(p_ip->payload(), p_ip->payload_size());
				}
				else
				{
					// IPv6 not yet handled
					DCS_EXCEPTION_THROW(::std::runtime_error, "IPv6 packet analysis not yet implemented");
				}

				//::std::string srv_address;
				//::boost::uint16_t srv_port(0);
				::boost::uint16_t cli_port(0);

				std::string src_addr = host_address(p_ip->source_address());
				std::string dst_addr = host_address(p_ip->destination_address());

				if (src_addr == srv_address_ && p_tcp->source_port_field() == srv_port_)
				{
					// SERVER --> CLIENT

					if (p_tcp->payload_size() > 0)
					{
						DCS_DEBUG_TRACE( "TCP have PAYLOAD" );

						// The server sends back data to the client
						//srv_address_ = src_addr;
						//srv_port_ = p_tcp->source_port_field();
						::std::string cli_address = dst_addr;
						cli_port = p_tcp->destination_port_field();

						try
						{
							connection_status_category status = p_conn_mgr_->connection_status(srv_address_, srv_port_, cli_address, cli_port);
							if (wait_connection_status == status)
							{
								DCS_DEBUG_TRACE("END CONNECTION ESTABLISHMENT");//XXX
								p_conn_mgr_->end_connection_establishment(srv_address_, srv_port_, cli_address, cli_port);
							}
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
							//srv_address_ = src_addr;
							//srv_port_ = p_tcp->source_port_field();
							::std::string cli_address = dst_addr;
							cli_port = p_tcp->destination_port_field();
							DCS_DEBUG_TRACE("END CONNECTION TERMINATION");//XXX

							try
							{
								p_conn_mgr_->end_connection_termination(srv_address_, srv_port_, cli_address, cli_port);
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
				else if (dst_addr == srv_address_ && p_tcp->destination_port_field() == srv_port_)
				{
					// CLIENT --> SERVER

					if (p_tcp->have_flags(dcs::network::tcp_segment::flags_syn))
					{
						DCS_DEBUG_TRACE( "TCP have SYN" );

						// The client begins the 3-way handshake by sending SYN to the server
						::std::string cli_address = src_addr;
						cli_port = p_tcp->source_port_field();
						//srv_address_ = dst_addr;
						//srv_port_ = p_tcp->destination_port_field();
						DCS_DEBUG_TRACE("BEGIN CONNECTION ESTABLISHMENT");//XXX

						try
						{
							p_conn_mgr_->begin_connection_establishment(srv_address_, srv_port_, cli_address, cli_port);
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
						::std::string cli_address = src_addr;
						cli_port = p_tcp->source_port_field();
						//srv_address_ = dst_addr;
						//srv_port_ = p_tcp->destination_port_field();
						DCS_DEBUG_TRACE("BEGIN CONNECTION TERMINATION");//XXX

						try
						{
							p_conn_mgr_->begin_connection_termination(srv_address_, srv_port_, cli_address, cli_port);
						}
						catch (std::exception const& e)
						{
							std::ostringstream oss;
							oss << "Stats update for begin of connection termination: " << e.what();
							dcs::log_error(DCS_LOGGING_AT, oss.str());
						}
					}
	//						std::cout << ":: Num Waiting Connections for (" << srv_address_ << ":" << srv_port_ << "): " << p_conn_mgr_->num_connections(srv_address_, srv_port_, detail::wait_connection_status) << std::endl;
				}
				//std::cout << ":: Num Waiting Connections for (" << srv_address_ << ":" << srv_port_ << "): " << p_conn_mgr_->num_connections(srv_address_, srv_port_, detail::wait_connection_status) << std::endl;
				//std::cout << ":: Num Waiting Connections for (" << srv_address_ << ":" << srv_port_ << "): " << p_conn_mgr_->num_connections(srv_address_, srv_port_) << std::endl;
			}

			one_more_time = !one_more_time;
		}
		while (!*p_sniffer_done_ && !one_more_time);
	}


	private: ::std::string srv_address_;
	private: ::boost::uint16_t srv_port_;
	private: network_connection_manager* p_conn_mgr_;
	private: ::boost::atomic<bool>* p_sniffer_done_;
	private: packet_queue_type* p_pkt_queue_;
}; // packet_analyzer_runner

}} // Namespace detail::<unnamed>


int main(int argc, char* argv[])
{
	std::string db_uri; // URI to connect to the DB
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
		db_uri = dcs::cli::simple::get_option<std::string>(argv, argv+argc, "--db", detail::default_db_uri);
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

	dcs::uri uri(db_uri);

	boost::shared_ptr<detail::base_data_store> p_data_store;
#if defined(DCS_TESTBED_NETSNIF_USE_SQLITE_DATA_STORE)
	std::string db_name(uri.path());
	//detail::network_connection_manager conn_mgr(boost::make_shared<detail::sqlite_data_store>(db_name));
	p_data_store = boost::make_shared<detail::sqlite_data_store>(db_name);
#elif defined(DCS_TESTBED_NETSNIF_USE_MYSQL_DATA_STORE)
	std::string db_host;
	std::string db_name(uri.path().substr(1));
	std::string db_user;
	std::string db_pass;
	{
		std::ostringstream mysql_oss;
		mysql_oss << uri.scheme() << "://" << uri.host() << ":" << uri.port();
		db_host = mysql_oss.str();

		std::string query(uri.query());
		if (!query.empty())
		{
			std::size_t pos1;
			std::size_t pos2;
			std::size_t off;
			std::string tok;
			tok = "user=";
			off = tok.length();
			pos1 = query.find(tok);
			pos2 = query.find("&", pos1);
			db_user = query.substr(pos1+off, pos2-pos1-off);
			tok = "password=";
			off = tok.length();
			pos1 = query.find(tok);
			pos2 = query.find("&", pos1);
			db_pass = query.substr(pos1+off, pos2-pos1-off);
		}
	}
	//detail::network_connection_manager conn_mgr(boost::make_shared<detail::mysql_data_store>(db_host, db_name, db_user, db_pass));
	p_data_store = boost::make_shared<detail::mysql_data_store>(db_host, db_name, db_user, db_pass);
#elif defined(DCS_TESTBED_NETSNIF_USE_RAM_DATA_STORE)
	//detail::network_connection_manager conn_mgr(boost::make_shared<detail::ram_data_store>());
	p_data_store = boost::make_shared<detail::ram_data_store>();
#endif // DCS_TESTBED_NETSNIF_USE_*_DATA_STORE

#if defined(DCS_TESTBED_NETSNIF_USE_BOOST_THREAD_SYNC_PACKET_QUEUE)
	typedef boost::sync_queue< boost::shared_ptr<dcs::network::pcap::raw_packet> > packet_queue_type;
#elif defined(DCS_TESTBED_NETSNIF_USE_BOOST_LOCKFREE_MWMR_PACKET_QUEUE)
	typedef boost::lockfree::queue< boost::shared_ptr<dcs::network::pcap::raw_packet>, boost::lockfree:capacity<1024> > packet_queue_type;
#elif defined(DCS_TESTBED_NETSNIF_USE_BOOST_LOCKFREE_SPSC_PACKET_QUEUE)
	typedef boost::lockfree::spsc_queue< boost::shared_ptr<dcs::network::pcap::raw_packet> > packet_queue_type;
#elif defined(DCS_TESTBED_NETSNIF_USE_DCS_CONCURRENT_BLOCKING_PACKET_QUEUE)
	typedef dcs::concurrent::blocking_queue< boost::shared_ptr<dcs::network::pcap::raw_packet> > packet_queue_type;
#endif // DCS_TESTBED_NETSNIF_USE_*_PACKET_QUEUE

	try
	{
		p_data_store->open();
		p_data_store->clear();
		detail::network_connection_manager conn_mgr(p_data_store);

		boost::thread_group thd_group;

#if defined(DCS_TESTBED_NETSNIF_USE_BOOST_THREAD_SYNC_PACKET_QUEUE)
		packet_queue_type pkt_queue;
#elif defined(DCS_TESTBED_NETSNIF_USE_BOOST_LOCKFREE_MWMR_PACKET_QUEUE)
		packet_queue_type pkt_queue;
#elif defined(DCS_TESTBED_NETSNIF_USE_BOOST_LOCKFREE_SPSC_PACKET_QUEUE)
		packet_queue_type pkt_queue(1024);
#elif defined(DCS_TESTBED_NETSNIF_USE_DCS_CONCURRENT_BLOCKING_PACKET_QUEUE)
		packet_queue_type pkt_queue;
#endif // DCS_TESTBED_NETSNIF_USE_*_PACKET_QUEUE
		boost::atomic<bool> sniffer_done(false);

		thd_group.create_thread(
				detail::packet_sniffer_runner<packet_queue_type>(dev,
											  srv_address,
											  srv_port,
											  &sniffer_done,
											  &pkt_queue));

		thd_group.create_thread(
				detail::packet_analyzer_runner<packet_queue_type>(srv_address,
											   srv_port,
											   &conn_mgr,
											   &sniffer_done,
											   &pkt_queue));

		thd_group.join_all();

#if defined(DCS_TESTBED_NETSNIF_USE_BOOST_THREAD_SYNC_PACKET_QUEUE)
		pkt_queue.close();
#endif // DCS_TESTBED_NETSNIF_USE_BOOST_THREAD_SYNC_PACKET_QUEUE
	}
	catch (std::runtime_error const& e)
	{
		p_data_store->close();

		std::ostringstream oss;
		oss << "Something went wrong: " << e.what();
		dcs::log_error(DCS_LOGGING_AT, oss.str());
	}

/*
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
								conn_mgr.end_connection_establishment(srv_address, srv_port, cli_address, cli_port);
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
									conn_mgr.end_connection_termination(srv_address, srv_port, cli_address, cli_port);
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
								conn_mgr.begin_connection_establishment(srv_address, srv_port, cli_address, cli_port);
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
								conn_mgr.begin_connection_termination(srv_address, srv_port, cli_address, cli_port);
							}
							catch (std::exception const& e)
							{
								std::ostringstream oss;
								oss << "Stats update for begin of connection termination: " << e.what();
								dcs::log_error(DCS_LOGGING_AT, oss.str());
							}
						}
//						std::cout << ":: Num Waiting Connections for (" << srv_address << ":" << srv_port << "): " << conn_mgr.num_connections(srv_address, srv_port, detail::wait_connection_status) << std::endl;
					}
					std::cout << ":: Num Waiting Connections for (" << srv_address << ":" << srv_port << "): " << conn_mgr.num_connections(srv_address, srv_port, detail::wait_connection_status) << std::endl;
				}
				std::cout << "--------------------------------------------" << std::endl;
			}
		}

		//::sleep(zzz_time);
	}
	//while (trial < num_trials);
	while (true);
*/
}
