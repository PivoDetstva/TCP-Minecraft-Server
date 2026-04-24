# CMake generated Testfile for 
# Source directory: /home/feetlover/C++/TCP
# Build directory: /home/feetlover/C++/TCP/build
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test([=[AllTests]=] "/home/feetlover/C++/TCP/build/tcpserver_tests")
set_tests_properties([=[AllTests]=] PROPERTIES  _BACKTRACE_TRIPLES "/home/feetlover/C++/TCP/CMakeLists.txt;40;add_test;/home/feetlover/C++/TCP/CMakeLists.txt;0;")
subdirs("external/googletest")
