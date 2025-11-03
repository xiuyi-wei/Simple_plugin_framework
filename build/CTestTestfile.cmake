# CMake generated Testfile for 
# Source directory: G:/HKAVISION/Porject/Simple_plugin_framework
# Build directory: G:/HKAVISION/Porject/Simple_plugin_framework/build
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
if(CTEST_CONFIGURATION_TYPE MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
  add_test(all_tests "G:/HKAVISION/Porject/Simple_plugin_framework/build/Debug/tests.exe")
  set_tests_properties(all_tests PROPERTIES  _BACKTRACE_TRIPLES "G:/HKAVISION/Porject/Simple_plugin_framework/CMakeLists.txt;60;add_test;G:/HKAVISION/Porject/Simple_plugin_framework/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
  add_test(all_tests "G:/HKAVISION/Porject/Simple_plugin_framework/build/Release/tests.exe")
  set_tests_properties(all_tests PROPERTIES  _BACKTRACE_TRIPLES "G:/HKAVISION/Porject/Simple_plugin_framework/CMakeLists.txt;60;add_test;G:/HKAVISION/Porject/Simple_plugin_framework/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
  add_test(all_tests "G:/HKAVISION/Porject/Simple_plugin_framework/build/MinSizeRel/tests.exe")
  set_tests_properties(all_tests PROPERTIES  _BACKTRACE_TRIPLES "G:/HKAVISION/Porject/Simple_plugin_framework/CMakeLists.txt;60;add_test;G:/HKAVISION/Porject/Simple_plugin_framework/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
  add_test(all_tests "G:/HKAVISION/Porject/Simple_plugin_framework/build/RelWithDebInfo/tests.exe")
  set_tests_properties(all_tests PROPERTIES  _BACKTRACE_TRIPLES "G:/HKAVISION/Porject/Simple_plugin_framework/CMakeLists.txt;60;add_test;G:/HKAVISION/Porject/Simple_plugin_framework/CMakeLists.txt;0;")
else()
  add_test(all_tests NOT_AVAILABLE)
endif()
subdirs("_deps/googletest-build")
