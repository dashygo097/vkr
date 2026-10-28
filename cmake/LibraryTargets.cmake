# vkr
add_library(vkr ${VKR_SOURCES} ${VKR_HEADERS})
add_library(vkr::vkr ALIAS vkr)
target_compile_features(vkr PUBLIC cxx_std_17)

target_precompile_headers(vkr PRIVATE
  $<$<COMPILE_LANGUAGE:CXX>:${CMAKE_CURRENT_SOURCE_DIR}/include/vkr/pch.hh>
)

target_include_directories(vkr PUBLIC 
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

# Vulkan
target_link_libraries(vkr PUBLIC Vulkan::Vulkan Vulkan::shaderc_combined)

if(HAS_SLANG)
  target_link_libraries(vkr PUBLIC slang::slang)
  target_compile_definitions(vkr PUBLIC VKR_HAS_SLANG=1)
endif()

if(UNIX AND NOT APPLE)
  target_link_libraries(vkr PUBLIC
    SPIRV-Tools-opt
    SPIRV-Tools-static
    glslang::glslang
    glslang::SPIRV
  )

endif()

# Bundled dependencies
target_link_libraries(vkr PUBLIC glfw)
target_link_libraries(vkr PUBLIC glm::glm-header-only)
target_link_libraries(vkr PUBLIC tinyobjloader)
target_link_libraries(vkr PUBLIC stb)
target_link_libraries(vkr PUBLIC tomlplusplus::tomlplusplus)
target_link_libraries(vkr PUBLIC spdlog::spdlog)
target_link_libraries(vkr PUBLIC imgui)
target_link_libraries(vkr PUBLIC ImGuiColorTextEdit)
target_link_libraries(vkr PUBLIC imguizmo)

if (APPLE) 
  target_link_libraries(vkr PUBLIC
    "-framework Metal"
    "-framework Foundation"
    "-framework IOSurface"
  )
endif()

set_target_properties(vkr PROPERTIES
  ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/lib"
  LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/lib"
)

# Export the dependency closure so consumers only link vkr::vkr.
install(TARGETS
  vkr glfw glm-header-only tinyobjloader stb tomlplusplus_tomlplusplus
  spdlog imgui ImGuiColorTextEdit imguizmo
  EXPORT vkrTargets
  ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
  LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
  RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
)

install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include/"
  DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)
install(DIRECTORY
  "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/glfw/include/GLFW"
  "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/glm/glm"
  "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/spdlog/include/spdlog"
  "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/tomlplusplus/include/toml++"
  DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
  FILES_MATCHING
    PATTERN "*.h" PATTERN "*.hpp" PATTERN "*.inl"
    PATTERN "*.ipp"
)
if(SPDLOG_GENERATE_TWEAKME_H)
  install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/3rdparty/spdlog/include/spdlog/tweakme.h"
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/spdlog"
  )
endif()
install(FILES
  "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/tinyobjloader/tiny_obj_loader.h"
  DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)
foreach(dependency imgui ImGuiColorTextEdit stb)
  file(GLOB dependency_headers CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/${dependency}/*.h"
  )
  install(FILES ${dependency_headers}
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/vkr/thirdparty/${dependency}"
  )
endforeach()
install(FILES
  "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/imgui/backends/imgui_impl_glfw.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/imgui/backends/imgui_impl_vulkan.h"
  DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/vkr/thirdparty/imgui/backends"
)
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/ImGuizmo/src/"
  DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/vkr/thirdparty/ImGuizmo"
  FILES_MATCHING PATTERN "*.h"
)

foreach(dependency glfw glm imgui ImGuiColorTextEdit ImGuizmo stb
                   tinyobjloader spdlog tomlplusplus)
  file(GLOB dependency_licenses
    "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/${dependency}/LICENSE*"
    "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/${dependency}/copying.txt"
  )
  install(FILES ${dependency_licenses}
    DESTINATION "${CMAKE_INSTALL_DATADIR}/licenses/vkr/${dependency}"
  )
endforeach()
install(FILES
  "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/spdlog/include/spdlog/fmt/bundled/fmt.license.rst"
  DESTINATION "${CMAKE_INSTALL_DATADIR}/licenses/vkr/spdlog"
)

include(CMakePackageConfigHelpers)
configure_package_config_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/vkrConfig.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/vkrConfig.cmake"
  INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/vkr"
)
write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/vkrConfigVersion.cmake"
  VERSION "${PROJECT_VERSION}"
  COMPATIBILITY ExactVersion
)
install(EXPORT vkrTargets
  FILE vkrTargets.cmake
  NAMESPACE vkr::
  DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/vkr"
)
install(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/vkrConfig.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/vkrConfigVersion.cmake"
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/config.cmake"
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/VkApp.cmake"
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompilerSetup.cmake"
  DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/vkr"
)
