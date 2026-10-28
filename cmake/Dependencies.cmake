# Find and configure all external dependencies

# Vulkan SDK
if (APPLE)
  find_package(Vulkan REQUIRED COMPONENTS MoltenVK shaderc_combined)
  message(STATUS "Vulkan support for macOS enabled")

  # MoltenVK setup
  if (NOT Vulkan_MOLTENVK_LIBRARY)
    set(Vulkan_MOLTENVK_LIBRARY "${Vulkan_INCLUDE_DIRS}/../lib/libMoltenVK.dylib")
  endif()

  message(STATUS "MoltenVK library: ${Vulkan_MOLTENVK_LIBRARY}")

  set(HAS_VULKAN TRUE)
  set(HAS_MOLTENVK TRUE)

elseif(UNIX)
  find_package(Vulkan REQUIRED COMPONENTS shaderc_combined)
  message(STATUS "Vulkan support for Linux enabled")

  set(HAS_VULKAN TRUE)

elseif(WIN32)
  find_package(Vulkan REQUIRED COMPONENTS shaderc_combined)
  message(STATUS "Vulkan support for Windows enabled")

  set(HAS_VULKAN TRUE)

endif()

set(HAS_SLANG FALSE)
if(ENABLE_SLANG)
  get_filename_component(vulkan_library_dir "${Vulkan_LIBRARY}" DIRECTORY)
  find_package(slang CONFIG QUIET
    HINTS "${vulkan_library_dir}/cmake/slang"
  )
  if(TARGET slang::slang)
    set(HAS_SLANG TRUE)
    message(STATUS "Slang support enabled")
  else()
    message(STATUS "Slang support disabled: Slang package not found")
  endif()
endif()

if(UNIX AND NOT APPLE)
  find_package(SPIRV-Tools CONFIG REQUIRED)
  find_package(SPIRV-Tools-opt CONFIG REQUIRED)
  find_package(glslang CONFIG REQUIRED)
endif()

# 3rdparty
set(GLFW_BUILD_DOCS     OFF CACHE BOOL "Disable GLFW docs" FORCE)
set(GLFW_BUILD_TESTS    OFF CACHE BOOL "Disable GLFW tests" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "Disable GLFW examples" FORCE)

add_subdirectory(3rdparty/glfw EXCLUDE_FROM_ALL)
add_subdirectory(3rdparty/glm EXCLUDE_FROM_ALL)
add_subdirectory(3rdparty/imgui EXCLUDE_FROM_ALL)
add_subdirectory(3rdparty/ImGuiColorTextEdit EXCLUDE_FROM_ALL)
add_subdirectory(3rdparty/stb EXCLUDE_FROM_ALL)
add_subdirectory(3rdparty/tinyobjloader EXCLUDE_FROM_ALL)
add_subdirectory(3rdparty/spdlog EXCLUDE_FROM_ALL)
add_subdirectory(3rdparty/tomlplusplus EXCLUDE_FROM_ALL)
add_subdirectory(3rdparty/ImGuizmo EXCLUDE_FROM_ALL)

# Keep bundled headers relocatable without modifying the submodules.
set_property(TARGET imgui PROPERTY INTERFACE_INCLUDE_DIRECTORIES
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/imgui>"
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/imgui/backends>"
  "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/vkr/thirdparty/imgui>"
  "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/vkr/thirdparty/imgui/backends>"
)
set_property(TARGET ImGuiColorTextEdit PROPERTY INTERFACE_INCLUDE_DIRECTORIES
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/ImGuiColorTextEdit>"
  "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/vkr/thirdparty/ImGuiColorTextEdit>"
)
set_property(TARGET stb PROPERTY INTERFACE_INCLUDE_DIRECTORIES
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/stb>"
  "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/vkr/thirdparty/stb>"
)
set_property(TARGET imguizmo PROPERTY INTERFACE_INCLUDE_DIRECTORIES
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/ImGuizmo/src>"
  "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/vkr/thirdparty/ImGuizmo>"
)
target_include_directories(tomlplusplus_tomlplusplus INTERFACE
  "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
)
target_link_libraries(ImGuiColorTextEdit PUBLIC imgui)
target_link_libraries(imguizmo PUBLIC imgui)

if (TARGET glfw)
  set(HAS_GLFW TRUE)
  get_target_property(GLFW_VERSION_STRING glfw VERSION)
endif()
