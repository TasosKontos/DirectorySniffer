cmake_minimum_required(VERSION 3.22)
project(first)

set(CMAKE_CXX_STANDARD 14)

add_executable(first
        manager.cpp)
add_executable( second worker.cpp)
