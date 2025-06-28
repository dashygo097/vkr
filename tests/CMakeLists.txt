print_info("―――――――――――――――――TEST SCOPE――――――――――――――――――" "0")

add_executable(test_list_extensions test_list_extensions.cc)
target_link_libraries(test_list_extensions PRIVATE Vulkan::Vulkan)

file(GLOB_RECURSE SOURCE_FILES *.cpp *.c *.cc)

# stastics
print_info("[DEBUG] Test files: ${SOURCE_FILES}" "92")
