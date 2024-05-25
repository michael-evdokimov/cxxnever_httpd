#pragma once
#include "server_tcp.hpp"
#include "server_tcp_con_std.hpp"
#include "server_tcp_con_ssl.hpp"
#include "task_connection_execable.hpp"
#include "manager.hpp"


namespace cxxnever
{

struct ServerTask : Task
{
	Manager& manager;
	ServerTcp server;
	bool use_ssl = false;
	std::shared_ptr<ContextSsl> ssl_ctx;
	std::shared_ptr<HttpService> http_service;

	ServerTask(Manager& manager)
		: manager(manager)
	{
	}

	int get_fd() const override { return server.fd; }

	int open(const char* addr, uint16_t port)
	{
		return server.open(addr, port);
	}

	int open_ssl(const char* file_cert_pem, const char* file_key_pem)
	{
		ssl_ctx.reset(new ContextSsl);
		if (int r = ssl_ctx->open(file_cert_pem, file_key_pem))
			return log(ERROR, "Cannot open ssl"), -1;

		use_ssl = true;

		return 0;
	}

	int process() override
	{
		int fd = server.accept();
		if (fd == -1)
			return -1;

		std::shared_ptr<ConnectionTask> con;

		if (http_service->at_least_one_cgi_executor)
			con.reset(new ConnectionTaskExecable);
		else
			con.reset(new ConnectionTask);

		con->service = http_service;

		if (use_ssl)
			con->con.reset(new ConnectionTcpSsl(ssl_ctx));
		else
			con->con.reset(new ConnectionTcpStd);

		con->con->fd = fd;

		if (con->open())
			return log(ERROR, "Cannot con_task->open()"), 0;

		if (manager.add(con))
			return log(ERROR, "Cannot add to manager"), 0;

		return -EAGAIN;
	}
};

}
