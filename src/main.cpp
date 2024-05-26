#include <signal.h>
#include <systemd/sd-daemon.h>
#include "manager_threaded.hpp"
#include "task_server.hpp"
#include "task_executor.hpp"
#include "config.hpp"


using cxxnever::INFO;
using cxxnever::ERROR;
using cxxnever::log;

struct Application
{
	cxxnever::ManagerThreaded manager;
	std::string config_file = "/etc/cxxnever_httpd/httpd.conf";

	int read_args(int argc, const char** argv)
	{
		for (int i = 1; i < argc; ++i) {
			if (strcmp(argv[i], "-c") == 0)
				if (i + 1 < argc)
					config_file = argv[++i];
		}

		if (config_file.empty()) {
			printf("Error: missing config file argument\n");
			printf("Usage: %s -c filename.conf\n", argv[0]);
			return -1;
		}

		return 0;
	}

	auto create_service(cxxnever::HttpdConfig::Server& server_conf)
	{
		using cxxnever::HttpService;
		using cxxnever::HttpVirtualHost;
		using cxxnever::ExecutorTask;
		using cxxnever::HttpPager;

		auto service = std::make_shared<HttpService>();
		for (auto& vhost_conf: server_conf.hosts) {
			auto vh = std::make_shared<HttpVirtualHost>();
			vh->hostname = vhost_conf.name;
			vh->root = vhost_conf.root;
			vh->index_files = vhost_conf.indexes;
			service->vhosts[vh->hostname] = vh;
			if (!service->default_vhost)
				service->default_vhost = &*vh;
			for (auto& fastcgi: vhost_conf.fast_cgi_list) {
				auto ex = std::make_shared<ExecutorTask>();
				ex->remote_addr = fastcgi.addr;
				ex->file_ext = fastcgi.file_ext;
				if (ex->remote_addr.starts_with("unix:")) {
					auto* p=strchr(&ex->remote_addr[0],':');
					ex->remote_addr = ++p;
				}
				if (ex->open() == -1) {
					log(ERROR, "Cannot open executor");
					return decltype(service)();
				}
				if (manager.add(ex) == -1) {
					log(ERROR,
					      "Cannot add executor to manager");
				}
				vh->executors.push_back(ex);
				service->at_least_one_cgi_executor = true;
			}
			if (vhost_conf.cxxpage_file_ext.size()) {
				auto pager = std::make_shared<HttpPager>();
				pager->file_ext = vhost_conf.cxxpage_file_ext;
				vh->executors.push_back(pager);
			}
		}
		if (!service->default_vhost) {
			log(ERROR, "No virtual host");
			return decltype(service)();
		}
		return service;
	}

	auto create_server(const cxxnever::HttpdConfig::Server& server_conf,
	                   const cxxnever::HttpdConfig::Addr& addr)
	{
		using cxxnever::ServerTask;

		auto server = std::make_shared<ServerTask>(manager);
		int r = server->open(addr.addr.c_str(), addr.port);
		if (r) {
			log(ERROR, "Cannot open server %s:%hu",
			           addr.addr.c_str(), addr.port);
			return decltype(server)();
		}
		if (addr.use_ssl) {
			int r = server->open_ssl(server_conf.cert.c_str(),
			                         server_conf.key.c_str());
			if (r) {
				log(ERROR, "Cannot open ssl for server");
				return decltype(server)();
			}
		}
		return server;
	}

	int open()
	{
		cxxnever::HttpdConfig config;
		if (config.read(config_file) == -1)
			return log(ERROR, "Cannot read config"), -1;

		if (int r = manager.open())
			return log(ERROR, "Cannot open manager"), -1;

		for (auto& conf: config.servers) {
			auto service = create_service(conf);
			if (!service)
				return -1;
			for (auto& addr: conf.addrs) {
				auto server = create_server(conf, addr);
				if (!server)
					return -1;
				server->http_service = service;
				manager.add(server);
			}
		}

		manager.time_interval_ms = 100;

		for (unsigned n = config.worker_processes; n--; )
			manager.add_thread();

		return 0;
	}
};

bool stop = false;

void sigkill_catcher(int)
{
	stop = true;
}

int main(int argc, const char** argv)
{
	signal(SIGPIPE, SIG_IGN);
	signal(SIGQUIT, sigkill_catcher);
	signal(SIGINT, sigkill_catcher);

	log(INFO, "Starting");

	std::shared_ptr<Application> app {new Application};

	if (int r = app->read_args(argc, argv))
		return r;

	if (int r = app->open())
		return r;

	sd_notify(0, "READY=1");

	while (stop == false)
		usleep(100'000);

	sd_notify(0, "STOPPING=1");

	log(INFO, "Stopping");
	app = {};
	log(INFO, "Stopped");

	return 0;
}
