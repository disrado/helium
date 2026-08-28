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


TEST_CASE("action_base chaining")
{
    SECTION("then runs on success")
    {
        auto then_ran{ false };

        auto root{ he::action{ [] (const he::action::context&) { return true; } } };

        root
        .then(
            he::action{ [&then_ran] (const he::action::context&)
            {
                then_ran = true;
                return true;
            } })
        .execute();

        REQUIRE(then_ran);
    }

    SECTION("otherwise runs on failure")
    {
        auto otherwise_ran{ false };

        auto root{ he::action{ [] (const he::action::context&) { return false; } } };

        root
        .otherwise(
            he::action{ [&otherwise_ran] (const he::action::context&)
            {
                otherwise_ran = true;
                return true;
            } })
        .execute();

        REQUIRE(otherwise_ran);
    }

    SECTION("then skipped on failure")
    {
        auto then_ran{ false };

        auto root{ he::action{ [] (const he::action::context&) { return false; } } };

        root
        .then(
            he::action{ [&then_ran] (const he::action::context&)
            {
                then_ran = true;
                return true;
            } })
        .execute();

        REQUIRE_FALSE(then_ran);
    }

    SECTION("otherwise skipped on success")
    {
        auto otherwise_ran{ false };

        auto root{ he::action{ [] (const he::action::context&) { return true; } } };

        root
        .otherwise(
            he::action{ [&otherwise_ran] (const he::action::context&)
            {
                otherwise_ran = true;
                return true;
            } })
        .execute();

        REQUIRE_FALSE(otherwise_ran);
    }

    SECTION("only the branch matching the outcome runs")
    {
        auto then_ran{ false };
        auto otherwise_ran{ false };

        auto root{ he::action{ [] (const he::action::context&) { return true; } } };

        root.then(
            he::action{ [&then_ran] (const he::action::context&)
            {
                then_ran = true;
                return true;
            } });
        root.otherwise(
            he::action{ [&otherwise_ran] (const he::action::context&)
            {
                otherwise_ran = true;
                return true;
            } });

        root.execute();

        REQUIRE(then_ran);
        REQUIRE_FALSE(otherwise_ran);
    }

    SECTION("nested chain in order")
    {
        auto order{ std::string{} };

        he::action{ [&order] (const he::action::context&)
        {
            order += "a";
            return true;
        } }
        .then(
            he::action{ [&order] (const he::action::context&)
            {
                order += "b";
                return true;
            } }
            .then(
                he::action{ [&order] (const he::action::context&)
                {
                    order += "c";
                    return true;
                } }))
        .execute();

        REQUIRE(order == "abc");
    }

    SECTION("preserves subclass override")
    {
        static auto custom_execute_ran{ false };
        custom_execute_ran = false;

        class custom_action final: public he::exec::action_base<custom_action>
        {
        public:
            auto execute() -> void override
            {
                custom_execute_ran = true;

                succeed();
            }
        };

        he::action{ [] (const he::action::context&) { return true; } }
        .then(custom_action{})
        .execute();

        REQUIRE(custom_execute_ran);
    }

    SECTION("context propagates to branch")
    {
        auto received{ false };

        auto context{ he::action::context{ { "flag", true } } };

        he::action{ [] (const he::action::context&) { return true; }, std::move(context) }
        .then(
            he::action{
                [&received] (const he::action::context& ctx)
                {
                    received = std::any_cast<bool>(ctx.at("flag"));
                    return true;
                }
            })
        .execute();

        REQUIRE(received);
    }
}


TEST_CASE("action default leaf")
{
    SECTION("constructible from callable")
    {
        auto ran{ false };

        he::action{ [&ran] (const he::action::context&)
        {
            ran = true;
            return true;
        } }.execute();

        REQUIRE(ran);
    }
}
