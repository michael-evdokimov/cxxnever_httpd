#pragma once
#include <cstdint>
#include <string>


namespace cxxnever
{
namespace impl
{

struct url_decoder_t
{
	std::string decode(std::string_view input)
	{
		std::string r;
		r.reserve(input.size());

		char buf[4] = {};
		uint8_t i = 0;
		for (char c: input) {
			if (c != '%' && i == 0) {
				r += c;
				continue;
			}

			buf[i++] = c;
			if (i < 3)
				continue;

			i = 0;
			char* end = nullptr;
			uint8_t num = strtoull(&buf[1], &end, 16);
			if (*end != 0) {
				r += '?';
				continue;
			}

			r += char(num);
		}

		return r;
	}
};

}
}
