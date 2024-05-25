cmake_minimum_required(VERSION 3.5)

project(cxxnever_httpd)

find_library(LIB_SSL libssl.so REQUIRED)
find_library(LIB_CRYPTO libcrypto.so REQUIRED)

set(CMAKE_CXX_STANDARD "20")
set(CMAKE_CXX_FLAGS "-ggdb3")

file(GLOB HEADERS src/*.hpp)

add_executable(cxxnever_httpd src/main.cpp ${HEADERS})

target_include_directories(cxxnever_httpd PUBLIC ${CMAKE_SOURCE_DIR}/include)

target_link_libraries(cxxnever_httpd ${LIB_SSL} ${LIB_CRYPTO})

install(TARGETS cxxnever_httpd DESTINATION bin)

install(FILES ${CMAKE_SOURCE_DIR}/etc/httpd.conf
	DESTINATION /etc/cxxnever_httpd/ COMPONENT config)

file(GLOB LIB_HEADERS include/cxxnever_httpd/*)

install(FILES ${LIB_HEADERS} DESTINATION /usr/include/cxxnever_httpd/)

add_subdirectory(example)
