add_library(simple SHARED SimplePlugin.cpp)

# Use target include directories (private to the plugin)
target_include_directories(simple PRIVATE ${CMAKE_SOURCE_DIR}/include)

# Place plugin runtime (shared library) into project-root/plugins
set_target_properties(simple PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/plugins"
	LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/plugins"
	ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/$<CONFIG>"
)
