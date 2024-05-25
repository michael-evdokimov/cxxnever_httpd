#pragma once
#include <string>
#include <cstring>


namespace cxxnever
{
namespace narrow
{

enum { left };
enum { right };
enum { right_left };
enum { right_or_not };
enum { trim_left };
enum { right_n };

static inline
std::string_view view(std::string_view str)
{
	return str;
}

template<typename... Args>
static inline
std::string_view
view(std::string_view str, decltype(left), const char* key, Args... args)
{
	size_t i = str.find(key);
	if (i == str.npos)
		return {};
	i += strlen(key);
	str.remove_prefix(i);
	return view(str, args...);
}

template<typename... Args>
static inline
std::string_view
view(std::string_view str, decltype(right), const char* key, Args... args)
{
	size_t i = str.find(key);
	if (i == str.npos)
		return {};
	str.remove_suffix(str.size() - i);
	return view(str, args...);
}

template<typename... Args>
static inline
std::string_view
view(std::string_view str, decltype(right_left), const char* key, Args... args)
{
	size_t i = str.rfind(key);
	if (i == str.npos)
		return {};
	i += strlen(key);
	str.remove_prefix(i);
	return view(str, args...);
}

template<typename... Args>
static inline
std::string_view
view(std::string_view str,decltype(right_or_not), const char* key, Args... args)
{
	size_t i = str.find(key);
	if (i != str.npos)
		str.remove_suffix(str.size() - i);
	return view(str, args...);
}

template<typename... Args>
static inline
std::string_view
view(std::string_view str, decltype(trim_left), Args... args)
{
	size_t i = str.find_first_not_of('\x20');
	if (i != str.npos)
		str.remove_prefix(i);
	return view(str, args...);
}

template<typename... Args>
static inline
std::string_view
view(std::string_view str, decltype(right_n), int n, Args... args)
{
	std::string_view ret {&*str.begin(), &*str.end() + n};

	return view(ret, args...);
}

}
}
