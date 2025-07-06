cmake_minimum_required(VERSION 3.24)
project(stb LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

file(GLOB_RECURSE STB_HEADERS "*.h" "*.hpp")

add_library(stb STATIC ${STB_HEADERS})
target_include_directories(stb PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
