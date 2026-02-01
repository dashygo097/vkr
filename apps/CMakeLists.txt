cmake_minimum_required(VERSION 3.24)
project(apps LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

function(add_app_target APP_NAME APP_DIR)
  # Collect all source files
  file(GLOB_RECURSE APP_SOURCES 
    ${APP_DIR}/*.c 
    ${APP_DIR}/*.cc 
    ${APP_DIR}/*.cpp
  )
  
  file(GLOB_RECURSE APP_HEADERS 
    ${APP_DIR}/*.h 
    ${APP_DIR}/*.hh 
    ${APP_DIR}/*.hpp 
  )
  
  # Skip if no sources
  if(NOT APP_SOURCES)
    return()
  endif()
  
  # Create executable
  add_executable(${APP_NAME} ${APP_SOURCES} ${APP_HEADERS})
  target_link_libraries(${APP_NAME} PRIVATE vkr)
  
  # Set output directory
  set_target_properties(${APP_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
  )
  
  print_info("  -- Adding Executable: ${APP_NAME}\n" "92")
  
  # Increment counter in parent scope
  math(EXPR APP_COUNT "${APP_COUNT} + 1")
  set(APP_COUNT ${APP_COUNT} PARENT_SCOPE)
endfunction()

# Build apps
set(APP_COUNT 0)
print_info("\n" "0")

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR})
  file(GLOB APP_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/*)
  
  if(APP_DIRS)
    foreach(app_dir ${APP_DIRS})
      if(IS_DIRECTORY ${app_dir})
        get_filename_component(app_name ${app_dir} NAME)
        add_app_target("${app_name}" ${app_dir})
        set(APP_COUNT ${APP_COUNT} PARENT_SCOPE)
      endif()
    endforeach()
  endif()
endif()

# Summary
if(APP_COUNT GREATER 0)
  print_info("  ✓ Total executables: ${APP_COUNT}\n" "92")
  print_info("  ✓ Output directories:\n" "92")
  print_info("  -- Apps: ${CMAKE_BINARY_DIR}/bin\n" "90")
else()
  print_info("[WARN] No executables found\n" "93")
endif()

print_info("\n" "0") 
