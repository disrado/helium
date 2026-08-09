cmake_minimum_required(VERSION 4.3)

project(helium LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)

enable_testing()

add_subdirectory(src)

include(deps/godot.cmake)

# Keeps the Godot project (project.godot at repo root) from scanning/reimporting these as resources.
# Recreated on every configure since neither survives a fresh build dir / submodule reset.
file(TOUCH ${CMAKE_BINARY_DIR}/.gdignore)
file(TOUCH ${CMAKE_SOURCE_DIR}/deps/godot/.gdignore)
