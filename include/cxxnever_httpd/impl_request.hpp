#pragma once
#include <cstdint>


extern "C" {

struct executing_request_value_t
{
	const char* value;
	size_t value_len;
};

struct executing_request_t
{
	executing_request_value_t method;
	executing_request_value_t headers;
	executing_request_value_t input;
	executing_request_value_t url;
	executing_request_value_t path;
	executing_request_value_t query_string;

	int output_code;

	executing_request_value_t output_headers;
	executing_request_value_t output_body;

	void (*callback)(void* private_, struct executing_request_t*);
	void* callback_data;

	void (*log_callback)(void* private_, const char* message);
	void* log_callback_data;
};

}
