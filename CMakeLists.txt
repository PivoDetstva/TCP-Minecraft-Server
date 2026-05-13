cmake_minimum_required(VERSION 3.22)
project(MinecraftServer)

set(CMAKE_CXX_STANDARD 23)     
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(MinecraftServer     
    src/main.cpp
    src/game/player.cpp
    src/game/world.cpp
    src/network/server.cpp
    src/network/tcpclient.cpp
    src/protocol/packet.cpp
    src/protocol/varint.cpp
    src/protocol/helper.cpp
    src/world/RegionFile.cpp
    src/world/NBT.cpp
)

target_include_directories(MinecraftServer PRIVATE
    ${CMAKE_SOURCE_DIR}/src/include   
     ${CMAKE_SOURCE_DIR}/src/protocol
     ${CMAKE_SOURCE_DIR}/src/network
     ${CMAKE_SOURCE_DIR}/src/world
)

target_link_libraries(MinecraftServer z)

enable_testing()
add_subdirectory(external/googletest)

add_executable(tcpserver_tests
tests/test_varint.cpp
tests/test_packet.cpp
src/protocol/varint.cpp
src/protocol/packet.cpp
)
target_include_directories(tcpserver_tests PRIVATE 
    ${CMAKE_SOURCE_DIR}/src/include
    ${CMAKE_SOURCE_DIR}/src/protocol
    ${CMAKE_SOURCE_DIR}/tests

)

target_link_libraries(tcpserver_tests PRIVATE gtest_main gtest)

add_test(NAME AllTests COMMAND tcpserver_tests)