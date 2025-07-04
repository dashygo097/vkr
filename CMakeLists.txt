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
add_subdirectory(3rdparty/glfw EXCLUDE_FROM_ALL)
add_subdirectory(3rdparty/glm EXCLUDE_FROM_ALL)
add_subdirectory(3rdparty/imgui EXCLUDE_FROM_ALL)
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

function(make_paths_relative out_var)
    set(result "")
    foreach(path IN LISTS ${ARGV1})
        file(RELATIVE_PATH rel "${CMAKE_SOURCE_DIR}" "${path}")
        list(APPEND result "${rel}")
    endforeach()
    set(${out_var} "${result}" PARENT_SCOPE)
endfunction()

print_info("――――――――――――――――――LIB SCOPE――――――――――――――――――" "0")

# Add targets
if(APPLE)
    print_info("[INFO] Vulkan SDK: ${Vulkan_INCLUDE_DIRS}" "90")
    print_info("[INFO] MoltenVK: ${Vulkan_MOLTENVK_LIBRARY}" "90")
    print_info("[INFO] GLFW version: ${GLFW_VERSION_STRING}" "90")
endif()

# vkr
file(GLOB_RECURSE SOURCE_FILES src/*.cpp src/*.c src/*.cc)
file(GLOB_RECURSE HEADER_FILES include/*.h include/*.hpp)

add_library(vkr ${SOURCE_FILES} ${HEADER_FILES})
target_include_directories(vkr PUBLIC include)
target_link_libraries(vkr PUBLIC glfw Vulkan::Vulkan glm::glm-header-only imgui)

target_link_libraries(vkr PUBLIC
  "-framework Metal"
  "-framework Foundation"
  "-framework IOSurface"
)

set_target_properties(vkr PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY src
    LIBRARY_OUTPUT_DIRECTORY src
)

# stastics
make_paths_relative(REL_HEADER_FILES HEADER_FILES)
make_paths_relative(REL_SOURCE_FILES SOURCE_FILES)

print_info("[TRACE] Header files: ${REL_HEADER_FILES}" "95")
print_info("[TRACE] Source files: ${REL_SOURCE_FILES}" "96")

# Apps
enable_testing()
add_subdirectory(tests)
add_subdirectory(demos)
