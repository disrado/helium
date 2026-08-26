add_library(helium_gdextension SHARED
    register_types.cpp
    system_node.cpp
    core/execution/gd_dispatcher.cpp)

target_link_libraries(helium_gdextension PRIVATE helium godot::cpp)

set_target_properties(helium_gdextension PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build/bin)

if(WIN32)
    # this target inherits the same wrong СКЕ value unless, enforce it target's own CRT
    set_target_properties(helium_gdextension PROPERTIES MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
endif()

# distinct per build type so debug/release outputs coexist in bin/, matching Godot's own convention
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set_target_properties(helium_gdextension PROPERTIES OUTPUT_NAME "helium_gdextension.windows.template_debug.x86_64")
else()
    set_target_properties(helium_gdextension PROPERTIES OUTPUT_NAME "helium_gdextension.windows.template_release.x86_64")
endif()
