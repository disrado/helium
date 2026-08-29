#include "core/execution/action/action.hpp"

#include <catch2/catch_test_macros.hpp>


namespace
{

auto succeeding_callback(const he::action::context&) -> bool
{
    return true;
}

}


TEST_CASE("basic_action construction")
{
    SECTION("default")
    {
        const auto instance{ he::action{} };

        REQUIRE(instance.get_state() == he::action::state::dormant);
    }

    SECTION("from callable")
    {
        const auto instance{ he::action{ [] (const he::action::context&) { return true; } } };

        REQUIRE(instance.get_state() == he::action::state::dormant);
    }

    SECTION("from delegate")
    {
        auto delegate{ he::delegate<bool(const he::action::context&)>{ &succeeding_callback } };

        const auto instance{ he::action{ std::move(delegate) } };

        REQUIRE(instance.get_state() == he::action::state::dormant);
    }

    SECTION("from delegate with initial context")
    {
        auto delegate{ he::delegate<bool(const he::action::context&)>{ &succeeding_callback } };

        auto context{ he::action::context{ { "flag", true } } };

        auto instance{ he::action{ std::move(delegate), std::move(context) } };

        instance.execute();

        REQUIRE(instance.get_state() == he::action::state::succeeded);
    }

    SECTION("move construction")
    {
        auto original{ he::action{ [] (const he::action::context&) { return true; } } };
        const auto moved{ std::move(original) };

        REQUIRE(moved.get_state() == he::action::state::dormant);
    }

    SECTION("move assignment")
    {
        auto original{ he::action{ [] (const he::action::context&) { return true; } } };
        auto target{ he::action{} };

        target = std::move(original);

        REQUIRE(target.get_state() == he::action::state::dormant);
    }
}


TEST_CASE("basic_action execution")
{
    SECTION("succeeds")
    {
        auto instance{ he::action{ [] (const he::action::context&) { return true; } } };

        instance.execute();

        REQUIRE(instance.get_state() == he::action::state::succeeded);
    }

    SECTION("fails")
    {
        auto instance{ he::action{ [] (const he::action::context&) { return false; } } };

        instance.execute();

        REQUIRE(instance.get_state() == he::action::state::failed);
    }

    SECTION("fails when unbound")
    {
        auto instance{ he::action{} };

        instance.execute();

        REQUIRE(instance.get_state() == he::action::state::failed);
    }

    SECTION("context passed to callable")
    {
        auto received{ false };

        auto context{ he::action::context{ { "flag", true } } };

        auto instance{
            he::action{
                [&received] (const he::action::context& ctx)
                {
                    received = std::any_cast<bool>(ctx.at("flag"));
                    return true;
                },
                std::move(context)
            }
        };

        instance.execute();

        REQUIRE(received);
    }
}


TEST_CASE("basic_action abort")
{
    SECTION("sets aborted state")
    {
        auto instance{ he::action{ [] (const he::action::context&) { return true; } } };

        instance.abort();

        REQUIRE(instance.get_state() == he::action::state::aborted);
    }

    SECTION("fires aborted listeners")
    {
        auto fired{ false };

        auto instance{ he::action{ [] (const he::action::context&) { return true; } } };

        instance.on(he::action::state::aborted, [&fired] { fired = true; });

        instance.abort();

        REQUIRE(fired);
    }

    SECTION("clears branches so they don't run afterward")
    {
        auto then_ran{ false };

        auto root{ he::action{ [] (const he::action::context&) { return true; } } };

        root.then(
            he::action{ [&then_ran] (const he::action::context&)
            {
                then_ran = true;
                return true;
            } });

        root.abort();
        root.execute();

        REQUIRE_FALSE(then_ran);
    }
}


TEST_CASE("basic_action on/set_state")
{
    SECTION("succeeded listener fires")
    {
        auto fired{ false };

        auto instance{ he::action{ [] (const he::action::context&) { return true; } } };

        instance.on(he::action::state::succeeded, [&fired] { fired = true; });

        instance.execute();

        REQUIRE(fired);
    }

    SECTION("failed listener fires")
    {
        auto fired{ false };

        auto instance{ he::action{ [] (const he::action::context&) { return false; } } };

        instance.on(he::action::state::failed, [&fired] { fired = true; });

        instance.execute();

        REQUIRE(fired);
    }

    SECTION("unmatched listener doesn't fire")
    {
        auto fired{ false };

        auto instance{ he::action{ [] (const he::action::context&) { return true; } } };

        instance.on(he::action::state::failed, [&fired] { fired = true; });

        instance.execute();

        REQUIRE_FALSE(fired);
    }

    SECTION("multiple listeners fire")
    {
        auto first_fired{ false };
        auto second_fired{ false };

        auto instance{ he::action{ [] (const he::action::context&) { return true; } } };

        instance.on(he::action::state::succeeded, [&first_fired] { first_fired = true; });
        instance.on(he::action::state::succeeded, [&second_fired] { second_fired = true; });

        instance.execute();

        REQUIRE(first_fired);
        REQUIRE(second_fired);
    }

    SECTION("accepts a pre-built delegate")
    {
        auto fired{ false };

        auto instance{ he::action{ [] (const he::action::context&) { return true; } } };

        instance.on(he::action::state::succeeded, he::delegate{ [&fired] { fired = true; } });

        instance.execute();

        REQUIRE(fired);
    }
}
