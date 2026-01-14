cmake_minimum_required(VERSION 4.0)

project(dependencies LANGUAGES CXX)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")

add_subdirectory(catch2)

add_subdirectory(SDL EXCLUDE_FROM_ALL)
