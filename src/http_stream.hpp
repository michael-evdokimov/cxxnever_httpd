#pragma once
#include <string>
#include <vector>
#include "logging.hpp"
#include "http_return_codes.hpp"
#include "fast_cgi_client.hpp"


namespace cxxnever
{

struct HttpStream
{
	std::string full_path;

	bool received = false;
	bool sent = false;
	int code = 0;
	std::string headers;

	std::string headers_raw;
	std::string body;

	std::ifstream file;
	size_t file_start = 0;
	size_t file_end = 0;

	std::shared_ptr<FastCGIClient::Request> cgi_req;

	bool headers_flushed = false;
	bool may_close = false;

	std::string output_buffer;


	void join_headers()
	{
		char buf[0x100] = {};
		if (code == 0)
			code = 200;
		const char *s_code = http_code_str(code);
		snprintf(buf, sizeof(buf), "HTTP/1.1 %d %s\r\n", code, s_code);
		headers_raw = {};
		headers_raw += buf;
		headers_raw += headers;
		headers_raw += "\r\n";
		if (body.size() && body.size() < 64 * 1024) {
			headers_raw += body;
			body = {};
		}
	}

	void header(const char* name, std::string_view value, const char* add=0)
	{
		headers += name;
		headers += ": ";
		headers += value;
		headers += add ? add : "";
		headers += "\r\n";
	}

	void header(const char* name, size_t value)
	{
		char buf[0x100] = {};
		snprintf(buf, sizeof(buf), "%s: %zd\r\n", name, value);
		headers += buf;
	}

	HttpStream& operator << (std::string_view str)
	{
		body += str;
		return *this;
	}

	void push_file(std::ifstream file, size_t start, size_t end)
	{
		size_t len = end - start;
		if (len < 64 * 1024) {
			body.resize(0);
			body.resize(len);
			file.seekg(start);
			file.read(&body[0], len);
			if (file.gcount() == len)
				return;
		}
		this->file = std::move(file);
		file_start = start;
		file_end = end;
	}
};

struct HttpStreamReader
{
	HttpStream& strm;

	int read_next(std::string_view& ret)
	{
		if (!strm.headers_flushed) {
			if (strm.headers_raw.empty())
				strm.join_headers();
			ret = strm.headers_raw;
			return 0;
		}

		if (strm.body.size()) {
			ret = strm.body;
			return 0;
		}

		if (strm.file_start < strm.file_end) {
			strm.file.seekg(strm.file_start);
			size_t chunk_size = strm.file_end - strm.file_start;
			size_t size = std::min(size_t(64 * 1024), chunk_size);
			std::string& buf = strm.output_buffer;
			if (buf.size() < size)
				buf.resize(size);
			strm.file.read(&buf[0], size);
			if (strm.file.gcount() != size)
				return -1;
			ret = std::string_view {&buf[0], size};
			return 0;
		}

		ret = {};
		return 0;
	}

	void commit(size_t n)
	{
		if (strm.headers_raw.size()) {
			strm.headers_raw.erase(0, n);
			if (strm.headers_raw.empty())
				strm.headers_flushed = true;

			if (strm.headers_raw.empty())
				if (strm.body.empty())
					if (strm.file_start == strm.file_end)
						strm.sent = true;

			return;
		}

		if (strm.body.size()) {
			strm.body.erase(0, n);
			if (strm.body.empty())
				if (strm.file_start == strm.file_end)
					strm.sent = true;
			return;
		}

		if (strm.file_start < strm.file_end) {
			strm.file_start += n;
			if (strm.file_start == strm.file_end)
				strm.sent = true;
			return;
		}

		throw std::runtime_error("Stream all sent");
	}
};

}
