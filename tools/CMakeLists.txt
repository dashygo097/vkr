# tools

set(EXECUTABLE_OUTPUT_PATH ../bin)
add_executable(list_extensions list_extensions.cpp)
target_link_libraries(list_extensions PRIVATE Vulkan::Vulkan)

add_executable(check_physical_devices check_physical_devices.cpp)
target_link_libraries(check_physical_devices PRIVATE glfw Vulkan::Vulkan)

add_executable(list_validation_layers list_validation_layers.cpp)
target_link_libraries(list_validation_layers PRIVATE glfw Vulkan::Vulkan)

add_executable(check_device_extensions check_device_extensions.cpp)
target_link_libraries(check_device_extensions PRIVATE glfw Vulkan::Vulkan)

add_executable(check_queue_families check_queue_families.cpp)
target_link_libraries(check_queue_families PRIVATE glfw Vulkan::Vulkan)

add_executable(check_memory_heaps check_memory_heaps.cpp)
target_link_libraries(check_memory_heaps PRIVATE glfw Vulkan::Vulkan)

add_executable(check_features check_features.cpp)
target_link_libraries(check_features PRIVATE glfw Vulkan::Vulkan)

add_executable(check_limits check_limits.cpp)
target_link_libraries(check_limits PRIVATE glfw Vulkan::Vulkan)

add_executable(check_surface_support check_surface_support.cpp)
target_link_libraries(check_surface_support PRIVATE glfw Vulkan::Vulkan)

file(GLOB_RECURSE SOURCE_FILES *.c *.cc *.cpp)

# stastics
make_paths_relative(REL_SOURCE_FILES SOURCE_FILES)
print_info("  -- Tools : ${REL_SOURCE_FILES}\n" "93")
