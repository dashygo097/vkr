cmake_minimum_required(VERSION 3.24)
project(vkr LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Prequisites
find_package(Vulkan REQUIRED COMPONENTS MoltenVK)

# MoltenVK
if (NOT Vulkan_MOLTENVK_LIBRARY)
    set(Vulkan_MOLTENVK_LIBRARY "${Vulkan_INCLUDE_DIRS}/../lib/libMoltenVK.dylib")
endif()

# 3rdparty
add_subdirectory(3rdparty/glfw-3.4 EXCLUDE_FROM_ALL)
include_directories(3rdparty)


set(GLFW_BUILD_DOCS
    OFF
    CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS
    OFF
    CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES
    OFF
    CACHE BOOL "" FORCE)

if (TARGET glfw)
    get_target_property(GLFW_VERSION_STRING glfw VERSION)
endif()

# Colored printing
function(print_info text color)
    if(CMAKE_COLOR_MAKEFILE)
        execute_process(COMMAND printf "\\033[1;${color}m${text}\\033[0m\\n")
    else()
        message(STATUS "${text}")
    endif()
endfunction()

print_info("――――――――――――――――――LIB SCOPE――――――――――――――――――" "91")

# Add targets
if(APPLE)
    print_info("[INFO] Vulkan SDK: ${Vulkan_INCLUDE_DIRS}" "90")
    print_info("[INFO] MoltenVK: ${Vulkan_MOLTENVK_LIBRARY}" "90")
    print_info("[INFO] GLFW version: ${GLFW_VERSION_STRING}" "90")
endif()

# vkr
add_library(vkr
    src/test.cpp
)
target_include_directories(vkr PUBLIC include)
set_target_properties(vkr PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY src
    LIBRARY_OUTPUT_DIRECTORY src
)

# Apps
add_subdirectory(demos)
