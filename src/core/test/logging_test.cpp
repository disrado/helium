#include "core/logging.hpp"

#include <catch2/catch_test_macros.hpp>


TEST_CASE("logging")
{
    const auto noop{ [] (const he::log_entry&) {} };

    he::logging::instance().set_severity_dispatcher(info, noop);
    he::logging::instance().set_severity_dispatcher(warning, noop);
    he::logging::instance().set_severity_dispatcher(error, noop);

    he::log{ info, "tag", "format: {}", "message", 10, std::string{} };

    he::log{ warning, "tag", "formated message" };

    he::log{ error, "tag", "unexpected error" };

    he::log{ error, "tag", "format: {}:{}", "unexpected error", 10 };

    he::log{ info, "tag", "format: {}:{}:{}", "unexpected error", 10, "" };


    SUCCEED();
}
