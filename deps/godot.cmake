find_program(SCONS_EXECUTABLE NAMES scons)

if(SCONS_EXECUTABLE)
    include(ProcessorCount)
    ProcessorCount(HELIUM_NPROC)

    if(HELIUM_NPROC EQUAL 0)
        set(HELIUM_NPROC 4)
    endif()

    if(WIN32)
        set(HELIUM_GODOT_PLATFORM_ARGS platform=windows d3d12=no angle=no use_llvm=yes)
    else()
        set(HELIUM_GODOT_PLATFORM_ARGS platform=linuxbsd)
    endif()
    set(HELIUM_GODOT_COMMON_ARGS ${HELIUM_GODOT_PLATFORM_ARGS} accesskit=no "-j${HELIUM_NPROC}")

    add_custom_target(godot_editor_dev
        COMMAND ${SCONS_EXECUTABLE} ${HELIUM_GODOT_COMMON_ARGS} target=editor dev_build=yes
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/deps/godot
        USES_TERMINAL VERBATIM)

    add_custom_target(godot_editor_release
        COMMAND ${SCONS_EXECUTABLE} ${HELIUM_GODOT_COMMON_ARGS} target=editor dev_build=no
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/deps/godot
        USES_TERMINAL VERBATIM)

    add_custom_target(godot_game_debug
        COMMAND ${SCONS_EXECUTABLE} ${HELIUM_GODOT_COMMON_ARGS} target=template_debug extra_suffix=.helium-debug
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/deps/godot
        USES_TERMINAL VERBATIM)

    add_custom_target(godot_game_release
        COMMAND ${SCONS_EXECUTABLE} ${HELIUM_GODOT_COMMON_ARGS} target=template_release extra_suffix=.helium-release
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/deps/godot
        USES_TERMINAL VERBATIM)
endif()
