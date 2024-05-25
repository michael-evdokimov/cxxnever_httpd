#pragma once
#include <fcntl.h>
#include <sys/eventfd.h>
#include "task_connection.hpp"


namespace cxxnever
{

struct ConnectionTaskExecable : ConnectionTask
{
	int epoll_fd = -1;
	int event_fd = -1;

	~ConnectionTaskExecable()
	{
		if (event_fd != -1)
			::close(event_fd);
		if (epoll_fd != -1)
			::close(epoll_fd);
	}

	int open()
	{
		if (ConnectionTask::open() == -1)
			return -1;

		epoll_fd = epoll_create(1);
		if (epoll_fd == -1)
			return log(ERROR, "Cannot create sub epoll: %m"), -1;

		event_fd = eventfd(0, 0);
		if (event_fd == -1)
			return log(ERROR, "Cannot create sub event: %m"), -1;

		struct epoll_event ev = {};
		ev.events = EPOLLIN | EPOLLET | EPOLLOUT;
		ev.data.fd = con->fd;
		if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, con->fd, &ev) == -1)
			return log(ERROR, "Cannot sub epoll add fd: %m"), -1;

		ev = {};
		ev.events = EPOLLIN | EPOLLET;
		ev.data.fd = event_fd;
		if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, ev.data.fd, &ev) == -1)
			return log(ERROR, "Cannot sub epoll add ev: %m"), -1;

		return 0;
	}

	int notify_fd() const override { return event_fd; }

	int get_fd() const override { return epoll_fd; }

	int process() override
	{
		struct epoll_event ev = {};
		epoll_wait(epoll_fd, &ev, 1, 0);

		return ConnectionTask::process();
	}
};

}
