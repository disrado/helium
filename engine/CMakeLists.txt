cmake_minimum_required(VERSION 4.3)

project(helium LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(magic_enum CONFIG REQUIRED)
find_package(SDL3 CONFIG REQUIRED)
find_package(Catch2 CONFIG REQUIRED)

set(sources ${CMAKE_CURRENT_SOURCE_DIR}/src/helium)

set(helium_headers
    ${sources}/core/world.hpp
    ${sources}/core_macro.hpp
    ${sources}/systems/logging.hpp
    ${sources}/systems/system_base.hpp
    ${sources}/systems/root_system.hpp
    ${sources}/utils/singleton.hpp
    ${sources}/utils/containers/ordered_tree.hpp
    ${sources}/utils/type_traits/type_index.hpp
    ${sources}/utils/type_traits/type_name.hpp
    ${sources}/utils/type_traits/type_id.hpp
    ${sources}/utils/delegate/delegate.hpp
    ${sources}/utils/delegate/multicast_delegate.hpp)

set(helium_sources
    ${sources}/systems/logging.cpp
    ${sources}/systems/system_base.cpp
    ${sources}/core/world.cpp)

add_library(helium STATIC ${helium_sources})

target_link_libraries(helium PUBLIC magic_enum::magic_enum SDL3::SDL3)

target_include_directories(helium PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src/)
target_include_directories(helium PUBLIC ${sources})


project(helium_tests LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(test_sources
    ${sources}/systems/test/logging_test.cpp
    ${sources}/utils/containers/test/ordered_tree_test.cpp
    ${sources}/utils/type_traits/test/type_index_test.cpp
    ${sources}/utils/type_traits/test/type_name_test.cpp
    ${sources}/utils/type_traits/test/type_id_test.cpp
    ${sources}/utils/delegate/test/delegate_test.cpp
    ${sources}/utils/delegate/test/multicast_delegate_test.cpp
    ${sources}/utils/test/event_bus_test.cpp
)

add_executable(helium_test_suite ${test_sources})
target_link_libraries(helium_test_suite PRIVATE helium Catch2::Catch2WithMain)



