cmake_minimum_required(VERSION 4.0)

project(helium LANGUAGES CXX)


set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(magic_enum CONFIG REQUIRED)
find_package(SDL3 CONFIG REQUIRED)
find_package(Catch2 CONFIG REQUIRED)

set(HEADERS
    src/core/core_macro.hpp
    src/core/type_index.hpp
    src/core/type_name.hpp
    src/core/type_id.hpp
    src/utils/logger/logger.hpp)

set(SOURCES
    src/utils/logger/logger.cpp)

add_executable(run ${HEADERS} src/main.cpp)
target_link_libraries(run PRIVATE SDL3::SDL3)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)
add_library(helium STATIC ${SOURCES})
target_link_libraries(helium PRIVATE magic_enum::magic_enum)

set(TESTS
    src/core/test/type_index_test.cpp
    src/core/test/type_name_test.cpp
    src/core/test/type_id_test.cpp
    src/utils/containers/test/lock_free_queue_test.cpp)

add_executable(test_suite ${TESTS})

target_link_libraries(test_suite PRIVATE helium)
target_link_libraries(test_suite PRIVATE Catch2::Catch2WithMain)
