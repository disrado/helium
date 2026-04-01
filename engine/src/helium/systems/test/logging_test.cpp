#include "systems/root_system.hpp"
#include "systems/logging.hpp"

#include <catch2/catch_test_macros.hpp>


TEST_CASE("logging_system")
{
    he::world::get_system<he::root_system>().add_subsystem<ne::logging>();

    ne::info{ "tag", "format: {}", "message", 10, std::string{} };

    ne::warning{ "tag", "formated message" };

    ne::error{ "tag", "unexpected error" };

    SUCCEED();
}
