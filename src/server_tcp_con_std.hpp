#pragma once
#include "logging.hpp"
#include "server_tcp_con.hpp"


namespace cxxnever
{

struct ConnectionTcpStd : ConnectionTcp
{
	std::string inbuf;
	size_t parsed_len = 0;
	size_t inbuf_len = 0;

	int read() override
	{
		ssize_t r = 1;
		while (r > 0) {
			if (inbuf_len == inbuf.size())
				inbuf.resize(inbuf.size() * 2 + 16 * 1024);
			size_t space = inbuf.size() - inbuf_len;
			r = ::read(fd, &inbuf[inbuf_len], space);
			if (r > 0)
				inbuf_len += r;
			if (r < space)
				break;
		}

		if (r == -1) {
			if (errno == EAGAIN)
				return -EAGAIN;
			if (errno == ECONNRESET)
				return -EPIPE;
			return log(ERROR, "Cannot read: %m"), -1;
		}
		if (r == 0)
			return -EPIPE;

		return 0;
	}

	void commit_read(size_t n) override
	{
		parsed_len += n;
		if (parsed_len == inbuf_len)
			parsed_len = 0, inbuf_len = 0;
	}

	std::string_view input() override
	{
		size_t n = inbuf_len - parsed_len;
		return std::string_view {&inbuf[parsed_len], n};
	}

	ssize_t write(const std::string_view& data) override
	{
		ssize_t r = ::write(fd, &data[0], data.size());
		if (r == -1) {
			if (errno == EAGAIN)
				return -EAGAIN;
			if (errno == EPIPE)
				return -EPIPE;
			return log(ERROR, "Cannot write: %m"), -1;
		}
		return r;
	}
};

}
