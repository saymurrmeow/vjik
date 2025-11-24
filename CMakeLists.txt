cmake_minimum_required(VERSION 4.0.3)

project(vjik LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED yes)
set(CMAKE_EXPORT_COMPILE_COMMANDS yes)

if (CMAKE_EXPORT_COMPILE_COMMANDS)
  execute_process(
    COMMAND ${CMAKE_COMMAND} -E create_symlink
        ${CMAKE_BINARY_DIR}/compile_commands.json
        ${CMAKE_SOURCE_DIR}/compile_commands.json
)
endif()

add_library(vjnet INTERFACE)

target_include_directories(vjnet INTERFACE
  ${CMAKE_SOURCE_DIR}/lib/net/core
  ${CMAKE_SOURCE_DIR}/lib/net/stun
)

add_executable(vjik bin/vjik.cpp)
target_link_libraries(vjik PRIVATE vjnet)

if (BUILD_TESTS)
  add_subdirectory("tests")
endif()

