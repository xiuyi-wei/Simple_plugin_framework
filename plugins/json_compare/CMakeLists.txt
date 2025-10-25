cmake_minimum_required(VERSION 3.16)
project(json_compare_plugin LANGUAGES CXX)

# 要求至少 C++17（nlohmann/json 也 OK）
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(json_compare SHARED
    JsonComparator.cpp
)

# 仅包含顶层 include（含 core/* 与 nlohmann/*）
# 你刚才说的“../../${CMAKE_SOURCE_DIR}/include”是不对的；
# 这里直接用 ${CMAKE_SOURCE_DIR}/include 即到达项目根的 include
target_include_directories(json_compare PRIVATE
    "${CMAKE_SOURCE_DIR}/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../include"
)

# MSVC 统一 UTF-8，避免 C4819
if (MSVC)
  target_compile_options(json_compare PRIVATE /utf-8)
endif()

# 产物输出到项目根/plugins；静态/导入库丢到 build/lib/<配置>
set_target_properties(json_compare PROPERTIES
  OUTPUT_NAME "json_compare"
  RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/plugins"   # Windows 的 .dll
  LIBRARY_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/plugins"   # Linux/mac 的 .so/.dylib
  ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/$<CONFIG>"
)

# 首次构建确保 plugins 目录存在
add_custom_command(TARGET json_compare POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E make_directory "${PROJECT_SOURCE_DIR}/plugins"
)
