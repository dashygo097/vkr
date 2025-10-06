cmake_minimum_required(VERSION 3.24)
project(tinyobjloader LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)

add_library(tinyobjloader INTERFACE)
target_include_directories(tinyobjloader INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
