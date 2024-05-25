#pragma once
#include <unistd.h>


namespace cxxnever
{

struct ConnectionTcp
{
	int fd = -1;

	virtual ~ConnectionTcp()
	{
		::close(fd);
	}

	virtual int read() = 0;

	virtual std::string_view input() = 0;

	virtual void commit_read(size_t n) = 0;

	virtual ssize_t write(const std::string_view&) = 0;
};

}
