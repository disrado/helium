find_program(SCONS_EXECUTABLE NAMES scons)

if(SCONS_EXECUTABLE)
    include(ProcessorCount)
    ProcessorCount(HELIUM_NPROC)

    if(HELIUM_NPROC EQUAL 0)
        set(HELIUM_NPROC 4)
    endif()

    if(WIN32)
        set(HELIUM_GODOT_PLATFORM_ARGS platform=windows d3d12=no use_llvm=yes)
    else()
        set(HELIUM_GODOT_PLATFORM_ARGS platform=linuxbsd)
    endif()

    add_custom_target(godot_editor
        COMMAND ${SCONS_EXECUTABLE} ${HELIUM_GODOT_PLATFORM_ARGS} target=editor dev_build=yes accesskit=no "-j${HELIUM_NPROC}"
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/deps/godot
        USES_TERMINAL VERBATIM)
endif()
