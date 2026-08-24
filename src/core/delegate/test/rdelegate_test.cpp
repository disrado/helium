#include "core/delegate/rdelegate.hpp"

#include <catch2/catch_test_macros.hpp>


namespace
{

auto free_function() -> int
{
    return 7;
}

}

TEST_CASE("rdelegate")
{
    struct owner final
    {
        auto callable() -> int { return 1; };
    };

    SECTION("instantiating via default constructor")
    {
        const auto delegate{ he::rdelegate<int>{} };

        REQUIRE_FALSE(delegate.is_bound());
    }

    SECTION("instantiating from callable without parameters")
    {
        const auto delegate{ he::rdelegate<int>{ [] { return 1; } } };

        REQUIRE(delegate.is_bound());
    }

    SECTION("instantiating from callable with parameters")
    {
        const auto delegate{ he::rdelegate<int, bool, uint32_t>{ [] (bool, uint32_t) { return 1; } } };

        REQUIRE(delegate.is_bound());
    }

    SECTION("instantiating from lambda")
    {
        const auto lamda{ [] { return 1; } };

        const auto delegate{ he::rdelegate<int>{ std::move(lamda) } };

        REQUIRE(delegate.is_bound());
    }

    SECTION("instantiating from free function")
    {
        const auto delegate{ he::rdelegate<int>{ std::weak_ptr{ std::make_shared<owner>() }, &free_function } };

        REQUIRE(delegate.is_bound());
    }

    SECTION("instantiating from std::function")
    {
        const auto delegate{ he::rdelegate<int>{ std::function<int()>{} } };

        REQUIRE(delegate.is_bound());
    }

    SECTION("instantiating from member function weak owner")
    {
        const auto delegate{ he::rdelegate<int>{ std::weak_ptr{ std::make_shared<owner>() }, &owner::callable } };

        REQUIRE(delegate.is_bound());
    }

    SECTION("copy construction")
    {
        const auto original{ he::rdelegate<int>{ [] { return 1; } } };
        const auto copy{ original };

        REQUIRE(original.is_bound());
        REQUIRE(copy.is_bound());
        REQUIRE(*original.execute() == 1);
        REQUIRE(*copy.execute() == 1);
    }

    SECTION("copy assignment")
    {
        const auto original{ he::rdelegate<int>{ [] { return 1; } } };
        auto copy{ he::rdelegate<int>{} };

        copy = original;

        REQUIRE(copy.is_bound());
        REQUIRE(*copy.execute() == 1);
    }

    SECTION("move construction")
    {
        auto original{ he::rdelegate<int>{ [] { return 1; } } };
        const auto moved{ std::move(original) };

        REQUIRE(moved.is_bound());
        REQUIRE(*moved.execute() == 1);
    }

    SECTION("move assignment")
    {
        auto original{ he::rdelegate<int>{ [] { return 1; } } };
        auto target{ he::rdelegate<int>{} };

        target = std::move(original);

        REQUIRE(target.is_bound());
        REQUIRE(*target.execute() == 1);
    }
}

TEST_CASE("rdelegate execution")
{
    SECTION("execution of unbound delegate")
    {
        REQUIRE_FALSE(he::rdelegate<int>{}.try_execute().has_value());
    }

    SECTION("trying to execute")
    {
        REQUIRE_FALSE(he::rdelegate<int>{}.try_execute().has_value());

        const auto result{ he::rdelegate<int>{ [] { return 42; } }.try_execute() };

        REQUIRE(result.has_value());
        REQUIRE(*result == 42);
    }

    SECTION("execution")
    {
        const auto delegate{ he::rdelegate<int>{ [] { return 42; } } };

        const auto result{ delegate.execute() };

        REQUIRE(result.has_value());
        REQUIRE(*result == 42);
    }

    SECTION("execution of unbound delegate returns nullopt")
    {
        const auto delegate{ he::rdelegate<int>{} };

        REQUIRE_FALSE(delegate.execute().has_value());
    }

    SECTION("multiple execution")
    {
        auto counter{ 0 };

        const auto delegate{ he::rdelegate<int>{ [&counter] { return ++counter; } } };

        REQUIRE(*delegate.execute() == 1);
        REQUIRE(*delegate.execute() == 2);
        REQUIRE(*delegate.execute() == 3);
    }

    SECTION("execution with non-related owner")
    {
        struct owner final
        {
        };

        REQUIRE_FALSE(he::rdelegate<int>{ std::weak_ptr<owner>{}, [] { return 1; } }.try_execute().has_value());

        const auto lifetime_owner{ std::make_shared<owner>() };

        const auto result{ he::rdelegate<int>{ std::weak_ptr{ lifetime_owner }, [] { return 1; } }.try_execute() };

        REQUIRE(result.has_value());
        REQUIRE(*result == 1);
    }

    SECTION("execution prevented if lifetime owner is dead")
    {
        struct owner final
        {
        };

        auto lifetime_owner{ std::make_shared<owner>() };

        const auto delegate{ he::rdelegate<int>{ std::weak_ptr{ lifetime_owner }, [] { return 1; } } };

        REQUIRE(delegate.execute().has_value());

        lifetime_owner.reset();

        REQUIRE_FALSE(delegate.execute().has_value());
    }

    SECTION("execution through member pointer")
    {
        struct owner final
        {
            int value{ 0 };
            auto increment() -> int { return ++value; }
        };

        auto lifetime_owner{ std::make_shared<owner>() };

        const auto delegate{ he::rdelegate<int>{ std::weak_ptr{ lifetime_owner }, &owner::increment } };

        REQUIRE(*delegate.execute() == 1);
        REQUIRE(*delegate.execute() == 2);

        lifetime_owner.reset();

        REQUIRE_FALSE(delegate.execute().has_value());
    }

    SECTION("execution after rebinding")
    {
        auto delegate{ he::rdelegate<int>{ [] { return 1; } } };

        REQUIRE(*delegate.execute() == 1);

        delegate.bind([] { return 2; });

        REQUIRE(*delegate.execute() == 2);
    }

    SECTION("rebinding to a lifetime-owned callback")
    {
        struct owner final
        {
        };

        auto delegate{ he::rdelegate<int>{ [] { return 1; } } };

        auto lifetime_owner{ std::make_shared<owner>() };

        delegate.bind(std::weak_ptr{ lifetime_owner }, [] { return 2; });

        REQUIRE(*delegate.execute() == 2);

        lifetime_owner.reset();

        REQUIRE_FALSE(delegate.execute().has_value());
    }

    SECTION("is_bound reflects assignment, not lifetime owner liveness")
    {
        struct owner final
        {
        };

        auto lifetime_owner{ std::make_shared<owner>() };

        const auto delegate{ he::rdelegate<int>{ std::weak_ptr{ lifetime_owner }, [] { return 1; } } };

        REQUIRE(delegate.is_bound());

        lifetime_owner.reset();

        REQUIRE(delegate.is_bound());
        REQUIRE_FALSE(delegate.execute().has_value());
    }

    SECTION("try_execute reflects lifetime owner dying mid-flight")
    {
        struct owner final
        {
        };

        auto lifetime_owner{ std::make_shared<owner>() };

        const auto delegate{ he::rdelegate<int>{ std::weak_ptr{ lifetime_owner }, [] { return 1; } } };

        REQUIRE(delegate.try_execute().has_value());

        lifetime_owner.reset();

        REQUIRE_FALSE(delegate.try_execute().has_value());
    }

    SECTION("verifying parameters consistency")
    {
        constexpr auto first_value{ 29ul };
        constexpr auto second_value{ false };

        const auto callback{ [] (uint32_t first_argument, bool second_argument)
        {
            return first_value == first_argument && second_value == second_argument;
        } };

        const auto result{ he::rdelegate<bool, uint32_t, bool>{ callback }.execute(first_value, second_value) };

        REQUIRE(result.has_value());
        REQUIRE(*result);
    }
}
