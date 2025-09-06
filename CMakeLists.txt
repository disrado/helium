cmake_minimum_required(VERSION 4.0)

project(helium LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(HEADERS
	src/core/core_macro.hpp
	src/core/type_id.hpp
	src/core/type_name.hpp)

set(SOURCES
	src/main.cpp
	)

add_executable(helium ${HEADERS} ${SOURCES})
