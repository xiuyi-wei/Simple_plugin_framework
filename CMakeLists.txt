cmake_minimum_required(VERSION 3.10)
project(SimplePluginFramework LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 只编 app（不再 add_subdirectory plugins/*）
add_executable(app
  src/app/main.cpp
  src/config/ConfigParser.cpp
  src/ui/console_ui.cpp
  src/runtime/Services.cpp
  src/workflow/Executor.cpp
  src/workflow/WorkflowParser.cpp
)

target_include_directories(app PRIVATE
  ${CMAKE_SOURCE_DIR}/include
)

# MSVC 用 UTF-8，避免 C4819
if (MSVC)
  target_compile_options(app PRIVATE /utf-8)
endif()

# 可执行文件放到“项目根”，并让 VS 调试时以项目根为工作目录
set_target_properties(app PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY          "${PROJECT_SOURCE_DIR}"
  VS_DEBUGGER_WORKING_DIRECTORY     "${PROJECT_SOURCE_DIR}"
)

# 仅 Linux 需要 -ldl；macOS 不需要
if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
  target_link_libraries(app PRIVATE dl)
endif()

# Optionally build bundled plugins
if (EXISTS "${CMAKE_SOURCE_DIR}/plugins/json_compare/CMakeLists.txt")
  add_subdirectory(plugins/json_compare)
endif()
if (EXISTS "${CMAKE_SOURCE_DIR}/plugins/simple/CMakeLists.txt")
  add_subdirectory(plugins/simple)
endif()
