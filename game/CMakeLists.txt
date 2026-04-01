cmake_minimum_required(VERSION 4.3)

project(game LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(magic_enum CONFIG REQUIRED)
find_package(SDL3 CONFIG REQUIRED)
find_package(Catch2 CONFIG REQUIRED)

set(game_headers
)

set(game_sources
    ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
)

add_library(game STATIC ${game_sources})
target_link_libraries(game PUBLIC magic_enum::magic_enum SDL3::SDL3 helium)
target_include_directories(game PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/)

add_executable(game_app main.cpp)
target_link_libraries(game_app PRIVATE game)


project(game_test LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(test_sources
    test/test.cpp
)

add_executable(game_test_suite ${test_sources})
target_link_libraries(game_test_suite PUBLIC game Catch2::Catch2WithMain)
