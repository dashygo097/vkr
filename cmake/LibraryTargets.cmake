# vkr
add_library(vkr ${VKR_SOURCES} ${VKR_HEADERS})

target_include_directories(vkr PUBLIC 
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)

target_precompile_headers(vkr PRIVATE include/vkr/pch.hh)
# 3rdparty libs to link statically
target_link_libraries(vkr PUBLIC glfw Vulkan::Vulkan glm::glm-header-only imgui)
target_link_libraries(vkr PRIVATE stb tinyobjloader)

if (APPLE) 
  target_link_libraries(vkr PUBLIC
    "-framework Metal"
    "-framework Foundation"
    "-framework IOSurface"
  )
endif()

set_target_properties(vkr PROPERTIES
  ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
  LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
)
