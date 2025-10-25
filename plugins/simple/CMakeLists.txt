add_library(simple SHARED SimplePlugin.cpp)
set_target_properties(simple PROPERTIES PREFIX "lib" SUFFIX ".so")
target_include_directories(simple PRIVATE ${PROJECT_SOURCE_DIR}/include)
