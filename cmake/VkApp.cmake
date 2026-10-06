# Helpers for applications and examples that link against VKR.

function(_vk_app_print MSG)
  if(COMMAND print_info)
    print_info("${MSG}" "92")
  else()
    message(STATUS "${MSG}")
  endif()
endfunction()

function(add_vk_app TARGET_NAME)
  set(options)
  set(one_value_args
    ASSET_DIR
    OUTPUT_DIR
    OUTPUT_NAME
    WORKING_DIRECTORY
  )
  set(multi_value_args
    SOURCES
    HEADERS
    LIBRARIES
    INCLUDE_DIRS
    COMPILE_DEFINITIONS
  )

  cmake_parse_arguments(VK_APP
    "${options}"
    "${one_value_args}"
    "${multi_value_args}"
    ${ARGN}
  )

  if(VK_APP_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR
      "add_vk_app(${TARGET_NAME}) received unknown arguments: "
      "${VK_APP_UNPARSED_ARGUMENTS}"
    )
  endif()

  if(NOT VK_APP_SOURCES)
    file(GLOB_RECURSE VK_APP_SOURCES
      CONFIGURE_DEPENDS
      "${CMAKE_CURRENT_SOURCE_DIR}/*.c"
      "${CMAKE_CURRENT_SOURCE_DIR}/*.cc"
      "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
      "${CMAKE_CURRENT_SOURCE_DIR}/*.cxx"
    )
  endif()

  if(NOT VK_APP_HEADERS)
    file(GLOB_RECURSE VK_APP_HEADERS
      CONFIGURE_DEPENDS
      "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
      "${CMAKE_CURRENT_SOURCE_DIR}/*.hh"
      "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp"
      "${CMAKE_CURRENT_SOURCE_DIR}/*.hxx"
    )
  endif()

  if(NOT VK_APP_SOURCES)
    message(FATAL_ERROR "add_vk_app(${TARGET_NAME}) requires at least one source file")
  endif()

  add_executable(${TARGET_NAME} ${VK_APP_SOURCES} ${VK_APP_HEADERS})
  target_link_libraries(${TARGET_NAME} PRIVATE vkr ${VK_APP_LIBRARIES})

  if(VK_APP_INCLUDE_DIRS)
    target_include_directories(${TARGET_NAME} PRIVATE ${VK_APP_INCLUDE_DIRS})
  endif()

  if(VK_APP_COMPILE_DEFINITIONS)
    target_compile_definitions(${TARGET_NAME} PRIVATE ${VK_APP_COMPILE_DEFINITIONS})
  endif()

  if(VK_APP_OUTPUT_NAME)
    set_target_properties(${TARGET_NAME} PROPERTIES
      OUTPUT_NAME ${VK_APP_OUTPUT_NAME}
    )
  endif()

  if(VK_APP_OUTPUT_DIR AND IS_ABSOLUTE "${VK_APP_OUTPUT_DIR}")
    set(app_output_dir "${VK_APP_OUTPUT_DIR}")
  elseif(VK_APP_OUTPUT_DIR)
    set(app_output_dir "${CMAKE_BINARY_DIR}/${VK_APP_OUTPUT_DIR}")
  else()
    set(app_output_dir "${CMAKE_BINARY_DIR}/bin/${TARGET_NAME}")
  endif()

  if(VK_APP_WORKING_DIRECTORY AND IS_ABSOLUTE "${VK_APP_WORKING_DIRECTORY}")
    set(app_working_dir "${VK_APP_WORKING_DIRECTORY}")
  elseif(VK_APP_WORKING_DIRECTORY)
    set(app_working_dir "${CMAKE_BINARY_DIR}/${VK_APP_WORKING_DIRECTORY}")
  else()
    set(app_working_dir "${app_output_dir}")
  endif()

  set_target_properties(${TARGET_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${app_output_dir}"
    VS_DEBUGGER_WORKING_DIRECTORY "${app_working_dir}"
  )

  foreach(config_type ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER "${config_type}" config_type_upper)
    set_target_properties(${TARGET_NAME} PROPERTIES
      RUNTIME_OUTPUT_DIRECTORY_${config_type_upper} "${app_output_dir}"
    )
  endforeach()

  set(app_asset_dir_explicit FALSE)
  if(VK_APP_ASSET_DIR)
    set(app_asset_dir_explicit TRUE)
    if(IS_ABSOLUTE "${VK_APP_ASSET_DIR}")
      set(app_asset_dir "${VK_APP_ASSET_DIR}")
    else()
      set(app_asset_dir "${CMAKE_CURRENT_SOURCE_DIR}/${VK_APP_ASSET_DIR}")
    endif()
  else()
    set(app_asset_dir "${CMAKE_CURRENT_SOURCE_DIR}/assets")
  endif()

  if(EXISTS "${app_asset_dir}")
    add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${app_asset_dir}"
        "$<TARGET_FILE_DIR:${TARGET_NAME}>/assets"
      COMMENT "Copying assets for ${TARGET_NAME}"
      VERBATIM
    )
  elseif(app_asset_dir_explicit)
    message(WARNING
      "add_vk_app(${TARGET_NAME}) asset directory does not exist: "
      "${app_asset_dir}"
    )
  endif()

  _vk_app_print("  -- Adding VK app: ${TARGET_NAME} -> ${app_output_dir}\n")
endfunction()
