#pragma once
#include <fstream>
#include <dirent.h>
#include "http_stream.hpp"
#include "http_service.hpp"
#include "narrow.hpp"


namespace cxxnever
{

struct HttpDirectory
{
	const HttpVirtualHost* vhost = nullptr;
	HttpParser& parser;

	HttpExecutor* executor = nullptr;


	std::string_view get_ext(std::string_view path)
	{
		using namespace narrow;
		return view(path, right_left, "/", right_left, ".");
	}

	void show_file(HttpStream& strm, std::ifstream file)
	{
		for (auto& exe: vhost->executors)
			if (exe->want(strm.full_path))
				return (void) (executor = &*exe);

		file.seekg(0, file.end);
		size_t file_size = file.tellg();
		file.seekg(0);

		std::string_view ext = get_ext(strm.full_path);
		if (ext == "html")
			strm.header("Content-Type", "text/html; charset=utf-8");
		else
			strm.header("Content-Type", "text/plain");

		strm.header("Content-Length", file_size);
		strm.push_file(std::move(file), 0, file_size);
	}

	void show_dir(HttpStream& strm, std::string_view path, DIR* dir)
	{
		struct closer { DIR* d = 0; ~closer() { closedir(d); } };
		closer cl {dir};

		if (path.size() && path.back() != '/') {
			strm.code = 301;
			strm.header("Location", path, "/");
			return;
		}

		strm << "<h3> " << path << " </h3>\n";
		while (struct dirent* entry = readdir(dir)) {
			const char* n = entry->d_name;
			strm << "entry ";
			strm << "<a href=\"" << n << "\">" << n << "</a><br>\n";
		}

		strm.header("Content-Type", "text/html; charset=utf-8");
		strm.header("Content-Length", strm.body.size());
	}

	void show(HttpStream& strm, std::string_view path)
	{
		if (path.find("/../") + 1)
			return (void) (strm.code = 400);

		strm.full_path = vhost->root;
		strm.full_path += path;

		for (auto& exe: vhost->executors) {
			const std::string& ext = exe->get_file_ext();
			if (size_t i = strm.full_path.find(ext) + 1) {
				if (strm.full_path[i-1 + ext.size()] == '/') {
					strm.full_path.resize(i-1 + ext.size());
					size_t skip = 0;
					skip += strm.full_path.size();
					skip -= vhost->root.size();
					parser.path_info = path;
					parser.path_info.remove_prefix(skip);
				}
				if (access(strm.full_path.c_str(), R_OK) == 0) {
					executor = &*exe;
					return;
				} else {
					strm.full_path = vhost->root;
					strm.full_path += path;
					parser.path_info = {};
				}
			}
		}

		if (path.size() && path.back() == '/') {
			size_t saved = strm.full_path.size();
			for (const std::string& index: vhost->index_files) {
				strm.full_path += index;
				std::ifstream f {strm.full_path};
				if (f)
					return show_file(strm, std::move(f));
				strm.full_path.resize(saved);
			}
		}

		if (DIR* dir = opendir(strm.full_path.c_str()))
			return show_dir(strm, path, dir);
		else if (errno == ENOENT)
			return (void) (strm.code = 404);
		else if (errno == EACCES)
			return (void) (strm.code = 403);

		if (std::ifstream f {strm.full_path})
			return show_file(strm, std::move(f));
		else if (errno == ENOENT)
			return (void) (strm.code = 404);
		else if (errno == EACCES)
			return (void) (strm.code = 403);

		if (errno == ENOTDIR && false) {
			std::ifstream f;
			while (errno == ENOTDIR) {
				size_t i = strm.full_path.rfind('/');
				strm.full_path[i] = 0;
				errno = 0;
				f.open(strm.full_path.c_str());
			}

			strm.full_path = strm.full_path.c_str();

			size_t skip = strm.full_path.size()-vhost->root.size();
			parser.path_info = path;
			parser.path_info.remove_prefix(skip);

			for (auto& exe: vhost->executors)
				if (exe->want(strm.full_path))
					return (void) (executor = &*exe);
		}

		strm.code = 400;
		return;
	}
};

}
