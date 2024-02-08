cmake_minimum_required(VERSION 3.1)
project(lift_controller)

## Compile as C++17

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")

set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Threads REQUIRED)
find_package(aws-crt-cpp REQUIRED)
find_package(nlohmann_json 3.2.0 REQUIRED)
find_package(Boost 1.71 REQUIRED COMPONENTS log_setup log date_time)
find_library(yaml-cpp REQUIRED)

add_subdirectory(lib/ADS/AdsLib)
include_directories(include)

file(GLOB_RECURSE sources src/*.cpp)
add_library(lift_controller ${sources})
target_include_directories(lift_controller PUBLIC .)
target_link_libraries(lift_controller yaml-cpp ads -lpthread -lboost_system ${Boost_LIBRARIES} AWS::aws-crt-cpp)