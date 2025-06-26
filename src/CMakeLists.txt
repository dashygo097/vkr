cmake_minimum_required(VERSION 3.24)
project(macos-vulkan-renderer LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

file(GLOB sample_projects ./basic/*)
file(GLOB_RECURSE header_files ../include/*)

set(index 0)
foreach(header ${header_files})
  get_filename_component(header-name ${header} NAME)
  print_info("[TRACE] Include: ${header-name}" "95")
endforeach()

foreach(project ${sample_projects})
  if(IS_DIRECTORY ${project})
    set(EXECUTABLE_OUTPUT_PATH ../output)
    math(EXPR index "${index} + 1" OUTPUT_FORMAT DECIMAL)
    get_filename_component(project-name ${project} NAME)
    file(GLOB ${project}-src ${project}/*.cpp ${project}/*.c ${project}/*.cc)

    add_executable(${project-name} ${${project}-src})
    target_include_directories(${project-name} PUBLIC ../include)

    target_link_libraries(${project-name} PRIVATE glfw Vulkan::Vulkan Vulkan::MoltenVK)

    print_info("[TRACE] Adding: ${project-name}" "0")

  endif()
endforeach()

print_info("[TRACE] CMake configuration completed" "92")
print_info("[TRACE] Total ${index} exec files will be generated" "92")
