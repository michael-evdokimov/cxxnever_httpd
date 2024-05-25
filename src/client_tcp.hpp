#pragma once
#include <sys/socket.h>
#include <sys/un.h>


namespace cxxnever
{

struct ClientTcp
{
	int fd = -1;

	~ClientTcp()
	{
		if (fd != -1)
			close(fd);
	}

	int open(const char* unix_socket)
	{
		fd = socket(AF_UNIX, SOCK_STREAM, 0);
		if (fd == -1)
			return log(ERROR, "Cannot creat socket"), -1;

		struct sockaddr_un addr {AF_UNIX};
		snprintf(addr.sun_path, sizeof(addr.sun_path),"%s",unix_socket);

		auto p_addr = reinterpret_cast<struct sockaddr*>(&addr);

		if (connect(fd, p_addr, sizeof(addr)) == -1) {
			log(ERROR, "Cannot connect: %m");
			log(ERROR, "Cannot connect to %s", unix_socket);
			return -1;
		}

		return 0;
	}
};

}
