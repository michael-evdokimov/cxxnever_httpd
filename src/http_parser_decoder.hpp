#pragma once
#include <string>


namespace cxxnever
{

struct UrlParserDecoder
{
	int decode(std::string_view& value, std::string& storage)
	{
		size_t i = 0;
		while (i != value.size() && value[i] != '%')
			++i;

		if (i == value.size())
			return 0;

		storage.reserve(value.size());
		storage.assign(&value[0], &value[i]);

		char buf[4] = {};
		uint8_t idx = 0;
		for ( ; i != value.size(); ++i) {
			if (value[i] != '%' && idx == 0) {
				storage += value[i];
				continue;
			}

			buf[idx++] = value[i];
			if (idx < 3)
				continue;

			idx = 0;
			char* end = nullptr;
			uint8_t num = strtoull(&buf[1], &end, 16);
			if (*end)
				return -1;

			storage += char(num);
		}

		value = storage;

		return idx ? -1 : 0;
	}
};

}
