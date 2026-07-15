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
include(VkApp)
include(CompilerSetup)
include(SourceCollections)
include(Dependencies)
include(LibraryTargets)
include(Stastics)

# tools 
if(ENABLE_TOOLING)
  enable_testing()
  add_subdirectory(tools)
endif()

# examples
if(NOT DEFINED ENABLE_EXAMPLES AND DEFINED ENABLE_EXECUTABLES)
  set(ENABLE_EXAMPLES ${ENABLE_EXECUTABLES})
endif()

if(ENABLE_EXAMPLES)
  add_subdirectory(examples)
endif()
