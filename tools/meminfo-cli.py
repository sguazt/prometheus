import argparse
import json
import socket
import struct
import sys


def recvall(sock, n):
	# Helper function to recv n bytes or return None if EOF is hit
	data = ''
	while len(data) < n:
		packet = sock.recv(n - len(data))
		if not packet:
			return None
		data += packet
	return data

def recv_msg(sock):
	# Read message length and unpack it into an integer
	raw_msglen = recvall(sock, 4)
	if not raw_msglen:
		return None
	msglen = struct.unpack('>I', raw_msglen)[0]
	# Read the message data
	return recvall(sock, msglen)



if __name__ == '__main__':
	arg_parser = argparse.ArgumentParser()
	arg_parser.add_argument('-a', '--srvaddr', dest='srvaddr', action='store', default='127.0.0.1', help='Server address (or name)')
	arg_parser.add_argument('-p', '--srvport', dest='srvport', action='store', type=int, default=9090, help='Server port')
	args = arg_parser.parse_args()
	if not args.srvaddr:
		arg_parser.error("Invalid server address")
	if args.srvport < 0 or args.srvport > 65535:
		arg_parser.error("Port number is out of range")

	# Create a TCP/IP socket
	sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

	# Connect the socket to the address given on the command line
	server_name = args.srvaddr
	server_port = args.srvport
	server_address = (server_name, server_port)
	print >>sys.stderr, 'Connecting to %s port %s' % server_address
	sock.connect(server_address)
	msg = recv_msg(sock)
	sock.close()
	if msg:
		meminfo = json.loads(msg)
		print >>sys.stderr, 'RAW MEMINFO: ', meminfo
		print 'MEMINFO => Tot: ', meminfo['MemTotal'], ', Avail: ', meminfo['MemAvailable']
	else:
		print 'Nothing received'
