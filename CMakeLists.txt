cmake_minimum_required (VERSION 3.8)

project ("stemlib")

add_executable (stemlib "stemlib.cpp" "stemlib.hpp")

set_property (TARGET stemlib PROPERTY CXX_STANDARD 20)

set (BUILD_SHARED_LIBS OFF)
add_subdirectory ("external/poco")

target_link_libraries (stemlib Poco::Net)