#include "systems/root_system.hpp"
#include "systems/logging.hpp"

#include <catch2/catch_test_macros.hpp>


TEST_CASE("logging_system")
{
    he::world::get_system<he::root_system>().add_subsystem<ne::logging>();

    ne::log{ info, "tag", "format: {}", "message", 10, std::string{} };

    ne::log{ warning, "tag", "formated message" };

    ne::log{ error, "tag", "unexpected error" };

    ne::log{ error, "tag", "format: {}:{}", "unexpected error", 10 };

    ne::log{ info, "tag", "format: {}:{}:{}", "unexpected error", 10, "" };


    SUCCEED();
}
