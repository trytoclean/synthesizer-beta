cmake_minimum_required(VERSION 3.16)
project(server)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE)
  set(default_build_type "Debug")
  message(STATUS "Set the build type to `${default_build_type}` as none was specified.")
  set(CMAKE_BUILD_TYPE ${default_build_type} CACHE STRING "Chooce the build type." FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
  "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()
message(STATUS "myProject Build Type: ${CMAKE_BUILD_TYPE}")

file(GLOB SOURCES src/*.cc)
add_executable(
  server
  ${SOURCES}
)

target_include_directories(server PUBLIC include)
#Set the version for myProject
