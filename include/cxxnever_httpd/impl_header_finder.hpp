#pragma once
#include <string>


namespace cxxnever
{
namespace impl
{

struct header_finder_t
{
	std::string_view find(std::string_view buf, std::string_view name)
	{
		size_t n = name.size();
		size_t i = 0;
		size_t next = -1;
		while (true) {
			next = buf.find("\r\n", i);
			if (next == -1)
				next = buf.size();
			if (n < i - next)
				if (memcmp(&buf[i], &name[0], n) == 0)
					if (buf[i + n] == ':')
						break;
			if (next == buf.size())
				return {};
			i = next + strlen("\r\n");
		}

		i += n + 1;

		while (i < next && buf[i] == '\x20')
			++i;

		return std::string_view {&buf[i], next - i};
	}
};

}
}
