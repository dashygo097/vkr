cmake_minimum_required(VERSION 3.24)
project(vkr LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Add cmake module path
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
message(STATUS "CMake module path: ${CMAKE_MODULE_PATH}")

# Generate and include build configuration
if(EXISTS "${CMAKE_CURRENT_BINARY_DIR}/config.cmake")
  include("${CMAKE_CURRENT_BINARY_DIR}/config.cmake")
endif()

include(Helpers)
include(CompilerSetup)
include(SourceCollections)
include(Dependencies)
include(LibraryTargets)
include(Stastics)

# tests 
if(ENABLE_TESTING)
  enable_testing()
  add_subdirectory(tests)
endif()

# excutables 
if(ENABLE_EXECUTABLES) 
  add_subdirectory(tools)
endif()
