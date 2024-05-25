#pragma once
#include <regex>
#include <cstring>
#include "impl_page.hpp"
#include "impl_pager.hpp"
#include "impl_request.hpp"


static std::string_view convert(struct executing_request_value_t value)
{
	return std::string_view {value.value, value.value_len};
}

static struct executing_request_value_t convert(std::string_view value)
{
	struct executing_request_value_t r;
	r.value = &value[0];
	r.value_len = value.size();

	return r;
}

extern "C" {

int execute(struct executing_request_t* req)
{
	cxxnever::Page page = {};
	page.method  = convert(req->method);
	page.headers = convert(req->headers);
	page.input   = convert(req->input);
	page.url     = convert(req->url);
	page.path    = convert(req->path);
	page.query_string = convert(req->query_string);

	auto& list = cxxnever::Pager::list();
	auto it = list.begin(), end = list.end();
	for ( ; it != end; ++it)
		if (page.path == it->first && !strchr(it->first.c_str(), '('))
			break;

	if (it == end) {
		std::string path {page.path};
		std::cmatch m;
		for (it = list.begin(); it != end; ++it) {
			std::regex re {it->first};
			if (std::regex_match(path.c_str(), m, re))
				break;
		}

		if (it == end)
			return (req->output_code = 404), 0;

		for (size_t i = 1; i < m.size(); ++i)
			page.args.emplace_back(m[i]);
	}

	it->second(page);

	if (page.code == 0)
		if (page.output_headers.empty())
			page.header("Content-Type", "text/html; charset=utf-8");

	req->output_code = page.code;
	req->output_headers = convert(page.output_headers);
	req->output_body = convert(page.output_body);

	(*req->callback)(req->callback_data, req);

	return 0;
}

}
