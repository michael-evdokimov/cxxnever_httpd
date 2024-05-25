
add_library(example.cxxpage SHARED example.cpp ${LIB_HEADERS})

target_include_directories(example.cxxpage PUBLIC ${CMAKE_SOURCE_DIR}/include)
