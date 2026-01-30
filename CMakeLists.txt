cmake_minimum_required(VERSION 3.31)
project(my_phlex_example VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)

find_package(Boost REQUIRED json)
find_package(TBB REQUIRED)
find_package(spdlog REQUIRED)
find_package(phlex REQUIRED)

add_subdirectory(gaus_hit_finder)

# Proxies for framework-agnostic experiment libraries
# N.B. The specified library type should be SHARED.
add_library(my_add SHARED my_add.cpp)

# Register providers for data products
add_library(numbers_source MODULE numbers_source.cpp)
target_link_libraries(numbers_source PRIVATE phlex::module)

# Register experiment libraries with Phlex
add_library(add_and_verify MODULE add_and_verify.cpp)
target_link_libraries(add_and_verify PRIVATE phlex::module my_add)
