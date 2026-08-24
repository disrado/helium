cmake_minimum_required(VERSION 4.3)

project(helium LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(magic_enum CONFIG REQUIRED)
find_package(Catch2 CONFIG REQUIRED)

set(helium_headers
    core/core_macro.hpp
    core/singleton.hpp
    core/event_bus.hpp
    core/string_literal.hpp
    core/containers/ordered_tree.hpp
    core/type_traits/type_index.hpp
    core/type_traits/type_name.hpp
    core/type_traits/type_id.hpp
    core/delegate/delegate.hpp
    core/delegate/rdelegate.hpp
    core/delegate/multicast_delegate.hpp
    core/delegate/utils.hpp
    core/execution/action/action.hpp
    core/execution/utils.hpp
    core/execution/run.hpp
    systems/logging.hpp
    systems/system_base.hpp
    systems/root_system.hpp)

set(helium_sources
    core/execution/action/action.cpp
    core/execution/run.cpp
    systems/logging.cpp
    systems/system_base.cpp)

add_library(helium STATIC ${helium_sources})

target_link_libraries(helium PUBLIC magic_enum::magic_enum)

target_include_directories(helium PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if(WIN32)
    # CMake's default CRT resolution lands on the wrong runtime. Force the correct, config-aware value explicitly
    set_target_properties(helium PROPERTIES MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
endif()


project(helium_tests LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(test_sources
    systems/test/logging_test.cpp
    core/containers/test/ordered_tree_test.cpp
    core/type_traits/test/type_index_test.cpp
    core/type_traits/test/type_name_test.cpp
    core/type_traits/test/type_id_test.cpp
    core/delegate/test/delegate_test.cpp
    core/delegate/test/rdelegate_test.cpp
    core/delegate/test/multicast_delegate_test.cpp
    core/test/event_bus_test.cpp
    core/execution/test/action_test.cpp
    core/execution/test/run_test.cpp
)

add_executable(helium_test_suite ${test_sources})
target_link_libraries(helium_test_suite PRIVATE helium Catch2::Catch2WithMain)

if(WIN32)
    # CMake's default CRT resolution lands on the wrong runtime. Force the correct, config-aware value explicitly
    set_target_properties(helium_test_suite PROPERTIES MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
endif()

add_test(NAME helium_tests COMMAND helium_test_suite)
