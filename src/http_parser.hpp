#pragma once
#include <string>
#include "http_parser_decoder.hpp"
#include "narrow.hpp"


namespace cxxnever
{

struct HttpParser
{
	std::string_view header;
	std::string_view first;
	std::string_view method;
	std::string_view url;
	std::string_view version;
	std::string_view params;
	std::string_view input;

	std::string_view url_path;
	std::string_view path_info;
	std::string_view query_string;

	std::string url_path_storage;

	int return_code = 0;

	size_t parse(std::string_view in)
	{
		using namespace cxxnever::narrow;

		header = view(in, right, "\r\n\r\n");
		if (header.empty())
			return 0;

		first = view(header, right, "\r\n");
		method = view(first, right, "\x20");
		url = view(first, left, "\x20", right, "\x20");
		version = view(first, left, "\x20", left, "\x20");
		params = view(header, left, "\r\n");
		input = view(in, left, "\r\n\r\n");

		url_path = view(url, right_or_not, "?");
		query_string = view(url, left, "?");

		if (method == "POST") {
			auto len_str = find("Content-Length");
			char buf[21] = {};
			len_str.copy(buf, sizeof(buf));
			size_t len = strtoull(buf, nullptr, 10);
			if (input.size() < len)
				return 0;
			input = std::string_view {&input[0], &input[0] + len};
		} else {
			input = {};
		}

		if (method != "GET" && method != "POST")
			return_code = 405;

		if (url_path.empty())
			return_code = 400;

		UrlParserDecoder p;
		if (p.decode(url_path, url_path_storage) == -1)
			return_code = 400;

		return header.size() + strlen("\r\n\r\n") + input.size();
	}

	bool next_header(std::string_view& all, std::string_view& name,
	                                        std::string_view& value)
	{
		using namespace cxxnever::narrow;
		std::string_view line = view(all, right_or_not, "\r\n");
		name = view(line, right, ":");
		value = view(line, left, ":", trim_left);
		all = view(all, left, "\r\n");
		return name.size();
	}

	std::string_view find(std::string_view header_name, const char* v = "")
	{
		std::string_view hdrs = params;
		std::string_view hdr_name;
		std::string_view hdr_value;
		while (next_header(hdrs, hdr_name, hdr_value))
			if (hdr_name == header_name)
				return hdr_value;
		return v;
	}
};

}
