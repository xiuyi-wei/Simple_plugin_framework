cmake_minimum_required(VERSION 3.10)
project(plugin_framework LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(${PROJECT_SOURCE_DIR}/include)

add_subdirectory(plugins/simple)

add_executable(main src/app/main.cpp)
target_link_libraries(main dl)
