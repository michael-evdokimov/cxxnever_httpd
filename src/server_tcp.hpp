#pragma once
#include <cstdint>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "logging.hpp"


namespace cxxnever
{

struct ServerTcp
{
	int fd = -1;

	~ServerTcp()
	{
		if (fd != -1)
			close(fd);
	}

	int open(const char* addr, uint16_t port)
	{
		int family = 0;

		struct in6_addr addr6 = {};
		struct in_addr addr4 = {};
		if (inet_pton(AF_INET6, addr, &addr6) == 1)
			family = AF_INET6;
		if (inet_pton(AF_INET, addr, &addr4) == 1)
			family = AF_INET;

		if (family == 0)
			return log(ERROR, "Cannot read IP addr: %m"), -1;


		fd = socket(family, SOCK_STREAM, 0);
		if (fd == -1)
			return log(ERROR, "Cannot create socket: %m"), -1;

		int yes = 1;
		int opts = SO_REUSEADDR | SO_REUSEPORT;
		if (setsockopt(fd, SOL_SOCKET, opts, &yes, sizeof(yes)) == -1)
			return log(ERROR, "Cannot set reuse: %m"), -1;

		struct sockaddr_in addr_4 = {};
		addr_4.sin_family = family;
		addr_4.sin_addr = addr4;
		addr_4.sin_port = htons(port);

		struct sockaddr_in6 addr_6 = {};
		addr_6.sin6_family = family;
		addr_6.sin6_addr = addr6;
		addr_6.sin6_port = htons(port);

		struct sockaddr* paddr4 = reinterpret_cast<sockaddr*>(&addr_4);
		struct sockaddr* paddr6 = reinterpret_cast<sockaddr*>(&addr_6);
		struct sockaddr* paddr_ = (family == AF_INET) ? paddr4 : paddr6;
		size_t sl = (family==AF_INET) ? sizeof(addr_4) : sizeof(addr_6);
		if (bind(fd, paddr_, sl) == -1) {
			log(ERROR, "Cannot bind %s:%hu: %m", addr, port);
			return -1;
		}

		if (listen(fd, 1) == -1)
			return log(ERROR, "Cannot listen(): %m"), -1;

		return 0;
	}

	int accept()
	{
		int r = ::accept(fd, nullptr, nullptr);
		if (r == -1)
			return log(ERROR, "Cannot accept(): %m"), -1;

		return r;
	}
};

}
