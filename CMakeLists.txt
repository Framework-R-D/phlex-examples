cmake_minimum_required(VERSION 3.31)
project(my_phlex_example VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)

find_package(Boost REQUIRED json)
find_package(TBB REQUIRED)
find_package(spdlog REQUIRED)
find_package(phlex REQUIRED)

# Proxies for framework-agnostic experiment libraries
# N.B. The specified library type should be SHARED.
add_library(my_add SHARED my_add.cpp)

# Register providers for data products
add_library(ij_source MODULE ij_source.cpp)
target_link_libraries(ij_source PRIVATE phlex::module)

# Register experiment libraries with Phlex
add_library(module MODULE module.cpp)
target_link_libraries(module PRIVATE phlex::module my_add)
