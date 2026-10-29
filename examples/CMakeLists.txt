cmake_minimum_required(VERSION 3.24)
project(vkr_examples LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(vkr CONFIG REQUIRED)

add_subdirectory(shadertoy)
add_subdirectory(skybox)
add_subdirectory(teapot)
add_subdirectory(vector_ops)
add_subdirectory(mmul)
add_subdirectory(rotate_cube)
add_subdirectory(softmax)
