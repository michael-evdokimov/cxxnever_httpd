#pragma once
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include "logging.hpp"


namespace cxxnever
{

struct ContextSsl
{
	SSL_CTX* ctx = nullptr;

	~ContextSsl()
	{
		if (ctx)
			SSL_CTX_free(ctx);
	}

	bool init()
	{
		SSL_library_init();
		OpenSSL_add_all_algorithms();
		OpenSSL_add_all_ciphers();
		SSL_load_error_strings();
		ERR_load_crypto_strings();
		return true;
	}

	int open(const char* file_cert_pem, const char* file_key_pem)
	{
		static bool loaded = init();
		const SSL_METHOD* method = TLS_server_method();
		ctx = SSL_CTX_new(method);
		if (!ctx) {
			long unsigned err = ERR_get_error();
			log(ERROR, "SSL err: %lu", err);
			log(ERROR, "SSL err: %s",ERR_error_string(err,nullptr));
			log(ERROR, "SSL err: %s", ERR_reason_error_string(err));
			log(ERROR, "Cannot open ssl context");
			return -1;
		}

		if (!file_cert_pem || !file_key_pem)
			return log(ERROR, "SSL cert or key is missing"), -1;

		if (SSL_CTX_use_certificate_file(ctx, file_cert_pem,
		                                 SSL_FILETYPE_PEM) <= 0) {
			log(ERROR, "Cannot SSL_CTX_use_certificate_file()");
			return -1;
		}

		if (SSL_CTX_use_PrivateKey_file(ctx, file_key_pem,
		                                SSL_FILETYPE_PEM) <= 0) {
			log(ERROR, "Cannot SSL_CTX_use_PrivateKey_file()");
			return -1;
		}

		return 0;
	}
};

}
