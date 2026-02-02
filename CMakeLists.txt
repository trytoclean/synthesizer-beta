cmake_minimum_required(VERSION 3.16)
project(server)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Boost REQUIRED COMPONENTS system)
find_package(GTest REQUIRED)
find_package(Threads REQUIRED)

add_library(
  tcp_lib
  src/echo_server.cc
)
target_include_directories(tcp_lib PUBLIC include)
target_link_libraries(tcp_lib PUBLIC Boost::boost)

add_executable(
  server
  src/main.cc
)
target_link_libraries(server PRIVATE tcp_lib)

enable_testing()
add_executable(
  tcp_tests
  test/echo_server_test.cc
)
target_link_libraries(tcp_tests PRIVATE tcp_lib GTest::gtest_main)
add_test(NAME tcp_tests COMMAND tcp_tests)
