cmake_minimum_required(VERSION 3.10)
project(SimplePluginFramework LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add plugin subdirectory
add_subdirectory(plugins/simple)

# Application
add_executable(app src/app/main.cpp)
target_include_directories(app PRIVATE ${CMAKE_SOURCE_DIR}/include)

# Put the app binary and plugins into predictable folders under the build tree
set_target_properties(app PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/bin/$<CONFIG>"
	LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/bin/$<CONFIG>"
	ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/bin/$<CONFIG>"
)


# On Unix we need to link to dl for dynamic loading; on Windows it's not needed.
if(UNIX)
	target_link_libraries(app PRIVATE dl)
endif()
