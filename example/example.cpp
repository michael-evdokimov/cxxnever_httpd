#include <cxxnever_httpd/page.hpp>
#include <cxxnever_httpd/main.hpp>


static void mainpage(cxxnever::Page& page)
{
	page.header("Location", std::string(page.url) + "/");
	page.code = 301;
}
cxxnever::Pager mainpager {mainpage, ""};


static void hello(cxxnever::Page& page)
{
	page << "<h1> Hello world! </h1>\n";

	page << "Click <a href=account/13?format=xml&search=word>"
	        "here</a> <br>\n";
}
cxxnever::Pager hello_ {hello, "/"};


static void account(cxxnever::Page& page)
{
	std::string& id = page.args[0];
	page << "<h1> Account " << id << " </h1>\n";

	std::string_view format = page.get_query_sview("format");
	if (format == "xml")
		page << "XML format is requested. <br>\n";

	std::string search = page.get_query_str("search");
	if (search.size())
		page << "Searching for " << search << "; <br>\n";

	std::string_view user_agent = page.find_header("User-Agent");
	if (user_agent.find("Linux") != -1)
		page << "Hello you from Linux <br>\n";
}
cxxnever::Pager account_ {account, "/account/([0-9]+)"};
