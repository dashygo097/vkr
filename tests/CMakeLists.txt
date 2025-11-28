print_info("―――――――――――――――――TEST SCOPE――――――――――――――――――" "0")

set(EXECUTABLE_OUTPUT_PATH ../bin)
add_executable(list_extensions list_extensions.cc)
target_link_libraries(list_extensions PRIVATE Vulkan::Vulkan)
add_test(NAME list_extensions_test COMMAND list_extensions)
add_executable(check_physical_devices check_physical_devices.cc)
target_link_libraries(check_physical_devices PRIVATE glfw Vulkan::Vulkan)
add_test(NAME check_physical_devices_test COMMAND check_physical_devices)
add_executable(list_validation_layers list_validation_layers.cc)
target_link_libraries(list_validation_layers PRIVATE glfw Vulkan::Vulkan)
add_test(NAME list_validation_layers_test COMMAND list_validation_layers)

file(GLOB_RECURSE SOURCE_FILES *.cpp *.c *.cc)

# stastics

make_paths_relative(REL_SOURCE_FILES SOURCE_FILES)
print_info("[DEBUG] Test files: ${REL_SOURCE_FILES}" "93")
