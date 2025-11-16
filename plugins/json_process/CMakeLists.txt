cmake_minimum_required(VERSION 3.16)
project(json_process_plugin LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(json_process SHARED
    JsonProcess.cpp
)

target_include_directories(json_process PRIVATE
    "${CMAKE_SOURCE_DIR}/include"
)

if (MSVC)
  target_compile_options(json_process PRIVATE /utf-8)
endif()

set_target_properties(json_process PROPERTIES
  OUTPUT_NAME "json_process"
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/plugins"
  LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/plugins"
  ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/$<CONFIG>"
)

add_custom_command(TARGET json_process POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_SOURCE_DIR}/plugins"
)

