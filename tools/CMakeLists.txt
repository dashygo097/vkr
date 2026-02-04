cmake_minimum_required(VERSION 3.24)
project(tools LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

function(add_tool_target TOOL_NAME TOOL_DIR)
  # Collect all source files
  file(GLOB_RECURSE TOOL_SOURCES 
    ${TOOL_DIR}/*.c 
    ${TOOL_DIR}/*.cpp
  )
  
  file(GLOB_RECURSE TOOL_HEADERS 
    ${TOOL_DIR}/*.h 
    ${TOOL_DIR}/*.hpp 
  )
  
  # Skip if no sources
  if(NOT TOOL_SOURCES)
    return()
  endif()
  
  # Create executable
  add_executable(${TOOL_NAME} ${TOOL_SOURCES} ${TOOL_HEADERS})
  target_link_libraries(${TOOL_NAME} PRIVATE vkr)
  
  # Set output directory
  set_target_properties(${TOOL_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
  )
  
  print_info("  -- Adding Executable: ${TOOL_NAME}\n" "92")
  
  # Increment counter in parent scope
  math(EXPR TOOL_COUNT "${TOOL_COUNT} + 1")
  set(TOOL_COUNT ${TOOL_COUNT} PARENT_SCOPE)
endfunction()

# Build tools 
set(TOOL_COUNT 0)

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR})
  file(GLOB TOOL_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/*)
  
  if(TOOL_DIRS)
    foreach(tool_dir ${TOOL_DIRS})
      if(IS_DIRECTORY ${tool_dir})
        get_filename_component(tool_name ${tool_dir} NAME)
        add_tool_target("${tool_name}" ${tool_dir})
        set(TOOL_COUNT ${TOOL_COUNT} PARENT_SCOPE)
      endif()
    endforeach()
  endif()
endif()

# Summary
if(TOOL_COUNT GREATER 0)
  print_info("  ✓ Total executables: ${TOOL_COUNT}\n" "92")
  print_info("  ✓ Output directories:\n" "92")
  print_info("  -- Apps: ${CMAKE_BINARY_DIR}/bin\n" "90")
else()
  print_info("[WARN] No executables found\n" "93")
endif()

print_info("\n" "0") 
