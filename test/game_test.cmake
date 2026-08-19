if(WIN32)
    set(GAME_BIN_EXT ".exe")
else()
    set(GAME_BIN_EXT "")
endif()

set(GAME_SUFFIX "${CMAKE_PROJECT_NAME}-${HELIUM_BUILD_TYPE}")

file(GLOB GAME_BIN_CANDIDATES ${CMAKE_SOURCE_DIR}/deps/godot/bin/*.${GAME_SUFFIX}${GAME_BIN_EXT})
list(LENGTH GAME_BIN_CANDIDATES GAME_BIN_COUNT)

if(GAME_BIN_COUNT EQUAL 1)
    list(GET GAME_BIN_CANDIDATES 0 GAME_BIN)

    add_test(NAME game_startup
        COMMAND ${GAME_BIN} --path ${CMAKE_SOURCE_DIR} --headless --quit
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

    set_tests_properties(game_startup PROPERTIES
        TIMEOUT 60
        FAIL_REGULAR_EXPRESSION "SCRIPT ERROR;ERROR:")
elseif(GAME_BIN_COUNT GREATER 1)
    message(WARNING
        "Multiple godot_game binaries match *.${GAME_SUFFIX}${GAME_BIN_EXT} in deps/godot/bin - "
        "game_startup test not registered, clean up stale builds")
endif()
