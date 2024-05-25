#pragma once
#include <vector>
#include "impl_page.hpp"


namespace cxxnever
{

struct Pager
{
	typedef void func_t(Page&);
	typedef std::vector<std::pair<std::string, func_t*>> list_t;

	func_t* saved = nullptr;

	static list_t& list()
	{
		static list_t r;
		return r;
	}

	Pager(func_t f, const char* path)
	{
		list().emplace_back(path, f);
		saved = f;
	}

	~Pager()
	{
		for (auto it = list().begin(); it != list().end(); ++it) {
			if (it->second == saved) {
				list().erase(it);
				break;
			}
		}
	}
};

}
