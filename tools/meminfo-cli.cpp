#include <arpa/inet.h>
#include <boost/array.hpp>
#include <boost/asio.hpp>
#include <boost/cstdint.hpp>
#include <boost/system/error_code.hpp>
#include <boost/system/system_error.hpp>
#include <boost/tuple/tuple.hpp>
#include <cstddef>
#include <jsoncpp/json/value.h>
#include <jsoncpp/json/reader.h>
#include <iostream>
#include <string>


inline
void usage(char const* progname)
{
	std::cerr << "Usage: " << progname << " <host> [<port>]" << std::endl;
}

boost::tuple<boost::uint32_t, std::string> unpack(char const* input)
{
	boost::tuple<boost::uint32_t, std::string> result;

	boost::uint32_t sz = ntohl(*reinterpret_cast<boost::uint32_t const*>(input));
	boost::get<0>(result) = sz;
	input += 1 * sizeof(boost::uint32_t);

	boost::get<1>(result) = std::string(reinterpret_cast<char const*>(input), sz);

	return result;
}

int main(int argc, char* argv[])
{
	if (argc < 2)
	{
		std::cerr << "Insufficient number of arguments." << std::endl;
		usage(argv[0]);
		return 1;
	}

	try
	{
		namespace asio = boost::asio;

		std::string server_addr = argv[1];
		std::string server_port = argv[2];

		std::cout << "Connecting to " << server_addr << " on port " << server_port << std::endl;

		asio::io_service io_service;

		asio::ip::tcp::resolver resolver(io_service);
		asio::ip::tcp::resolver::query query(server_addr, server_port);
		asio::ip::tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);

		asio::ip::tcp::socket socket(io_service);
		boost::asio::connect(socket, endpoint_iterator);

		std::ostringstream oss;
		for (;;)
		{
			boost::array<char, 1024> buf;
			boost::system::error_code error;

			const std::size_t len = socket.read_some(boost::asio::buffer(buf), error);
			if (error == boost::asio::error::eof)
			{
				break; // Connection closed cleanly by peer.
			}
			else if (error)
			{
				throw boost::system::system_error(error); // Some other error.
			}

			oss << std::string(buf.data(), len);
			//std::cout.write(buf.data(), len);
		}
		socket.close();

		if (oss.str().empty())
		{
			std::cout << "Nothing received" << std::endl;
		}
		else
		{
			boost::uint32_t sz;
			std::string meminfo;

			boost::tie(sz, meminfo) = unpack(oss.str().c_str());

			std::cerr << "MEMINFO: " << meminfo << std::endl;

			Json::Value root;   // will contains the root value after parsing
			Json::Reader reader;
			bool parse_ok = reader.parse(meminfo, root);
			if (!parse_ok)
			{
				std::cerr  << "Failed to parse configuration: " << reader.getFormattedErrorMessages() << std::endl;
				return 1;
			}

			long double mem_tot = 0;
			long double mem_avail = 0;
			std::istringstream iss;
			iss.str(root.get("MemTotal", "").asString());
			iss >> mem_tot;
			iss.str(root.get("MemAvailable", "").asString());
			iss >> mem_avail;
			std::cout << "MEMINFO => Tot: " << mem_tot << " kB, Avail: " << mem_avail << " kB" << std::endl;
		}
	}
	catch (std::exception const& e)
	{
		std::cerr << e.what() << std::endl;
		return 1;
	}
}
