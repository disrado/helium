#include "core/execution/action/action.hpp"
#include "core/execution/action/async_action.hpp"
#include "core/execution/run.hpp"

#include <catch2/catch_test_macros.hpp>

#include <condition_variable>
#include <mutex>


TEST_CASE("run")
{
    SECTION("execute forwards")
    {
        auto ran{ false };

        const auto chain{ he::run(
            he::action{ [&ran] (const he::action::context&)
            {
                ran = true;
                return true;
            } }) };

        chain.execute();

        REQUIRE(ran);
    }

    SECTION("abort forwards")
    {
        auto fired{ false };

        auto instance{ he::action{ [] (const he::action::context&) { return true; } } };

        instance.on(he::action::state::aborted, [&fired] { fired = true; });

        const auto chain{ he::run(std::move(instance)) };

        chain.abort();

        REQUIRE(fired);
    }

    SECTION("works with a custom action_base subclass, not just he::action")
    {
        static auto custom_execute_ran{ false };
        custom_execute_ran = false;

        class custom_action final: public he::action
        {
        public:
            auto execute() -> void override
            {
                custom_execute_ran = true;

                succeed();
            }
        };

        const auto chain{ he::run(custom_action{}) };

        chain.execute();

        REQUIRE(custom_execute_ran);
    }

    SECTION("execute() is repeatable")
    {
        auto run_count{ 0 };

        const auto chain{ he::run(
            he::action{ [&run_count] (const he::action::context&)
            {
                ++run_count;
                return true;
            } }) };

        chain.execute();
        chain.execute();

        REQUIRE(run_count == 2);
    }

    SECTION("survives async completion after execute() returns")
    {
        auto mutex{ std::mutex{} };
        auto cv{ std::condition_variable{} };
        auto done{ false };

        auto instance{ he::async_action{ [] (const he::async_action::context&) { return true; } } };

        instance.on(
            he::async_action::state::succeeded,
            [&]
            {
                const auto _{ std::lock_guard{ mutex } };
                done = true;
                cv.notify_one();
            });

        const auto chain{ he::run(std::move(instance)) };

        chain.execute();

        auto lock{ std::unique_lock{ mutex } };
        cv.wait(lock, [&] { return done; });

        REQUIRE(done);
    }
}
