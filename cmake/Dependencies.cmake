# Find and configure all external dependencies

# Vulkan SDK
if (APPLE)
  find_package(Vulkan REQUIRED COMPONENTS MoltenVK)
  message(STATUS "Vulkan support for macOS enabled")

  # MoltenVK setup
  if (NOT Vulkan_MOLTENVK_LIBRARY)
    set(Vulkan_MOLTENVK_LIBRARY "${Vulkan_INCLUDE_DIRS}/../lib/libMoltenVK.dylib")
  endif()

  message(STATUS "MoltenVK library: ${Vulkan_MOLTENVK_LIBRARY}")

  set(HAS_VULKAN TRUE)
  set(HAS_MOLTENVK TRUE)

elseif(WIN32)
  find_package(Vulkan REQUIRED)
  message(STATUS "Vulkan support for Windows enabled")

  set(HAS_VULKAN TRUE)

elseif(UNIX)
  find_package(Vulkan REQUIRED)
  message(STATUS "Vulkan support for Linux enabled")

  set(HAS_VULKAN TRUE)

endif()


# 3rdparty
set(GLFW_BUILD_DOCS     OFF CACHE BOOL "Disable GLFW docs" FORCE)
set(GLFW_BUILD_TESTS    OFF CACHE BOOL "Disable GLFW tests" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "Disable GLFW examples" FORCE)

add_subdirectory(3rdparty/glfw EXCLUDE_FROM_ALL)
add_subdirectory(3rdparty/glm EXCLUDE_FROM_ALL)
add_subdirectory(3rdparty/imgui EXCLUDE_FROM_ALL)
add_subdirectory(3rdparty/stb EXCLUDE_FROM_ALL)
add_subdirectory(3rdparty/tinyobjloader EXCLUDE_FROM_ALL)

if (TARGET glfw)
  set(HAS_GLFW TRUE)
  get_target_property(GLFW_VERSION_STRING glfw VERSION)
endif()
