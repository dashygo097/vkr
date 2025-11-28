cmake_minimum_required(VERSION 3.24)
project(demos LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

file(GLOB project ./*)

print_info("――――――――――――――――――APP SCOPE――――――――――――――――――" "0")

foreach(exec ${project})
  if(IS_DIRECTORY ${exec})
    set(EXECUTABLE_OUTPUT_PATH ../bin)
    math(EXPR index "${index} + 1" OUTPUT_FORMAT DECIMAL)
    get_filename_component(exec-name ${exec} NAME)
    file(GLOB_RECURSE ${exec}-src ${exec}/*.c ${exec}/*.cc ${exec}/*.cpp)
    file(GLOB_RECURSE ${exec}-hdr ${exec}/*.h ${exec}/*.hh ${exec}/*.hpp)

    add_executable(${exec-name} ${${exec}-src} ${${exec}-hdr})
    target_link_libraries(${exec-name} PRIVATE vkr)

    print_info("[TRACE] Adding: ${exec-name}" "0")

  endif()
endforeach()

print_info("[INFO] CMake configuration completed" "92")
print_info("[INFO] Total ${index} exec files will be generated" "92")
print_info("------------END OF CONFIGURATION-------------" "90")
