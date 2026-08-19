if(WIN32)
    set(HELIUM_GAME_BIN_EXT ".exe")
else()
    set(HELIUM_GAME_BIN_EXT "")
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(HELIUM_GAME_SUFFIX "helium-debug")
else()
    set(HELIUM_GAME_SUFFIX "helium-release")
endif()

file(GLOB HELIUM_GAME_BIN_CANDIDATES ${CMAKE_SOURCE_DIR}/deps/godot/bin/*.${HELIUM_GAME_SUFFIX}${HELIUM_GAME_BIN_EXT})
list(LENGTH HELIUM_GAME_BIN_CANDIDATES HELIUM_GAME_BIN_COUNT)

if(HELIUM_GAME_BIN_COUNT EQUAL 1)
    list(GET HELIUM_GAME_BIN_CANDIDATES 0 HELIUM_GAME_BIN)
    add_test(NAME game_startup
        COMMAND ${HELIUM_GAME_BIN} --path ${CMAKE_SOURCE_DIR} --headless --quit
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
    set_tests_properties(game_startup PROPERTIES
        TIMEOUT 60
        FAIL_REGULAR_EXPRESSION "SCRIPT ERROR;ERROR:")
elseif(HELIUM_GAME_BIN_COUNT GREATER 1)
    message(WARNING "Multiple godot_game binaries match *.${HELIUM_GAME_SUFFIX}${HELIUM_GAME_BIN_EXT} in deps/godot/bin - game_startup test not registered, clean up stale builds")
endif()
