cmake_minimum_required(VERSION 4.0)

project(libcatch2 LANGUAGES CXX)


set(SOURCES
    src/catch_amalgamated.cpp)

add_library(libcatch2 STATIC ${SOURCES})

target_include_directories(libcatch2 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
