add_library(simple SHARED SimplePlugin.cpp)

# Use target include directories (private to the plugin)
target_include_directories(simple PRIVATE ${PROJECT_SOURCE_DIR}/include)

# Place plugin runtime (shared library) into ${CMAKE_BINARY_DIR}/plugins so the
# application can load it at runtime using a predictable relative path.
set_target_properties(simple PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<CONFIG>"
	LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<CONFIG>"
	ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<CONFIG>"
)
