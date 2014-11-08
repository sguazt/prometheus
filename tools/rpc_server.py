#!/usr/bin/env python

import argparse
import mysql.connector
from mysql.connector import errorcode
from SimpleXMLRPCServer import SimpleXMLRPCServer
import sys
import time
import urlparse


## Internal logic

max_num_trials = 30
zzz_time = 1

def register_uri_scheme(scheme):
    for method in filter(lambda s: s.startswith('uses_'), dir(urlparse)):
        getattr(urlparse, method).append(scheme)

class NetworkConnectionsManager:
	"""Managed stored network connections."""

	unknown_connection_status = -1
	wait_connection_status = 0
	active_connection_status = 1
	closed_connection_status = 2

	default_host = "localhost"
	default_port = 3306
	default_db_name = "netsnif"

	def __init__(self, db_uri):
		# Register "tcp" scheme to let 'urlparse' to correctly parse associated URIs
		register_uri_scheme('tcp')
		uri = urlparse.urlsplit(db_uri)
		params = urlparse.parse_qs(uri.query)
		self.db_host_ = uri.hostname or self.default_host
		self.db_port_ = uri.port or self.default_port
		self.db_name_ = uri.path.lstrip("/") or self.default_db_name
		if "user" in params:
			self.db_user_ = params["user"][0]
		else:
			self.db_user_ = ""
		if "password" in params:
			self.db_passwd_ = params["password"][0]
		else:
			self.db_passwd_ = ""
		#print("URI: %s\n" %  (db_uri))

	def num_connections_by_status(self, host, port, status):
		sql = ("SELECT COUNT(*)"
			   " FROM network_connection"
			   " WHERE server_addr=%s AND server_port=%d"
			   " GROUP BY status"
			   " HAVING status=%d")

		ret = 0
		try:
			db_conn = mysql.connector.connect(host=self.db_host_,
											  port=self.db_port_,
											  database=self.db_name_,
											  user=self.db_user_,
											  password=self.db_passwd_)
			db_cursor = db_conn.cursor()
			db_cursor.execute(sql, (host, port, status))
			for (count) in db_cursor:
				ret = count
			db_cursor.close()
		except mysql.connector.Error as err:
			if err.errno == errorcode.ER_ACCESS_DENIED_ERROR:
				print("Something is wrong your username or password: {}".format(err))
			elif err.errno == errorcode.ER_BAD_DB_ERROR:
				print("Database does not exists: {}".format(err))
			else:
				print("Something went wrong during DB operation: {}".format(err))
		else:
			db_conn.close()
		return ret


## XML-RPC functions

class RpcFuncs:
	def __init__(self, db_uri):
		self.db_uri_ = db_uri

	def num_tcp_connections(self, host="localhost", port=9090, status=NetworkConnectionsManager.wait_connection_status):
		"""Return the number of pending and served TCP connection at the given
		   host and port."""
		mgr = NetworkConnectionsManager(self.db_uri_)
		return mgr.num_connections_by_status(host, port, status)


## Entry point

if __name__ == "__main__":
	arg_parser = argparse.ArgumentParser()
	# Positional args
	#arg_parser.add_argument("args")
	# Option args
	arg_parser.add_argument("-p", "--port", dest="port", action="store", type=int, default=9090, help="Port to listen for connections")
	arg_parser.add_argument("-b", "--db", dest="db", help="DB URI")
	args = arg_parser.parse_args();
	#if len(vars(args)) == 0:
	#	arg_parser.error("Invalid number of arguments")
	if args.port < 0 or args.port > 65535:
		arg_parser.error("Port number is out of range")
	if not args.db:
		arg_parser.error("DB has not been specified")

	# Setup the server
	# NOTE: Hostname "" is a symbolic name meaning all available interfaces
	server = SimpleXMLRPCServer(("", args.port));

	# Register functions
	server.register_introspection_functions()
	server.register_instance(RpcFuncs(args.db))
	#server.register_function(num_tcp_connections)

	# Run the server's main loop
	server.serve_forever()
