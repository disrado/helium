cmake_minimum_required(VERSION 4.0)

project(helium LANGUAGES CXX)


set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(magic_enum CONFIG REQUIRED)
find_package(SDL3 CONFIG REQUIRED)
find_package(Catch2 CONFIG REQUIRED)

set(engine_headers
    src/engine/core/system.hpp
    src/engine/core/system_base.hpp
    src/engine/core/world.hpp
    src/engine/core_macro.hpp
    src/engine/utils/type_traits/type_index.hpp
    src/engine/utils/type_traits/type_name.hpp
    src/engine/utils/type_traits/type_id.hpp
    src/engine/utils/log/log.hpp)

set(engine_sources
    src/engine/core/system.cpp
    src/engine/core/system_base.cpp
    src/engine/core/world.cpp
    src/engine/utils/log/log.cpp)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)
add_library(engine STATIC ${engine_sources})
target_link_libraries(engine PRIVATE magic_enum::magic_enum PUBLIC SDL3::SDL3)

set(game_headers
    src/game/systems/log_system.hpp
)

set(game_sources
    src/game/systems/log_system.cpp
    src/main.cpp
)

add_executable(game ${game_headers} ${game_sources})
target_link_libraries(game PRIVATE engine)


set(TESTS
    src/engine/utils/type_traits/test/type_index_test.cpp
    src/engine/utils/type_traits/test/type_name_test.cpp
    src/engine/utils/type_traits/test/type_id_test.cpp
    src/engine/utils/test/delegate_test.cpp
    src/engine/utils/log/test/log_test.cpp)

add_executable(test_suite ${TESTS})

target_link_libraries(test_suite PRIVATE engine)
target_link_libraries(test_suite PRIVATE Catch2::Catch2WithMain)
