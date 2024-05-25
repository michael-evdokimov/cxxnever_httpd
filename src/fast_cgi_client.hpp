#pragma once
#include <unordered_map>
#include <shared_mutex>
#include "client_tcp.hpp"
#include "fast_cgi_types.hpp"


namespace cxxnever
{

struct FastCGIClient
{
	ClientTcp client;

	struct Request
	{
		int notify_fd = -1;
		uint16_t req_id = 0;
		bool done = false;
		std::string output;
		std::mutex mutex;

		~Request() { ::close(notify_fd); }
	};

	std::unordered_map<uint16_t, std::shared_ptr<Request>> list;
	std::shared_mutex list_mutex;

	std::string inbuf;
	size_t inbuf_len = 0;
	size_t parsed_len = 0;

	std::mutex write_mutex;


	~FastCGIClient()
	{
		int64_t next = 1;
		for (auto& p: list) {
			p.second->done = true;
			::write(p.second->notify_fd, &next, sizeof(next));
		}
	}

	int open(const char* unix_socket)
	{
		return client.open(unix_socket);
	}

	void new_request(std::shared_ptr<Request> req)
	{
		std::unique_lock lock {list_mutex};
		for (uint16_t i = 1; i; ++i)
			if (list.count(i) == 0)
				return (void) (req->req_id = i, list[i] = req);

		throw std::logic_error("FastCGI req id all busy");
	}

	int send_begin_request(uint16_t req_id, uint8_t flags)
	{
		using namespace cxxnever::fastcgi;

		FCGI_BeginRequestRecord data = {};

		data.header.version = FCGI_VERSION_1;
		data.header.type = FCGI_BEGIN_REQUEST;
		data.header.requestIdB1 = req_id >> 8;
		data.header.requestIdB0 = req_id & 0xff;
		data.header.contentLengthB1 = 0;
		data.header.contentLengthB0 = sizeof(data.body);
		data.header.paddingLength = 0;

		data.body.roleB1 = 0;
		data.body.roleB0 = FCGI_RESPONDER;
		data.body.flags = flags;

		std::unique_lock lock {write_mutex};
		int r = ::write(client.fd, &data, sizeof(data));
		lock.unlock();
		if (r == -1)
			return log(ERROR, "Cannot write: %m"), -1;
		if (r != sizeof(data))
			return log(ERROR, "Cannot write sizeof buf"), -1;

		return 0;
	}

	void put_size(char*& p, size_t num)
	{
		if (num <= 127) {
			*p++ = num;
		} else {
			*p++ = (num >> 24) | 0x80;
			*p++ = num >> 16;
			*p++ = num >> 8;
			*p++ = num;
		}
	}

	typedef std::pair<std::string_view, std::string_view> OneParam;

	int send_params(uint16_t req_id, const std::vector<OneParam>& params)
	{
		using namespace cxxnever::fastcgi;

		thread_local std::string buf;
		buf.resize(0);
		buf.resize(sizeof(FCGI_Header));

		for (const OneParam& value: params) {
			char param_buf[8] = {};
			char* p = param_buf;
			put_size(p, value.first.size());
			put_size(p, value.second.size());
			buf.append(param_buf, p);
			buf += value.first;
			buf += value.second;
		}

		auto* header = reinterpret_cast<FCGI_Header*>(&buf[0]);
		header->version = FCGI_VERSION_1;
		header->type = FCGI_PARAMS;
		header->requestIdB1 = req_id >> 8;
		header->requestIdB0 = req_id & 0xff;
		size_t sz = buf.size() - sizeof(*header);
		header->contentLengthB1 = sz >> 8;
		header->contentLengthB0 = sz & 0xff;
		header->paddingLength = (8 - sz % 8) % 8;
		buf.resize(buf.size() + header->paddingLength);

		std::unique_lock lock {write_mutex};
		size_t r = ::write(client.fd, &buf[0], buf.size());
		lock.unlock();
		if (r == -1)
			return log(ERROR, "Cannot write: %m"), -1;
		if (r != buf.size())
			return log(ERROR, "Cannot write sizeof buf"), -1;

		return 0;
	}

	int send_stdin(uint16_t req_id, std::string_view data)
	{
		using namespace cxxnever::fastcgi;

		thread_local std::string buf;
		buf.resize(0);
		buf.resize(sizeof(FCGI_Header));
		buf += data;

		auto* header = reinterpret_cast<FCGI_Header*>(&buf[0]);
		header->version = FCGI_VERSION_1;
		header->type = FCGI_STDIN;
		header->requestIdB1 = req_id >> 8;
		header->requestIdB0 = req_id & 0xff;
		size_t sz = data.size();
		header->contentLengthB1 = sz >> 8;
		header->contentLengthB0 = sz & 0xff;
		header->paddingLength = (8 - sz % 8) % 8;
		buf.resize(buf.size() + header->paddingLength);

		std::unique_lock lock {write_mutex};
		size_t r = ::write(client.fd, &buf[0], buf.size());
		lock.unlock();
		if (r == -1)
			return log(ERROR, "Cannot write: %m"), -1;
		if (r != buf.size())
			return log(ERROR, "Cannot write sizeof buf"), -1;

		return 0;
	}

	ssize_t parse(std::string_view data)
	{
		using namespace fastcgi;

		auto* header = reinterpret_cast<const FCGI_Header*>(&data[0]);
		if (data.size() < sizeof(*header))
			return 0;

		if (header->type == FCGI_STDOUT || header->type == FCGI_STDERR){
			const char* str = &data[0] + sizeof(*header);
			uint16_t req_id = 0;
			req_id += header->requestIdB1 * 0x100;
			req_id += header->requestIdB0;
			size_t len = 0;
			len += header->contentLengthB1 * 0x100;
			len += header->contentLengthB0;
			size_t whole_len = sizeof(*header) + len +
			                                  header->paddingLength;
			if (data.size() < whole_len)
				return 0;
			if (header->type == FCGI_STDOUT) {
				std::shared_lock list_lock {list_mutex};
				auto req = list.at(req_id);
				list_lock.unlock();
				std::unique_lock lock {req->mutex};
				req->output.append(str, str + len);
			} else {
				std::string s(str, str + len);
				log(ERROR, "FCGI: %s", s.c_str());
			}

			return whole_len;
		}

		if (header->type == FCGI_END_REQUEST) {
			uint16_t req_id = 0;
			req_id += header->requestIdB1 * 0x100;
			req_id += header->requestIdB0;
			size_t len = 0;
			len += header->contentLengthB1 * 0x100;
			len += header->contentLengthB0;
			size_t whole_len = sizeof(*header) + len +
			                                  header->paddingLength;
			if (data.size() < whole_len)
				return 0;

			std::unique_lock lock {list_mutex};
			auto it = list.find(req_id);
			if (it != list.end()) {
				auto req = it->second;
				req->done = true;
				list.erase(it);
				lock.unlock();

				int64_t next = 1;
				::write(req->notify_fd, &next, sizeof(next));
			}
			return whole_len;
		}

		log(ERROR, "Unknown FCGI type %d", int(header->type));

		return -1;
	}

	int read()
	{
		if (inbuf_len == inbuf.size())
			inbuf.resize(inbuf.size() * 2 + 4 * 1024);

		size_t space = inbuf.size() - inbuf_len;
		size_t r = ::read(client.fd, &inbuf[inbuf_len], space);
		if (r == -1)
			return log(ERROR, "Cannot read: %m"), -1;
		if (r == 0)
			return log(ERROR, "Empty read from fcgi"), -1;

		inbuf_len += r;

		while (true) {
			size_t wehave = inbuf_len - parsed_len;
			std::string_view tail {&inbuf[parsed_len], wehave};
			ssize_t r = parse(tail);
			if (r == -1)
				return log(ERROR, "Cannot parse"), -1;
			if (r == 0)
				break;
			parsed_len += r;
		}
		if (parsed_len == inbuf_len)
			parsed_len = 0, inbuf_len = 0;

		return 0;
	}
};

}
