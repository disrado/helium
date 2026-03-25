#include "engine/utils/delegate.hpp"

#include <catch2/catch_test_macros.hpp>


namespace details
{
auto free_function() -> void {}
}

TEST_CASE("delegate", "[delegate construction]")
{
    struct owner final
    {
        auto callable() -> void {};
    };

    SECTION("instantiating via default constructor")
    {
        he::delegate delegate{};

        REQUIRE_FALSE(delegate.is_bound());
    }

    SECTION("instantiating from callable without parameters")
    {
        he::delegate delegate{ [] {} };

        REQUIRE(delegate.is_bound());
    }

    SECTION("instantiating from callable with parameters")
    {
        he::delegate<bool, uint32_t> delegate{ [] (bool, uint32_t) {} };

        REQUIRE(delegate.is_bound());
    }

    SECTION("instantiating from lambda")
    {
        const auto lamda{ [] {} };

        he::delegate delegate{ std::move(lamda) };

        REQUIRE(delegate.is_bound());
    }

    SECTION("instantiating from free function")
    {
        he::delegate delegate{ &details::free_function };

        REQUIRE(delegate.is_bound());
    }

    SECTION("instantiating from std::function")
    {
        he::delegate delegate{ std::function<void()>{} };

        REQUIRE(delegate.is_bound());
    }

    SECTION("instantiating from member function weak owner")
    {
        he::delegate delegate{ std::weak_ptr{ std::make_shared<owner>() }, &owner::callable };

        REQUIRE(delegate.is_bound());
    }

    SECTION("instantiating from member function and shared owner")
    {
        he::delegate delegate{ std::make_shared<owner>(), &owner::callable };

        REQUIRE(delegate.is_bound());
    }
}

TEST_CASE("delegate execution", "[delegate execution]")
{
    auto counter{ 0l };

    const auto increment_counter{ [&counter] { counter++; } };
    const auto decrement_counter{ [&counter] { counter--; } };

    SECTION("execution of unbound delegate")
    {
        REQUIRE_FALSE(he::delegate{}.try_execute());
    }

    SECTION("trying to execute")
    {
        REQUIRE_FALSE(he::delegate{}.try_execute());

        REQUIRE(he::delegate{ increment_counter }.try_execute());
        REQUIRE(counter == 1);
    }

    SECTION("execution")
    {
        he::delegate delegate{ increment_counter };

        delegate.execute();

        REQUIRE(counter == 1);
    }

    SECTION("multiple execution")
    {
        he::delegate delegate{ increment_counter };

        delegate.execute();
        REQUIRE(counter == 1);

        delegate.execute();
        REQUIRE(counter == 2);

        delegate.execute();
        REQUIRE(counter == 3);
    }

    SECTION("execution after rebinding")
    {
        he::delegate delegate{ decrement_counter };

        delegate.execute();
        REQUIRE(counter == -1);

        delegate.execute();
        REQUIRE(counter == -2);

        delegate.bind(increment_counter);

        delegate.execute();
        REQUIRE(counter == -1);

        delegate.execute();
        delegate.execute();
        REQUIRE(counter == 1);
    }
}
