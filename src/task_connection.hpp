#pragma once
#include "manager.hpp"
#include "http_handler.hpp"
#include "server_tcp_con.hpp"


namespace cxxnever
{

struct ConnectionTask : Task
{
	std::shared_ptr<ConnectionTcp> con;
	std::shared_ptr<HttpService> service;
	HttpStream strm;


	virtual int open()
	{
		int flags = fcntl(con->fd, F_GETFL, 0);
		flags |= O_NONBLOCK;
		fcntl(con->fd, F_SETFL, flags);

		return 0;
	}

	virtual int notify_fd() const { return -1; }

	int get_fd() const override { return con->fd; }

	int process() override
	{
		bool some_work_is_done = false;

		if (!strm.received || strm.sent) {
			int r = con->read();
			if (r == -EPIPE)
				return -EPIPE;
			if (r == -1)
				return log(ERROR, "Cannot read"), -1;
			if (r == 0)
				some_work_is_done = true;

			if (con->input().size()) {
				HttpHandler http;
				http.notify_fd = notify_fd();
				http.service = &*service;
				size_t done = http.process(con->input(), strm);
				if (done) {
					strm.received = true;
					con->commit_read(done);
				} else if (con->input().size() > 64 * 1024)
					return -EPIPE;
			}
		}

		HttpHandler http;
		if (http.add_cgi_content(strm) == -EAGAIN)
			return -EAGAIN;

		while (strm.received && !strm.sent) {
			HttpStreamReader reader {strm};
			std::string_view buf;
			if (reader.read_next(buf) == -1)
				return log(ERROR, "Cannot read more"), -EPIPE;
			ssize_t r = con->write(buf);
			if (r == -EPIPE)
				return -EPIPE;
			if (r == -EAGAIN)
				break;
			if (r == -1)
				return log(ERROR, "Cannot write"), -EPIPE;
			if (r)
				reader.commit(r), some_work_is_done;
		}

		if (strm.sent && strm.may_close)
			return -EPIPE;

		return some_work_is_done ? 0 : -EAGAIN;
	}
};

}
