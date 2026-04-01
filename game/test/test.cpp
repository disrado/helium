#include "systems/logging.hpp"

#include <catch2/catch_test_macros.hpp>



TEST_CASE("game test")
{
    ne::info{ "tag", "format: {}", "message", 10, std::string{} };

    SUCCEED();
}
