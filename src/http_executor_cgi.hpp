#pragma once
#include "http_executor.hpp"
#include "fast_cgi_con.hpp"
#include "narrow.hpp"


namespace cxxnever
{

struct HttpExecutorCGI : HttpExecutor
{
	typedef std::pair<std::string_view, std::string_view> OneParam;

	virtual int request(std::shared_ptr<FastCGIClient::Request> req,
	                    const std::vector<OneParam>& params,
	                    std::string_view input) = 0;

	void process(HttpStream& strm, HttpParser& parser,
	                                                 int notify_fd) override
	{
		typedef std::string_view sv;
		std::vector<std::pair<sv, sv>> params;
		params.reserve(8);

		size_t len = parser.input.size();
		char len_buf[21] = {};
		snprintf(len_buf, sizeof(len_buf), "%zd", len);

		params.emplace_back("SCRIPT_FILENAME", strm.full_path);
		params.emplace_back("REQUEST_METHOD", parser.method);
		params.emplace_back("QUERY_STRING", parser.query_string);
		params.emplace_back("REQUEST_URI", parser.url);
		params.emplace_back("PATH_INFO", parser.path_info);
		params.emplace_back("CONTENT_TYPE",parser.find("Content-Type"));
		params.emplace_back("CONTENT_LENGTH", len_buf);
		params.emplace_back("REFERER", parser.find("Referer"));

		strm.cgi_req = std::make_shared<FastCGIClient::Request>();
		strm.cgi_req->notify_fd = dup(notify_fd);

		if (request(strm.cgi_req, params, parser.input) == -1) {
			strm.cgi_req = {};
			strm.code = 502;
		}
	}

	static int add_cgi_content(HttpStream& strm)
	{
		if (!strm.cgi_req->done)
			return -EAGAIN;

		using namespace narrow;

		std::string_view output = strm.cgi_req->output;

		if (output.starts_with("Status:")) {
			std::string str {view(output, right, "\r\n",
			                              left, "\x20",
			                              right, "\x20")};
			strm.code = atoi(str.c_str());
			output = view(output, left, "\r\n");
		}

		auto headers = view(output, right, "\r\n\r\n", right_n, 2);
		auto body = view(output, left, "\r\n\r\n");

		strm.headers += headers;
		strm.body += body;

		if (strm.headers.find("Content-Length") == -1)
			strm.header("Content-Length", strm.body.size());

		strm.cgi_req = {};
		return 0;
	}
};

}
