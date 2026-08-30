#include "core/delegate/delegate.hpp"

#include <catch2/catch_test_macros.hpp>

#include <optional>


namespace
{

auto free_function() -> void
{
    //
}

auto free_int_function() -> int
{
    return 7;
}

}

TEST_CASE("delegate")
{
    struct owner final
    {
        auto callable() -> void {};
    };

    SECTION("instantiating via default constructor")
    {
        const auto delegate{ he::delegate{} };

        REQUIRE_FALSE(delegate.is_bound());
    }

    SECTION("instantiating from callable without parameters")
    {
        const auto delegate{ he::delegate{ [] {} } };

        REQUIRE(delegate.is_bound());
    }

    SECTION("instantiating from callable with parameters")
    {
        const auto delegate{ he::delegate<void(bool, uint32_t)>{ [] (bool, uint32_t) {} } };

        REQUIRE(delegate.is_bound());
    }

    SECTION("instantiating from lambda")
    {
        const auto lamda{ [] {} };

        const auto delegate{ he::delegate{ std::move(lamda) } };

        REQUIRE(delegate.is_bound());
    }

    SECTION("instantiating from free function")
    {
        const auto delegate{ he::delegate{ std::weak_ptr{ std::make_shared<owner>() }, &free_function } };

        REQUIRE(delegate.is_bound());
    }

    SECTION("instantiating from std::function")
    {
        const auto delegate{ he::delegate{ std::function<void()>{} } };

        REQUIRE(delegate.is_bound());
    }

    SECTION("instantiating from member function weak owner")
    {
        const auto delegate{ he::delegate{ std::weak_ptr{ std::make_shared<owner>() }, &owner::callable } };

        REQUIRE(delegate.is_bound());
    }

    SECTION("copy construction")
    {
        auto counter{ 0l };

        const auto original{ he::delegate{ [&counter] { counter++; } } };
        const auto copy{ original };

        REQUIRE(original.is_bound());
        REQUIRE(copy.is_bound());

        original.execute();
        copy.execute();

        REQUIRE(counter == 2);
    }

    SECTION("copy assignment")
    {
        auto counter{ 0l };

        const auto original{ he::delegate{ [&counter] { counter++; } } };
        auto copy{ he::delegate{} };

        copy = original;

        REQUIRE(copy.is_bound());

        copy.execute();

        REQUIRE(counter == 1);
    }

    SECTION("move construction")
    {
        auto counter{ 0l };

        auto original{ he::delegate{ [&counter] { counter++; } } };
        const auto moved{ std::move(original) };

        REQUIRE(moved.is_bound());

        moved.execute();

        REQUIRE(counter == 1);
    }

    SECTION("move assignment")
    {
        auto counter{ 0l };

        auto original{ he::delegate{ [&counter] { counter++; } } };
        auto target{ he::delegate{} };

        target = std::move(original);

        REQUIRE(target.is_bound());

        target.execute();

        REQUIRE(counter == 1);
    }
}

TEST_CASE("delegate execution")
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
        const auto delegate{ he::delegate{ increment_counter } };

        delegate.execute();

        REQUIRE(counter == 1);
    }

    SECTION("multiple execution")
    {
        const auto delegate{ he::delegate{ increment_counter } };

        delegate.execute();
        REQUIRE(counter == 1);

        delegate.execute();
        REQUIRE(counter == 2);

        delegate.execute();
        REQUIRE(counter == 3);
    }

    SECTION("supports mutable callable")
    {
        auto seen{ 0 };

        const auto delegate{ he::delegate{ [count{ 0 }, &seen] () mutable { seen = ++count; } } };

        delegate.execute();
        REQUIRE(seen == 1);

        delegate.execute();
        REQUIRE(seen == 2);

        delegate.execute();
        REQUIRE(seen == 3);
    }

    SECTION("execution with non-related owner")
    {
        struct owner final
        {
        };

        REQUIRE_FALSE(he::delegate{ std::weak_ptr<owner>{}, increment_counter }.try_execute());

        const auto lifetime_owner{ std::make_shared<owner>() };

        REQUIRE(he::delegate{ std::weak_ptr{ lifetime_owner }, increment_counter }.try_execute());
        REQUIRE(counter == 1);
    }

    SECTION("execution prevented if lifetime owner is dead")
    {
        struct owner final
        {
        };

        auto lifetime_owner{ std::make_shared<owner>() };

        const auto delegate{ he::delegate{ std::weak_ptr{ lifetime_owner }, increment_counter } };

        delegate.execute();

        REQUIRE(counter == 1);

        lifetime_owner.reset();

        REQUIRE_FALSE(delegate.try_execute());

        REQUIRE(counter == 1);
    }

    SECTION("execution through member pointer")
    {
        struct owner final
        {
            long& counter;
            auto increment() -> void { counter++; }
        };

        auto lifetime_owner{ std::make_shared<owner>(counter) };

        const auto delegate{ he::delegate{ std::weak_ptr{ lifetime_owner }, &owner::increment } };

        delegate.execute();
        REQUIRE(counter == 1);

        delegate.execute();
        REQUIRE(counter == 2);

        lifetime_owner.reset();

        REQUIRE_FALSE(delegate.try_execute());
        REQUIRE(counter == 2);
    }

    SECTION("execution after rebinding")
    {
        auto delegate{ he::delegate{ decrement_counter } };

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

    SECTION("rebinding to a lifetime-owned callback")
    {
        struct owner final
        {
        };

        auto delegate{ he::delegate{ increment_counter } };

        auto lifetime_owner{ std::make_shared<owner>() };

        delegate.bind(std::weak_ptr{ lifetime_owner }, increment_counter);

        REQUIRE(delegate.try_execute());
        REQUIRE(counter == 1);

        lifetime_owner.reset();

        REQUIRE_FALSE(delegate.try_execute());
        REQUIRE(counter == 1);
    }

    SECTION("is_bound reflects assignment, not lifetime owner liveness")
    {
        struct owner final
        {
        };

        auto lifetime_owner{ std::make_shared<owner>() };

        const auto delegate{ he::delegate{ std::weak_ptr{ lifetime_owner }, increment_counter } };

        REQUIRE(delegate.is_bound());

        lifetime_owner.reset();

        REQUIRE(delegate.is_bound());
        REQUIRE_FALSE(delegate.try_execute());
    }

    SECTION("try_execute reflects lifetime owner dying mid-flight")
    {
        struct owner final
        {
        };

        auto lifetime_owner{ std::make_shared<owner>() };

        const auto delegate{ he::delegate{ std::weak_ptr{ lifetime_owner }, increment_counter } };

        REQUIRE(delegate.try_execute());
        REQUIRE(counter == 1);

        lifetime_owner.reset();

        REQUIRE_FALSE(delegate.try_execute());
        REQUIRE(counter == 1);
    }

    SECTION("verifying parameters consistency")
    {
        constexpr auto first_value{ 29ul };
        constexpr auto second_value{ false };

        auto are_values_valid{ false };

        const auto callback{ [=, &are_values_valid] (uint32_t first_argument, bool seconds_argument)
        {
            are_values_valid = first_value == first_argument && second_value == seconds_argument;
        } };

        he::delegate<void(uint32_t, bool)>{ callback }.execute(first_value, second_value);

        REQUIRE(are_values_valid);
    }
}


TEST_CASE("delegate with return value")
{
    struct owner final
    {
        auto callable() -> int { return 1; };
    };

    SECTION("instantiating via default constructor")
    {
        const auto delegate{ he::delegate<int()>{} };

        REQUIRE_FALSE(delegate.is_bound());
    }

    SECTION("instantiating from callable without parameters")
    {
        const auto delegate{ he::delegate{ [] { return 1; } } };

        REQUIRE(delegate.is_bound());
    }

    SECTION("instantiating from callable with parameters")
    {
        const auto delegate{ he::delegate<int(bool, uint32_t)>{ [] (bool, uint32_t) { return 1; } } };

        REQUIRE(delegate.is_bound());
    }

    SECTION("instantiating from lambda")
    {
        const auto lamda{ [] { return 1; } };

        const auto delegate{ he::delegate{ std::move(lamda) } };

        REQUIRE(delegate.is_bound());
    }

    SECTION("instantiating from free function")
    {
        const auto delegate{ he::delegate{ std::weak_ptr{ std::make_shared<owner>() }, &free_int_function } };

        REQUIRE(delegate.is_bound());
    }

    SECTION("instantiating from std::function")
    {
        const auto delegate{ he::delegate{ std::function<int()>{} } };

        REQUIRE(delegate.is_bound());
    }

    SECTION("instantiating from member function weak owner")
    {
        const auto delegate{ he::delegate{ std::weak_ptr{ std::make_shared<owner>() }, &owner::callable } };

        REQUIRE(delegate.is_bound());
    }

    SECTION("copy construction")
    {
        const auto original{ he::delegate{ [] { return 1; } } };
        const auto copy{ original };

        REQUIRE(original.is_bound());
        REQUIRE(copy.is_bound());
        REQUIRE(original.execute() == 1);
        REQUIRE(copy.execute() == 1);
    }

    SECTION("copy assignment")
    {
        const auto original{ he::delegate{ [] { return 1; } } };
        auto copy{ he::delegate<int()>{} };

        copy = original;

        REQUIRE(copy.is_bound());
        REQUIRE(copy.execute() == 1);
    }

    SECTION("move construction")
    {
        auto original{ he::delegate{ [] { return 1; } } };
        const auto moved{ std::move(original) };

        REQUIRE(moved.is_bound());
        REQUIRE(moved.execute() == 1);
    }

    SECTION("move assignment")
    {
        auto original{ he::delegate{ [] { return 1; } } };
        auto target{ he::delegate<int()>{} };

        target = std::move(original);

        REQUIRE(target.is_bound());
        REQUIRE(target.execute() == 1);
    }
}


TEST_CASE("delegate with return value execution")
{
    SECTION("try_execute on unbound delegate")
    {
        REQUIRE_FALSE(he::delegate<int()>{}.try_execute().has_value());
    }

    SECTION("trying to execute")
    {
        REQUIRE_FALSE(he::delegate<int()>{}.try_execute().has_value());

        const auto result{ he::delegate{ [] { return 42; } }.try_execute() };

        REQUIRE(result.has_value());
        REQUIRE(result.value() == 42);
    }

    SECTION("execution")
    {
        const auto delegate{ he::delegate{ [] { return 42; } } };

        REQUIRE(delegate.execute() == 42);
    }

    SECTION("multiple execution")
    {
        auto counter{ 0 };

        const auto delegate{ he::delegate{ [&counter] { return ++counter; } } };

        REQUIRE(delegate.execute() == 1);
        REQUIRE(delegate.execute() == 2);
        REQUIRE(delegate.execute() == 3);
    }

    SECTION("supports mutable callable")
    {
        auto delegate{ he::delegate<int()>{ [count{ 0 }] () mutable { return ++count; } } };

        REQUIRE(delegate.execute() == 1);
        REQUIRE(delegate.execute() == 2);
        REQUIRE(delegate.execute() == 3);
    }

    SECTION("execution with non-related owner")
    {
        struct owner final
        {
        };

        REQUIRE_FALSE(he::delegate{ std::weak_ptr<owner>{}, [] { return 1; } }.try_execute().has_value());

        const auto lifetime_owner{ std::make_shared<owner>() };

        const auto result{ he::delegate{ std::weak_ptr{ lifetime_owner }, [] { return 1; } }.try_execute() };

        REQUIRE(result.has_value());
        REQUIRE(result.value() == 1);
    }

    SECTION("execution through member pointer")
    {
        struct owner final
        {
            int value{ 0 };
            auto increment() -> int { return ++value; }
        };

        auto lifetime_owner{ std::make_shared<owner>() };

        const auto delegate{ he::delegate{ std::weak_ptr{ lifetime_owner }, &owner::increment } };

        REQUIRE(delegate.execute() == 1);
        REQUIRE(delegate.execute() == 2);
    }

    SECTION("execution after rebinding")
    {
        auto delegate{ he::delegate{ [] { return 1; } } };

        REQUIRE(delegate.execute() == 1);

        delegate.bind([] { return 2; });

        REQUIRE(delegate.execute() == 2);
    }

    SECTION("rebinding to a lifetime-owned callback")
    {
        struct owner final
        {
        };

        auto delegate{ he::delegate{ [] { return 1; } } };

        auto lifetime_owner{ std::make_shared<owner>() };

        delegate.bind(std::weak_ptr{ lifetime_owner }, [] { return 2; });

        REQUIRE(delegate.execute() == 2);
    }

    SECTION("is_bound reflects assignment, not lifetime owner liveness")
    {
        struct owner final
        {
        };

        auto lifetime_owner{ std::make_shared<owner>() };

        const auto delegate{ he::delegate{ std::weak_ptr{ lifetime_owner }, [] { return 1; } } };

        REQUIRE(delegate.is_bound());

        lifetime_owner.reset();

        REQUIRE(delegate.is_bound());
        REQUIRE_FALSE(delegate.try_execute().has_value());
    }

    SECTION("try_execute reflects lifetime owner dying mid-flight")
    {
        struct owner final
        {
        };

        auto lifetime_owner{ std::make_shared<owner>() };

        const auto delegate{ he::delegate{ std::weak_ptr{ lifetime_owner }, [] { return 1; } } };

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

        REQUIRE(he::delegate<bool(uint32_t, bool)>{ callback }.execute(first_value, second_value));
    }
}
