#pragma once
#include <vector>
#include <string>
#include <sstream>


namespace cxxnever
{

struct HttpdConfig
{
	struct FastCGI
	{
		std::string file_ext;
		std::string addr;
	};

	struct Host
	{
		std::string name;
		std::string root;
		std::vector<FastCGI> fast_cgi_list;
		std::vector<std::string> indexes;
		std::string cxxpage_file_ext;
	};

	struct Addr
	{
		std::string addr;
		uint16_t port = 0;
		bool use_ssl = false;
	};

	struct Server
	{
		std::vector<Addr> addrs;
		std::string cert;
		std::string key;
		std::vector<Host> hosts;
	};

	const unsigned cpu_count = std::thread::hardware_concurrency();

	std::vector<Server> servers;
	unsigned worker_processes = cpu_count;
	std::vector<std::string> stack = {"global"};


	int read(const std::string& filename)
	{
		std::ifstream file {filename};
		if (!file) {
			log(ERROR, "Cannot open file %s: %m", filename.c_str());
			return -1;
		}

		int line_number = 0;
		std::vector<std::string> commands;
		std::string line;
		while (getline(file, line)) {
			line_number++;
			if (size_t i = line.find('#') + 1)
				line.resize(i - 1);
			for (size_t i = line.size(); i--; )
				if (line[i] == ';')
					line.insert(i, "\x20");
			std::stringstream ss {line};
			std::string word;
			while (ss >> word) {
				commands.push_back(word);
				if (word == ";" || word == "{" || word == "}") {
					ssize_t r = process(commands);
					if (r == -1) {
						log(ERROR, "Error in line %d",
						           line_number);
						return r;
					}
					commands = {};
				}
			}
		}
		if (stack.back() != "global") {
			log(ERROR, "Unclosed %s", stack.back().c_str());
			return -1;
		}
		return 0;
	}

	int process(std::vector<std::string>& list)
	{
		std::string& cmd = list[0];
		if (stack.back() == "global") {
			if (cmd == "worker_processes") {
				if (list[1] == "auto")
					worker_processes = cpu_count;
				else
					worker_processes = atoi(&list[1][0]);
				return 0;
			}
			if (cmd == "server") {
				servers.push_back({});
				stack.push_back(cmd);
				return 0;
			}
		}
		if (stack.back() == "server") {
			if (cmd == "host") {
				if (list.size() < 3)
					return -1;
				Host host = {};
				host.name = list[1];
				servers.back().hosts.push_back(host);
				stack.push_back(cmd);
				return 0;
			}
			if (cmd == "}") {
				stack.pop_back();
				return 0;
			}
			if (cmd == "listen") {
				if (list.size() != 3 && list.size() != 4)
					return -1;
				Addr addr = {};
				std::string arg = list[1];
				if (char* p = strrchr(&arg[0], ':'))
					if (strchr(&arg[0], ']') < p)
						addr.port = atoi(p + 1), *p = 0;
				arg = &arg[0];
				if (arg[0] == '[' && arg.back() == ']') {
					arg.erase(arg.begin());
					arg.pop_back();
				}
				addr.addr = arg;
				if (list.size() == 4 && list[2] == "ssl")
					addr.use_ssl = true;
				servers.back().addrs.push_back(addr);
				return 0;
			}
			if (cmd == "ssl_cert") {
				if (list.size() != 3)
					return -1;
				servers.back().cert = list[1];
				return 0;
			}
			if (cmd == "ssl_key") {
				if (list.size() != 3)
					return -1;
				servers.back().key = list[1];
				return 0;
			}
		}
		if (stack.back() == "host") {
			if (cmd == "}") {
				stack.pop_back();
				return 0;
			}
			if (cmd == "root") {
				if (list.size() != 3)
					return -1;
				servers.back().hosts.back().root = list[1];
				return 0;
			}
			if (cmd == "fastcgi") {
				if (list.size() != 4)
					return -1;
				auto& host = servers.back().hosts.back();
				FastCGI fast_cgi;
				fast_cgi.file_ext = list[1];
				fast_cgi.addr = list[2];
				host.fast_cgi_list.push_back(fast_cgi);
				return 0;
			}
			if (cmd == "index") {
				auto& host = servers.back().hosts.back();
				for (size_t i = 1; i < list.size() - 1; ++i)
					host.indexes.push_back(list[i]);
				return 0;
			}
			if (cmd == "cxxpage") {
				if (list.size() != 3)
					return -1;

				auto& host = servers.back().hosts.back();
				host.cxxpage_file_ext = list[1];
				return 0;
			}
		}
		log(ERROR, "Unknown command %s", cmd.c_str());
		return -1;
	}
};

}
