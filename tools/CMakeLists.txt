# tools

set(EXECUTABLE_OUTPUT_PATH ../bin)
add_executable(list_extensions list_extensions.cpp)
target_link_libraries(list_extensions PRIVATE Vulkan::Vulkan)

add_executable(check_physical_devices check_physical_devices.cpp)
target_link_libraries(check_physical_devices PRIVATE glfw Vulkan::Vulkan)

add_executable(list_validation_layers list_validation_layers.cpp)
target_link_libraries(list_validation_layers PRIVATE glfw Vulkan::Vulkan)

file(GLOB_RECURSE SOURCE_FILES *.c *.cc *.cpp)

# stastics
make_paths_relative(REL_SOURCE_FILES SOURCE_FILES)
print_info("  -- Tools : ${REL_SOURCE_FILES}\n" "93")
