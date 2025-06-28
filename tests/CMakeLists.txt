print_info("―――――――――――――――――TEST SCOPE――――――――――――――――――" "0")

add_executable(list_extensions list_extensions.cc)
target_link_libraries(list_extensions PRIVATE Vulkan::Vulkan)
add_executable(check_physical_devices check_physical_devices.cc)
target_link_libraries(check_physical_devices PRIVATE glfw Vulkan::Vulkan)

file(GLOB_RECURSE SOURCE_FILES *.cpp *.c *.cc)

# stastics
print_info("[DEBUG] Test files: ${SOURCE_FILES}" "92")
