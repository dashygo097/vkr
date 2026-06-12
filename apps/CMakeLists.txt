function(add_app_target APP_NAME APP_DIR)
  file(GLOB_RECURSE APP_SOURCES
    CONFIGURE_DEPENDS
    ${APP_DIR}/*.c
    ${APP_DIR}/*.cpp
  )

  file(GLOB_RECURSE APP_HEADERS
    CONFIGURE_DEPENDS
    ${APP_DIR}/*.h
    ${APP_DIR}/*.hpp
    ${APP_DIR}/*.hh
  )

  if(NOT APP_SOURCES)
    return()
  endif()

  add_executable(${APP_NAME} ${APP_SOURCES} ${APP_HEADERS})
  target_link_libraries(${APP_NAME} PRIVATE vkr)

  set(APP_OUTPUT_DIR ${CMAKE_BINARY_DIR}/bin/${APP_NAME})
  file(MAKE_DIRECTORY ${APP_OUTPUT_DIR})

  set_target_properties(${APP_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${APP_OUTPUT_DIR}
    VS_DEBUGGER_WORKING_DIRECTORY ${APP_OUTPUT_DIR}
  )

  foreach(CONFIG_TYPE ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER ${CONFIG_TYPE} CONFIG_TYPE_UPPER)
    set_target_properties(${APP_NAME} PROPERTIES
      RUNTIME_OUTPUT_DIRECTORY_${CONFIG_TYPE_UPPER} ${APP_OUTPUT_DIR}
    )
  endforeach()

  print_info("  -- Adding Executable: ${APP_NAME} -> ${APP_OUTPUT_DIR}\n" "92")

  math(EXPR APP_COUNT "${APP_COUNT} + 1")
  set(APP_COUNT ${APP_COUNT} PARENT_SCOPE)
endfunction()

set(APP_COUNT 0)

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR})
  file(GLOB APP_DIRS CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*)

  if(APP_DIRS)
    foreach(app_dir ${APP_DIRS})
      if(IS_DIRECTORY ${app_dir})
        get_filename_component(app_name ${app_dir} NAME)
        add_app_target(${app_name} ${app_dir})
      endif()
    endforeach()
  endif()
endif()

if(APP_COUNT GREATER 0)
  print_info("  ✓ Total executables: ${APP_COUNT}\n" "92")
  print_info("  ✓ Output directories:\n" "92")
  print_info("  -- Apps: ${CMAKE_BINARY_DIR}/bin/<app-name>\n" "90")
else()
  print_info("[WARN] No executables found\n" "93")
endif()

print_info("\n" "0")
