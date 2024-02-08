cmake_minimum_required(VERSION 3.1)
project(door_controller)

## Compile as C++17

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")

set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Boost 1.71 REQUIRED COMPONENTS log_setup log date_time)
include_directories(include)

file(GLOB_RECURSE sources "src/*.cpp")
add_library(door_controller ${sources})
target_link_libraries(door_controller -lboost_system ${Boost_LIBRARIES})
target_include_directories(door_controller PUBLIC .)