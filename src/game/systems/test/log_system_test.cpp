#include "game/systems/log_system.hpp"

#include <catch2/catch_test_macros.hpp>


TEST_CASE("type id")
{
    ne::info{ "tag", "format: {}", "message", 10, std::string{} };
}