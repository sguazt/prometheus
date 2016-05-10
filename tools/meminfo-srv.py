import argparse
#from collections import OrderedDict
import json
import socket
import struct
import subprocess
import sys


def meminfo():
	''' Return the information in /proc/meminfo as a dictionary '''
	#meminfo = OrderedDict()
	meminfo= dict()

	with open('/proc/meminfo') as f:
		for line in f:
			meminfo[line.split(':')[0]] = line.split(':')[1].strip()

        vmstat = subprocess.check_output(["vmstat", "-a"])
        meminfo['vmstat'] = dict(zip(*map(str.split, vmstat.splitlines())[-2:]))

	return meminfo

def send_msg(conn, data):
	# Prefix each message with a 4-byte length (network byte order)
	msg = struct.pack('>I', len(data)) + data
	conn.sendall(msg)


if __name__ == '__main__':
	# Parse arguments
	arg_parser = argparse.ArgumentParser()
	arg_parser.add_argument('-a', '--addr', dest='addr', action='store', default='', help='Address to listen for connections')
	arg_parser.add_argument('-p', '--port', dest='port', action='store', type=int, default=9090, help='Port to listen for connections')
	#arg_parser.add_argument('-x', '--xinfo', dest='xinfo', action='store', type=bool, default=False, help='If True, sends the whole /proc/meminfo content')
	args = arg_parser.parse_args()
	if args.port < 0 or args.port > 65535:
		arg_parser.error("Port number is out of range")

	# Create a TCP/IP socket
	sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
	sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1) # Tells the kernel to reuse a local socket in TIME_WAIT state, without waiting for its natural timeout to expire.

	# Bind the socket to the address given on the command line
	server_name = args.addr
	server_port = args.port
	server_address = (server_name, server_port)
	sock.bind(server_address)
	print >>sys.stderr, 'starting up on %s port %s' % sock.getsockname()
	sock.listen(1)

	while True:
		print >>sys.stderr, 'waiting for a connection'
		connection, client_address = sock.accept()
		try:
			print >>sys.stderr, 'client connected:', client_address
			data = json.dumps(meminfo())
			if data:
				send_msg(connection, data)
			else:
				break
		finally:
			connection.close()
