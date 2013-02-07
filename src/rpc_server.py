#!/usr/bin/env python

import apsw
import argparse
from SimpleXMLRPCServer import SimpleXMLRPCServer
import sys
import time


## Internal logic

class NetworkConnectionsManager:
	"""Managed stored network connections."""

	unknown_connection_status = -1
	wait_connection_status = 0
	active_connection_status = 1
	closed_connection_status = 2

	def __init__(self, db_file):
		self.db_file_ = db_file

	def num_connections_by_status(self, host, port, status):
		sql = "SELECT COUNT(*) FROM network_connection WHERE server_addr=? AND server_port=? GROUP BY status HAVING status=?"
		db_conn = apsw.Connection(self.db_file_, apsw.SQLITE_OPEN_READONLY)
		db_cursor = db_conn.cursor()
		ret = 0
		num_trials = 5
		trial = 1
		loop = True
		while loop:
			try:
				for row in db_cursor.execute(sql, (host, port, status)):
					ret = row[0]
					loop = False
			except:
				if trial < num_trials:
					++trial
					print >> sys.stderr, "Unable to query the DB: Zzz..."
					time.sleep(1)
				else:
					loop = False
					print >> sys.stderr, "Unable to query the DB. Give-up!"
					raise
			else:
				loop = False
		db_cursor.close()
		db_conn.close()
		return ret


## XML-RPC functions

class RpcFuncs:
	def __init__(self, db_file):
#		import string
		self.db_file_ = db_file
#		self.string = string

#	def _listMethods(self):
#		return list_public_methods(self) + \
#				['string.' + method for method in list_public_methods(self.string)]

	def num_tcp_connections(self, host="localhost", port=8080, status=NetworkConnectionsManager.wait_connection_status):
		"""Return the number of pending and served TCP connection at the given
		   host and port."""
		mgr = NetworkConnectionsManager(self.db_file_)
		return mgr.num_connections_by_status(host, port, status)

#def num_tcp_connections(host="localhost", port=8080, status=NetworkConnectionsManager.wait_connection_status):
#	"""Return the number of pending and served TCP connection at the given
#	   host and port."""
#	mgr = NetworkConnectionsManager(self.db_file_)
#	return mgr.num_connections_by_status(host, port, status)


## Entry point

if __name__ == "__main__":
	arg_parser = argparse.ArgumentParser()
	# Positional args
	#arg_parser.add_argument("args")
	# Option args
	arg_parser.add_argument("-p", "--port", dest="port", action="store", type=int, default=8080, help="Port to listen for connections")
	arg_parser.add_argument("-b", "--db", dest="db", help="Full path to the DB data")
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
