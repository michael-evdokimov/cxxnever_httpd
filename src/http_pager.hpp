#pragma once
#include <shared_mutex>
#include <sys/stat.h>
#include <sys/mman.h>
#include <dlfcn.h>
#include "http_executor.hpp"
#include <cxxnever_httpd/impl_request.hpp>


namespace cxxnever
{

struct HttpPageFile
{
	std::string path;
	int memfd = -1;
	void* handle = nullptr;
	int64_t mtime = 0;

	int (*execute)(struct executing_request_t*);

	~HttpPageFile()
	{
		if (handle)
			dlclose(handle);
		if (memfd != -1)
			close(memfd);
	}
};


struct HttpPager : HttpExecutor
{
	std::string file_ext;

	typedef std::shared_ptr<HttpPageFile> HttpPageFilePtr;

	std::unordered_map<std::string_view, HttpPageFilePtr> files;
	std::shared_mutex files_mutex;

	const std::string& get_file_ext() const override
	{
		return file_ext;
	}

	bool want(std::string_view url_path) const override
	{
		return url_path.ends_with(file_ext);
	}

	int64_t get_mtime(const char* filename)
	{
		struct stat p = {};
		if (stat(filename, &p) == -1)
			return -1;
		return p.st_mtime;
	}

	HttpPageFilePtr openlib(int& code, const char* libpath)
	{
		std::shared_ptr<HttpPageFile> file {new HttpPageFile};

		file->memfd = memfd_create(strrchr(libpath, '/'), 0);
		if (file->memfd == -1) {
			log(ERROR, "Cannot memfd_create()");
			code = 502;
			return {};
		}

		std::string buf;
		std::ifstream input {libpath};
		input.seekg(0, input.end);
		buf.resize(input.tellg());
		input.seekg(0);
		input.read(&buf[0], buf.size());
		if (input.gcount() != buf.size()) {
			log(ERROR, "Cannot read from %s", libpath);
			code = 502;
			return {};
		}
		ssize_t r = write(file->memfd, &buf[0], buf.size());
		if (r != buf.size()) {
			log(ERROR, "Cannot write to memfd: %m");
			code = 502;
			return {};
		}

		char fn[25] = {};
		snprintf(fn, sizeof(fn), "/proc/self/fd/%d", file->memfd);
		file->handle = dlmopen(LM_ID_NEWLM, fn, RTLD_LAZY);
		if (!file->handle) {
			code = 502;
			return {};
		}

		file->execute = (decltype(file->execute))
		                                 dlsym(file->handle, "execute");
		if (!file->execute) {
			log(ERROR, "cxxpage has not func execute");
			code = 502;
			return {};
		}

		file->mtime = get_mtime(libpath);
		if (file->mtime == -1) {
			code = 502;
			if (errno == ENOENT)
				code = 404;
			return {};
		}

		return file;
	}

	void process(HttpStream& strm, HttpParser& parser, int) override
	{
		std::shared_ptr<HttpPageFile> file;

		std::shared_lock lock {files_mutex};
		auto it = files.find(strm.full_path);
		if (it != files.end())
			file = it->second;
		lock.unlock();

		if (file) {
			int64_t mtime = get_mtime(strm.full_path.c_str());
			if (mtime > file->mtime || mtime == -1) {
				std::unique_lock lock {files_mutex};
				files.erase(strm.full_path);
				lock.unlock();
				file = {};
			}
		}

		if (!file) {
			file = openlib(strm.code, strm.full_path.c_str());
			if (!file)
				return;

			file->path = strm.full_path;

			std::unique_lock lock {files_mutex};
			files.emplace(file->path, file);
			lock.unlock();
		}

		struct executing_request_t req = {};
		req.method = convert(parser.method);
		req.headers = convert(parser.params);
		req.input = convert(parser.input);
		req.url = convert(parser.url_path);
		req.path = convert(parser.path_info);
		req.query_string = convert(parser.query_string);
		req.callback = &HttpPager::flush;
		req.callback_data = &strm;
		req.log_callback = &logger;
		req.log_callback_data = nullptr;

		(*file->execute)(&req);

		strm.code = req.output_code ? req.output_code : 200;

		if (strm.headers.find("Content-Length") == -1)
			strm.header("Content-Length", strm.body.size());
	}

	static void flush(void* that, struct executing_request_t* req)
	{
		HttpStream& strm = *static_cast<HttpStream*>(that);

		strm.headers += convert(req->output_headers);
		strm.body += convert(req->output_body);
	}

	static void logger(void*, const char* msg)
	{
		log(INFO, "Page: %s", msg);
	}

	static executing_request_value_t convert(std::string_view value)
	{
		executing_request_value_t r;
		r.value = &value[0];
		r.value_len = value.size();
		return r;
	}

	static std::string_view convert(executing_request_value_t value)
	{
		return std::string_view {value.value, value.value_len};
	}
};

}
