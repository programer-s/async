cmake_minimum_required(VERSION 3.14)
project(examples)

set (CMAKE_CXX_STANDARD 23)
set (CMAKE_C_STANDARD 23)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/examples/scheduler)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/examples/coroutine)

