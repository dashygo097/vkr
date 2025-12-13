# stastics

if(HAS_VULKAN) 
  print_info("  ✓ Vulkan: ${Vulkan_INCLUDE_DIRS}\n" "90")
else()
  print_info("  ✗ Vulkan: disabled\n" "31")
endif()

if(HAS_MOLTENVK) 
  print_info("  ✓ MoltenVK: ${Vulkan_MOLTENVK_LIBRARY}\n" "90")
else()
  print_info("  ✗ MoltenVK: disabled\n" "31")
endif()

if(HAS_GLFW)
  print_info("  ✓ GLFW: ${GLFW_VERSION_STRING}\n" "90")
else()
  print_info("  ✗ GLFW: disabled\n" "31")
endif()

make_paths_relative(REL_HEADER_FILES VKR_HEADERS)
make_paths_relative(REL_SOURCE_FILES VKR_SOURCES)

make_preview_string(REL_HEADER_FILES 3)
print_info("  -- Header files: ${PREVIOUS_SCOPE_VAR}\n" "94")
make_preview_string(REL_SOURCE_FILES 3)
print_info("  -- Source files: ${PREVIOUS_SCOPE_VAR}\n" "96")
