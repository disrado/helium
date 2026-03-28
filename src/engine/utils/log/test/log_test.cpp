#include "engine/utils/log/log.hpp"

#include <catch2/catch_test_macros.hpp>


TEST_CASE("lock_free_queue", "[lock_free_queue]")
{
    auto log{ he::log{ [] (const std::string&&) {} } };
}
