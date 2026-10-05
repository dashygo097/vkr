# vkr
add_library(vkr ${VKR_SOURCES} ${VKR_HEADERS})

# Package the existing UI fonts into the library, not each app's asset root.
foreach(vkr_ui_font IN ITEMS Karla Cousine)
  set(vkr_ui_font_path
    "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/imgui/misc/fonts/${vkr_ui_font}-Regular.ttf")
  set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
    "${vkr_ui_font_path}")
  file(READ "${vkr_ui_font_path}" VKR_UI_FONT_${vkr_ui_font} HEX)
  string(REGEX REPLACE "(..)" "0x\\1,"
    VKR_UI_FONT_${vkr_ui_font} "${VKR_UI_FONT_${vkr_ui_font}}")
endforeach()
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/cmake/ui_fonts.hh.in"
  "${CMAKE_CURRENT_BINARY_DIR}/generated/vkr_ui_fonts.hh" @ONLY)
target_include_directories(vkr PRIVATE
  "${CMAKE_CURRENT_BINARY_DIR}/generated")
set_property(TARGET vkr PROPERTY VKR_UI_FONT_LICENSE
  "${CMAKE_CURRENT_SOURCE_DIR}/licenses/ui-fonts.txt")

target_precompile_headers(vkr PRIVATE
  $<$<COMPILE_LANGUAGE:CXX>:${CMAKE_CURRENT_SOURCE_DIR}/include/vkr/pch.hh>
)

target_include_directories(vkr PUBLIC 
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)

# Vulkan
target_include_directories(vkr PUBLIC ${Vulkan_INCLUDE_DIRS})
target_link_libraries(vkr PUBLIC Vulkan::Vulkan Vulkan::shaderc_combined)

if(HAS_SLANG)
  target_include_directories(vkr PRIVATE ${SLANG_INCLUDE_DIR})
  target_link_libraries(vkr PUBLIC ${SLANG_LIBRARY})
  target_compile_definitions(vkr PUBLIC VKR_HAS_SLANG=1)
endif()

if (APPLE)

elseif(UNIX)
  target_link_libraries(vkr PUBLIC
    SPIRV-Tools-opt
    SPIRV-Tools
    glslang
    SPIRV
    MachineIndependent
    GenericCodeGen
  )

endif()

# 3rdparty libs to link statically
target_link_libraries(vkr PUBLIC glfw)
target_link_libraries(vkr PUBLIC glm::glm-header-only)
target_link_libraries(vkr PUBLIC tinyobjloader)
target_link_libraries(vkr PUBLIC stb)
target_link_libraries(vkr PUBLIC tomlplusplus::tomlplusplus)
target_link_libraries(vkr PUBLIC spdlog::spdlog)
target_link_libraries(vkr PUBLIC imgui)
target_link_libraries(vkr PUBLIC ImGuiColorTextEdit)

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
