### Introduction

This project is simple Web server which supports FastCGI and supports pages written is C++.
The server when requested loads shared library located at www/ folder and executes a page handler.
After execution the shared library with the page does not unload.

### Example

```cpp
#include <cxxnever_httpd/page.hpp>
#include <cxxnever_httpd/main.hpp>

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
```

```shell
g++ -o libexample.cxxpage.so -shared -fPIC example.cpp
cp libexample.cxxpage.so /var/www/html/
```
