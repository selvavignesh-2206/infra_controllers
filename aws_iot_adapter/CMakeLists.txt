cmake_minimum_required(VERSION 3.1)
project(aws_iot_adapter)

## Compile as C++17

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")

set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Threads REQUIRED)
find_package(aws-crt-cpp REQUIRED)
find_package(nlohmann_json 3.2.0 REQUIRED)
find_package(Boost 1.71 REQUIRED COMPONENTS log_setup log date_time)
find_library(yaml-cpp REQUIRED)
set(CMAKE_BUILD_TYPE debug)

add_subdirectory(../lift_controller lift_controller)
add_subdirectory(../door_controller door_controller)
include_directories(../lift_controller/include ../door_controller/include include)

### lift adapter
file(GLOB_RECURSE sources src/*.cpp)
add_executable(${PROJECT_NAME} ${sources})
add_dependencies(${PROJECT_NAME}  ads lift_controller door_controller)
target_link_libraries(${PROJECT_NAME} door_controller yaml-cpp lift_controller ads -lpthread -lboost_system ${Boost_LIBRARIES} AWS::aws-crt-cpp)