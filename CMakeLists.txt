cmake_minimum_required(VERSION 4.0)

project(helium LANGUAGES CXX)


set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)


add_subdirectory(deps)


set(HEADERS
	src/core/core_macro.hpp
	src/core/type_index.hpp
	src/core/type_name.hpp
	src/core/type_id.hpp)

set(SOURCES
	src/main.cpp)

add_executable(run ${HEADERS} ${SOURCES})


include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)
add_library(helium SHARED ${SOURCES})


set(TESTS
	src/core/test/type_index_test.cpp
	src/core/test/type_name_test.cpp
	src/core/test/type_id_test.cpp)

add_executable(test_suite ${TESTS})

target_link_libraries(test_suite PRIVATE helium)
target_link_libraries(test_suite PRIVATE libcatch2)
