cmake_minimum_required(VERSION 4.3)

project(helium LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# no code here uses C++20 modules, and CMake's clang-scan-deps auto-detection produces a broken
# (empty) path under this clang-cl toolchain - disable module scanning rather than chase that.
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)

enable_testing()

add_subdirectory(src)

# do not propagate our compilation flags to godot-cpp
set(_saved_cxx_flags "${CMAKE_CXX_FLAGS}")
set(CMAKE_CXX_FLAGS "")
# SYSTEM: also silences warnings from godot-cpp's own headers when our own code is build against them
add_subdirectory(deps/godot_cpp SYSTEM)
set(CMAKE_CXX_FLAGS "${_saved_cxx_flags}")

if(WIN32)
    # godot-cpp bug: -lstdc++ leaks in under clang-cl, which doesn't use libstdc++ - strip it.
    get_target_property(_godotcpp_link_options godot-cpp INTERFACE_LINK_OPTIONS)
    if(_godotcpp_link_options)
        list(FILTER _godotcpp_link_options EXCLUDE REGEX "lstdc\\+\\+")
        set_target_properties(godot-cpp PROPERTIES INTERFACE_LINK_OPTIONS "${_godotcpp_link_options}")
    endif()

    # godot-cpp bug: sets a fixed non-debug CRT for this target regardless of config, breaking ABI
    # match against vcpkg's debug-triplet libs once this target's in the link graph.
    set_target_properties(godot-cpp PROPERTIES MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
endif()

add_subdirectory(src/gdextension)

include(deps/godot.cmake)
include(test/game_test.cmake)

# Keeps the Godot project (project.godot at repo root) from scanning/reimporting these as resources.
# Recreated on every configure since neither survives a fresh build dir / submodule reset.
file(TOUCH ${CMAKE_BINARY_DIR}/.gdignore)
file(TOUCH ${CMAKE_SOURCE_DIR}/deps/godot/.gdignore)
