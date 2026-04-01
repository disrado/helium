#include "core/world.hpp"

#include <catch2/catch_test_macros.hpp>


TEST_CASE("logging_system")
{
    const auto& world{ he::world::instance() };

    SUCCEED();
}
