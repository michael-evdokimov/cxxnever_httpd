#pragma once
#include "fast_cgi_client.hpp"


namespace cxxnever
{

struct FastCGICon
{
	FastCGIClient client;

	int open(const char* unix_socket)
	{
		return client.open(unix_socket);
	}

	typedef std::pair<std::string_view, std::string_view> OneParam;

	int send_request(std::shared_ptr<FastCGIClient::Request>& req,
	                 const std::vector<OneParam>& params,
	                 std::string_view input)
	{
		using namespace fastcgi;

		client.new_request(req);

		int r = 0;
		r = client.send_begin_request(req->req_id, FCGI_KEEP_CONN);
		if (r == -1)
			return log(ERROR, "Cannot send begin_req"), -1;

		if (params.size()) {
			r = client.send_params(req->req_id, params);
			if (r == -1)
				return log(ERROR, "Cannot send params"), -1;
		}

		const std::vector<OneParam> empty_params;
		r = client.send_params(req->req_id, empty_params);
		if (r == -1)
			return log(ERROR, "Cannot send empty params"), -1;

		for (size_t pos = 0; pos < input.size(); ) {
			size_t n = std::min(input.size() - pos, size_t(0xffff));
			std::string_view data {&input[pos], &input[pos] + n};
			int r = client.send_stdin(req->req_id, data);
			if (r == -1)
				return log(ERROR, "Cannot send stdin"), -1;
			pos += n;
		}

		r = client.send_stdin(req->req_id, "");
		if (r == -1)
			return log(ERROR, "Cannot send stdin"), -1;

		return 0;
	}

	int read()
	{
		bool work_done = false;
		while (true) {
			struct pollfd p = {};
			p.fd = client.client.fd;
			p.events = POLLIN;
			int r = poll(&p, 1, 0);
			if (r == -1)
				return log(ERROR, "Cannot poll fcgi: %m"), -1;
			if (r == 0)
				break;

			r = client.read();
			if (r == -1)
				return log(ERROR, "Cannot read fcgi"), -1;

			work_done = true;
		}

		return work_done ? 0 : -EAGAIN;
	}
};

}
