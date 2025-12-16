message(STATUS "Adding nlohmann/json library...")

add_library(nlohmann_json INTERFACE)

target_include_directories(nlohmann_json INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  $<INSTALL_INTERFACE:include>
)

add_library(nlohmann_json::nlohmann_json ALIAS nlohmann_json)

set_target_properties(nlohmann_json PROPERTIES
  INTERFACE_COMPILE_FEATURES cxx_std_11
)
message(STATUS "nlohmann/json include: ${CMAKE_CURRENT_SOURCE_DIR}")
