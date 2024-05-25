#pragma once
#include <string>
#include <cstdio>
#include <time.h>
#include <sys/time.h>


namespace cxxnever
{

enum loglevel
{
	DEBUG,
	INFO,
	ERROR,
};

const char* loglevel_str(loglevel n)
{
	switch (n) {
		case DEBUG: return "DEBUG";
		case INFO:  return "INFO";
		case ERROR: return "ERROR";
		default:    return "UNKOWN";
	}
}

static const char* thread_name()
{
	thread_local const char* name = nullptr;
	if (name)
		return name;

	static std::string next = "A";
	thread_local std::string str_name = next;

	size_t i = 0;
	while (i < next.size()) {
		if (next[next.size() - i - 1]++ != 'Z')
			break;
		next[next.size() - i - 1] = 'A';
		i++;
	}
	if (i == next.size())
		next = "A" + next;

	name = str_name.c_str();
	return name;
}

template<typename... Args>
void log(loglevel level, const char* fmt, Args... args)
{
	char timestr[31] = {};
	struct tm tmp = {};
	struct timeval tv = {};
	gettimeofday(&tv, nullptr);
	time_t t = tv.tv_sec;
	localtime_r(&t, &tmp);
	strftime(timestr, sizeof(timestr), "%Y.%m.%dT%H:%M:%S", &tmp);

	auto* f_snprintf = &snprintf;
	thread_local std::string msg;
	size_t w = f_snprintf(&msg[0], msg.size(), fmt, args...);
	if (msg.size() <= w) {
		msg.resize(w + 1);
		f_snprintf(&msg[0], msg.size(), fmt, args...);
	}

	thread_local std::string buf;
	if (buf.size() < 64 + msg.size())
		buf.resize(64 + msg.size());
	const char* ls = loglevel_str(level);
	snprintf(&buf[0], buf.size(),
	         "%s.%06ld %s %s %s",
	                       timestr, tv.tv_usec, thread_name(), ls, &msg[0]);

	printf("%s\n", buf.c_str());
}

}
