cmake_minimum_required(VERSION 4.0)

project(helium LANGUAGES CXX)

set(CMAKE_CXX_FLAGS "-std=c++23")

add_executable(helium src/main.cpp)

