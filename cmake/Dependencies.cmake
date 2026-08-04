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

if(VKR_ENABLE_SLANG)
  get_filename_component(VKR_VULKAN_LIBRARY_DIR "${Vulkan_LIBRARY}" DIRECTORY)

  find_path(VKR_SLANG_INCLUDE_DIR
    NAMES slang/slang.h
    HINTS "${Vulkan_INCLUDE_DIR}"
    NO_DEFAULT_PATH
  )
  find_library(VKR_SLANG_LIBRARY
    NAMES slang libslang
    HINTS "${VKR_VULKAN_LIBRARY_DIR}"
    NO_DEFAULT_PATH
  )

  if(VKR_SLANG_INCLUDE_DIR AND VKR_SLANG_LIBRARY)
    set(SLANG_INCLUDE_DIR "${VKR_SLANG_INCLUDE_DIR}" CACHE PATH
      "Slang include directory" FORCE)
    set(SLANG_LIBRARY "${VKR_SLANG_LIBRARY}" CACHE FILEPATH
      "Slang library" FORCE)
  else()
    find_path(SLANG_INCLUDE_DIR
      NAMES slang/slang.h
    )
    find_library(SLANG_LIBRARY
      NAMES slang libslang
    )
  endif()

  if(SLANG_INCLUDE_DIR AND SLANG_LIBRARY)
    set(HAS_SLANG TRUE)
    message(STATUS "Slang support enabled: ${SLANG_LIBRARY}")
  else()
    set(HAS_SLANG FALSE)
    message(STATUS "Slang support disabled: Slang headers/library not found")
  endif()
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

if (TARGET glfw)
  set(HAS_GLFW TRUE)
  get_target_property(GLFW_VERSION_STRING glfw VERSION)
endif()

if(NOT DEFINED ENGINE_ASSETS_DIR)
  set(ENGINE_ASSETS_DIR "${CMAKE_SOURCE_DIR}/")
endif()
message(STATUS "Default Assets directory: ${ENGINE_ASSETS_DIR}")
add_compile_definitions(-DENGINE_ASSETS_DIR="${ENGINE_ASSETS_DIR}")
