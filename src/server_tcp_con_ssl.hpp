#pragma once
#include "ssl_context.hpp"


namespace cxxnever
{

struct ConnectionTcpSsl : ConnectionTcp
{
	std::shared_ptr<ContextSsl> ssl_context;
	SSL* ssl = nullptr;
	bool established = false;
	int hang_check = 0;

	std::string inbuf;
	size_t parsed_len = 0;
	size_t inbuf_len = 0;


	ConnectionTcpSsl(std::shared_ptr<ContextSsl> ctx)
		: ssl_context(ctx)
	{
	}

	~ConnectionTcpSsl()
	{
		if (ssl) {
			SSL_shutdown(ssl);
			SSL_free(ssl);
		}
	}

	int establish()
	{
		if (!ssl) {
			ssl = SSL_new(ssl_context->ctx);
			if (ssl)
				SSL_set_fd(ssl, fd);
		}
		if (!ssl)
			return log(ERROR, "Cannot SSL_new()"), -1;

		if (SSL_accept(ssl) <= 0) {
			if (++hang_check > 1000) {
				log(ERROR, "Hang of SSL_accept is detected");
				return -1;
			}
			long unsigned err = ERR_get_error();
			if (err == 0)
				return 0;
			if (err == SSL_ERROR_WANT_READ)
				return 0;
			if (err == SSL_ERROR_WANT_WRITE)
				return 0;
			log(ERROR, "SSL err: %lu", err);
			log(ERROR, "SSL err: %s",ERR_error_string(err,nullptr));
			log(ERROR, "SSL err: %s", ERR_reason_error_string(err));
			log(ERROR, "Cannot SSL_accept()");
			return -1;
		}

		established = true;
		return 0;
	}

	int read() override
	{
		if (!established) {
			if (establish() == -1)
				return log(ERROR, "Cannot establish ssl"), -1;
			if (!established)
				return -EAGAIN;
		}

		size_t total = 0;
		int r = 1;
		while (r > 0) {
			if (inbuf_len == inbuf.size())
				inbuf.resize(inbuf.size() * 2 + 1024);
			size_t space = inbuf.size() - inbuf_len;
			size_t readed = 0;
			r = SSL_read_ex(ssl, &inbuf[inbuf_len], space, &readed);
			inbuf_len += readed;
			total += readed;
		}
		if (r <= 0) {
			long unsigned err = ERR_get_error();
			if (err == 0)
				return total ? 0 : -EAGAIN;
			if (err == SSL_ERROR_WANT_READ)
				return -EAGAIN;
			if (err == SSL_ERROR_WANT_WRITE)
				return -EAGAIN;
			log(ERROR, "SSL err: %lu", err);
			log(ERROR, "SSL err: %s",ERR_error_string(err,nullptr));
			log(ERROR, "SSL err: %s", ERR_reason_error_string(err));
			log(ERROR, "Cannot SSL_read_ex(), r = %d", r);
			return -1;
		}

		return total ? 0 : -EAGAIN;
	}

	void commit_read(size_t n) override
	{
		parsed_len += n;
		if (parsed_len == inbuf_len)
			parsed_len = 0, inbuf_len = 0;
	}

	std::string_view input() override
	{
		size_t n = inbuf_len - parsed_len;
		return std::string_view {&inbuf[parsed_len], n};
	}

	ssize_t write(const std::string_view& data) override
	{
		size_t written = 0;
		int r = SSL_write_ex(ssl, &data[0], data.size(), &written);
		if (r <= 0) {
			long unsigned err = ERR_get_error();
			if (err == 0)
				return 0;
			if (err == SSL_ERROR_WANT_READ)
				return -EAGAIN;
			if (err == SSL_ERROR_WANT_WRITE)
				return -EAGAIN;
			log(ERROR, "SSL err: %lu", err);
			log(ERROR, "SSL err: %s",ERR_error_string(err,nullptr));
			log(ERROR, "SSL err: %s", ERR_reason_error_string(err));
			log(ERROR, "Cannot SSL_write_ex(), r = %d", r);
			return -1;
		}

		return written;
	}
};

}
