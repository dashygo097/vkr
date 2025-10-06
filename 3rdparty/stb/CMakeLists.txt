cmake_minimum_required(VERSION 3.24)
project(stb LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)

add_library(stb INTERFACE)
target_include_directories(stb INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
