#pragma once
#include "http_executor_cgi.hpp"
#include "manager.hpp"


namespace cxxnever
{

struct ExecutorTask : Task, HttpExecutorCGI
{
	std::string remote_addr;
	std::string file_ext;

	std::shared_ptr<FastCGICon> con;

	int epoll_fd = -1;


	~ExecutorTask()
	{
		if (epoll_fd != -1)
			::close(epoll_fd);
	}

	int open()
	{
		epoll_fd = epoll_create(1);
		if (epoll_fd == -1)
			return log(ERROR, "Cannot create epoll: %m"), -1;

		return 0;
	}

	const std::string& get_file_ext() const override
	{
		return file_ext;
	}

	bool want(std::string_view path) const override
	{
		return path.ends_with(file_ext);
	}

	int get_fd() const override { return epoll_fd; }

	int reconnect()
	{
		std::shared_ptr<FastCGICon> new_con {new FastCGICon};
		if (new_con->open(remote_addr.c_str()) == -1) {
			log(ERROR, "Cannot connect to %s", remote_addr.c_str());
			log(ERROR, "Cannot connect FastCGI");
			return -1;
		}

		struct epoll_event ev = {};
		ev.data.fd = new_con->client.client.fd;
		ev.events = EPOLLIN;
		int r = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, ev.data.fd, &ev);
		if (r == -1)
			return log(ERROR, "Cannot add to fcgi epoll: %m"), -1;

		con = std::move(new_con);
		return 0;
	}

	typedef std::pair<std::string_view, std::string_view> OneParam;

	int request(std::shared_ptr<FastCGIClient::Request> req,
	            const std::vector<OneParam>& params,
		    std::string_view input) override
	{
		auto con_copy = con;
		if (!con_copy)
			reconnect();

		con_copy = con;
		if (!con_copy)
			return -1;

		int r = con_copy->send_request(req, params, input);
		if (r == -1) {
			log(ERROR, "Cannot fastcgi send request");
			con = {};
			return -1;
		}

		return 0;
	}

	int process() override
	{
		auto con_copy = con;

		if (!con_copy)
			return -EAGAIN;

		int r = con_copy->read();
		if (r == -1)
			con = {};

		return (r == -EAGAIN) ? -EAGAIN : 0;
	}
};

}
