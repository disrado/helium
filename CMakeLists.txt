cmake_minimum_required(VERSION 4.3)

project(helium LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_subdirectory(engine)
add_subdirectory(game)
