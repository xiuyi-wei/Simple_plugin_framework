cmake_minimum_required(VERSION 3.16)
project(json_compare_plugin LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(json_compare SHARED
    JsonComparator.cpp
)

target_include_directories(json_compare PRIVATE
    "${CMAKE_SOURCE_DIR}/include"
)

if (MSVC)
  target_compile_options(json_compare PRIVATE /utf-8)
endif()

set_target_properties(json_compare PROPERTIES
  OUTPUT_NAME "json_compare"
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/plugins"
  LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/plugins"
  ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/$<CONFIG>"
)

add_custom_command(TARGET json_compare POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_SOURCE_DIR}/plugins"
)

