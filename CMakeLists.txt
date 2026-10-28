cmake_minimum_required(VERSION 3.24)
project(vkr VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Add cmake module path
list(PREPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
message(STATUS "CMake module path: ${CMAKE_MODULE_PATH}")

# Optional local settings override the shared defaults.
include("${CMAKE_CURRENT_BINARY_DIR}/config.cmake" OPTIONAL)
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/config.cmake")
include(GNUInstallDirs)

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
