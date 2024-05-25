#pragma once
#include <string>
#include <vector>
#include "impl_query_string.hpp"
#include "impl_decoder.hpp"
#include "impl_header_finder.hpp"


namespace cxxnever
{

struct Page
{
	std::string_view method;
	std::string_view headers;
	std::string_view input;
	std::string_view url;
	std::string_view path;
	std::string_view query_string;

	int code = 0;
	std::string output_headers;
	std::string output_body;

	std::vector<std::string> args;

	void header(const char* name, std::string_view value)
	{
		output_headers += name;
		output_headers += ": ";
		output_headers += value;
		output_headers += "\r\n";
	}

	Page& operator << (std::string_view str)
	{
		output_body += str;
		return *this;
	}

	std::string_view get_query_sview(std::string_view name)
	{
		impl::query_string_parser_t p;
		return p.get(query_string, name);
	}

	std::string get_query_str(std::string_view name)
	{
		impl::query_string_parser_t p;
		impl::url_decoder_t d;
		std::string_view value = p.get(query_string, name);
		return d.decode(value);
	}

	std::string_view find_header(std::string_view name)
	{
		impl::header_finder_t p;
		return p.find(headers, name);
	}
};

}
