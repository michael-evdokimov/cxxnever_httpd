#pragma once
#include <cstring>
#include <string>


namespace cxxnever
{
namespace impl
{

struct query_string_parser_t
{
	std::string_view get(std::string_view line, std::string_view name)
	{
		size_t n = name.size();
		size_t idx = 0;
		size_t i = -1;
		while (true) {
			i = line.find('=', idx);
			if (i == -1)
				return {};
			if (i >= n)
				if (memcmp(&line[i - n], &name[0], n) == 0)
					if (i == n || line[i - n - 1] == '&')
						break;
			idx = ++i;
		}

		size_t e = line.find('&', ++i);
		if (e == -1)
			e = line.size();

		return std::string_view {&line[i], e - i};
	}
};

}
}
