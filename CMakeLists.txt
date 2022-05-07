cmake_minimum_required (VERSION 3.8)

project ("stemlib")

add_executable (stemlib "stemlib.cpp" "stemlib.hpp")

target_compile_features (stemlib PRIVATE cxx_std_20)

find_package (asio CONFIG REQUIRED)
target_link_libraries (stemlib PRIVATE asio asio::asio)

#use vcpkg