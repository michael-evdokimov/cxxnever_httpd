#pragma once
#include <list>
#include <thread>
#include "manager.hpp"


namespace cxxnever
{

struct ManagerThreaded : Manager
{
	std::list<std::thread> thread_list;
	int time_interval_ms = 100;
	bool stop_flag = false;

	~ManagerThreaded()
	{
		stop_flag = true;
		for (std::thread& th: thread_list)
			th.join();
	}

	void add_thread()
	{
		thread_list.emplace_back(&ManagerThreaded::run, this);
	}

	void run()
	{
		while (stop_flag == false) {
			Manager::process(time_interval_ms);
		}
	}
};

}
