#!/usr/bin/env python

import argparse
import xmlrpclib


arg_parser = argparse.ArgumentParser()
arg_parser.add_argument("-s", "--server", dest="server", help="Hostname or IP address of the XML-RPC server")
arg_parser.add_argument("-p", "--port", dest="port", action="store", type=int, help="Port of the XML-RPC server")
arg_parser.add_argument("-a", "--mserver", dest="mon_server", help="Hostname or IP address of the monitored server")
arg_parser.add_argument("-n", "--mport", dest="mon_port", action="store", type=int, help="Port of the monitored server")
args = arg_parser.parse_args();
#if len(vars(args)) == 0:
#	arg_parser.error("Invalid number of arguments")
if not args.server:
	arg_parser.error("Server name has not been specified")
if args.port < 0 or args.port > 65535:
	arg_parser.error("Port number is out of range")

# Connec to the server
url = "http://%s:%d" % (args.server, args.port)
rpc = xmlrpclib.ServerProxy(url)

# Query for the number of TCP connections
n = rpc.num_pending_connections(args.mon_server, args.mon_port)
print("Number of pending connections: %d" % n)
(na,nd,ts) = rpc.connection_stats(args.mon_server, args.mon_port)
print("num-arrivals: %d - num-departures: %d - ts: %d" % (na, nd, ts))
#n = rpc.num_connections_by_status(args.mon_server, args.mon_port, 0)
#print("Number of connections: %d" % n)
