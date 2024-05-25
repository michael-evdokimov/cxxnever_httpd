#pragma once
#include "http_stream.hpp"
#include "http_parser.hpp"


namespace cxxnever
{

struct HttpExecutor
{
	virtual ~HttpExecutor()
	{
	}

	virtual const std::string& get_file_ext() const = 0;

	virtual bool want(std::string_view path) const = 0;

	virtual void process(HttpStream&, HttpParser&, int notify_fd) = 0;
};

}
