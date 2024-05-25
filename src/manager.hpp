#pragma once
#include <unordered_map>
#include <memory>
#include <mutex>
#include <poll.h>
#include <sys/epoll.h>
#include "logging.hpp"


namespace cxxnever
{

struct Task
{
	std::mutex task_mutex;

	virtual ~Task()
	{
	}

	virtual int get_fd() const = 0;

	virtual int process() = 0;
};

struct Manager
{
	int fd = -1;
	std::unordered_map<int, std::shared_ptr<Task>> list;
	std::mutex list_mutex;

	~Manager()
	{
		close(fd);
	}

	int open()
	{
		fd = epoll_create(1);
		if (fd == -1)
			return log(ERROR, "Cannot create epoll: %m"), -1;

		return 0;
	}

	int add(std::shared_ptr<Task> task)
	{
		std::unique_lock lock {list_mutex};
		list.emplace(task->get_fd(), task);
		lock.unlock();

		struct epoll_event ev = {};
		ev.events = EPOLLIN | EPOLLOUT | EPOLLET;
		ev.data.fd = task->get_fd();
		int r = epoll_ctl(fd, EPOLL_CTL_ADD, task->get_fd(), &ev);
		if (r == -1)
			return log(ERROR, "Cannot epoll_add: %m"), -1;

		return 0;
	}

	void del(Task& task)
	{
		epoll_ctl(fd, EPOLL_CTL_DEL, task.get_fd(), nullptr);
		std::unique_lock lock {list_mutex};
		list.erase(task.get_fd());
		lock.unlock();
	}

	int process(int timeout_ms)
	{
		struct epoll_event ev = {};
		int r = epoll_wait(fd, &ev, 1, timeout_ms);
		if (r == -1)
			return log(ERROR, "Cannot epoll_wait: %m"), -1;
		if (r == 0)
			return 0;

		std::shared_ptr<Task> task;
		std::unique_lock lock {list_mutex};
		auto it = list.find(ev.data.fd);
		if (it != list.end())
			task = it->second;
		lock.unlock();

		if (!task)
			return 0;

		while (true) {
			std::unique_lock task_lock
			                    {task->task_mutex, std::defer_lock};
			if (!task_lock.try_lock())
				return 0;
			int r = task->process();
			task_lock.unlock();
			if (r == 0)
				continue;
			if (r == -EPIPE)
				return del(*task), 0;
			if (r != 0 && r != -EAGAIN) {
				log(ERROR, "Cannot process task");
				del(*task);
				return 0;
			}

			struct pollfd p = {};
			p.fd = task->get_fd();
			p.events = POLLIN;
			r = poll(&p, 1, 0);
			if (r == -1)
				return log(ERROR, "Cannot poll: %m"), -1;
			if (r == 0)
				break;
		}

		return 0;
	}
};

}
