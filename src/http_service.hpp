#pragma once
#include <unordered_map>
#include "http_executor.hpp"
#include "http_pager.hpp"


namespace cxxnever
{

struct HttpVirtualHost
{
	std::string hostname;
	std::string root;
	std::vector<std::string> index_files;
	std::vector<std::shared_ptr<HttpExecutor>> executors;
};

struct HttpService
{
	std::unordered_map<std::string_view,
	                               std::shared_ptr<HttpVirtualHost>> vhosts;

	HttpVirtualHost* default_vhost = nullptr;

	bool at_least_one_cgi_executor = false;


	HttpVirtualHost* find(std::string_view hostname)
	{
		auto it = vhosts.find(hostname);
		if (it != vhosts.end())
			return &*it->second;

		return default_vhost;
	}
};

}
