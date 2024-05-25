#pragma once
#include "http_parser.hpp"
#include "http_directory.hpp"
#include "http_service.hpp"
#include "http_executor_cgi.hpp"


namespace cxxnever
{

struct HttpHandler
{
	HttpService* service = nullptr;
	int notify_fd = -1;

	size_t process(std::string_view in, HttpStream& strm)
	{
		HttpParser parser;

		size_t parsed = parser.parse(in);
		if (!parsed)
			return 0;

		strm = {};

		if (parser.return_code) {
			strm.code = parser.return_code;
			strm.may_close = true;
			return parsed;
		}

		std::string_view host = parser.find("Host");
		using namespace narrow;
		host = view(host, right_or_not, ":");
		HttpVirtualHost* vhost = service->find(host);

		HttpDirectory dir {vhost, parser};
		dir.show(strm, parser.url_path);

		if (auto* exe = dir.executor)
			exe->process(strm, parser, notify_fd);

		if (strm.headers.size() == 0 && !strm.cgi_req)
			strm.header("Content-Length", strm.body.size());

		return parsed;
	}

	int add_cgi_content(HttpStream& strm)
	{
		if (strm.cgi_req)
			return HttpExecutorCGI::add_cgi_content(strm);

		return 0;
	}
};

}
