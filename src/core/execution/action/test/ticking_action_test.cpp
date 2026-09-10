#include "core/execution/action/action.hpp"
#include "core/execution/action/ticking_action.hpp"
#include "core/execution/run.hpp"
#include "core/execution/scheduler.hpp"

#include <catch2/catch_test_macros.hpp>


TEST_CASE("ticking_action")
{
    SECTION("first tick waits for a tick() call")
    {
        auto tick_count{ 0 };

        auto token{
            he::run(
                he::ticking_action{
                    [&tick_count] (const he::ticking_action::context&, std::stop_token) -> he::ticking_action::result
                    {
                        ++tick_count;
                        return he::ticking_action::result::keep_going;
                    } })
        };

        REQUIRE(tick_count == 0);

        he::exec::scheduler::instance().tick();

        REQUIRE(tick_count == 1);

        token.cancel();
        he::exec::scheduler::instance().tick();
    }

    SECTION("never fires more than once per tick() call")
    {
        auto tick_count{ 0 };

        auto token{
            he::run(
                he::ticking_action{
                    [&tick_count] (const he::ticking_action::context&, std::stop_token) -> he::ticking_action::result
                    {
                        ++tick_count;
                        return he::ticking_action::result::keep_going;
                    } })
        };

        he::exec::scheduler::instance().tick();
        REQUIRE(tick_count == 1);

        he::exec::scheduler::instance().tick();
        REQUIRE(tick_count == 2);

        he::exec::scheduler::instance().tick();
        REQUIRE(tick_count == 3);

        token.cancel();
        he::exec::scheduler::instance().tick();
    }

    SECTION("runs across multiple tick() calls, then succeeds")
    {
        auto tick_count{ 0 };
        auto done{ false };

        auto token{
            he::run(
                he::ticking_action{
                    [&tick_count] (const he::ticking_action::context&, std::stop_token) -> he::ticking_action::result
                    {
                        ++tick_count;
                        return tick_count < 3 ? he::ticking_action::result::keep_going : he::ticking_action::result::succeeded;
                    } }
                .and_then(
                    he::action{ [&done] (const he::action::context&)
                    {
                        done = true;
                        return true;
                    } }))
        };

        while (!done)
        {
            he::exec::scheduler::instance().tick();
        }

        REQUIRE(tick_count == 3);
        REQUIRE(done);
    }

    SECTION("and_then runs exactly once after succeeded")
    {
        auto then_count{ 0 };

        auto token{
            he::run(
                he::ticking_action{
                    [] (const he::ticking_action::context&, std::stop_token) { return he::ticking_action::result::succeeded; }
                }
                .and_then(
                    he::action{ [&then_count] (const he::action::context&)
                    {
                        ++then_count;
                        return true;
                    } }))
        };

        he::exec::scheduler::instance().tick();

        REQUIRE(then_count == 1);

        he::exec::scheduler::instance().tick();

        REQUIRE(then_count == 1);
    }

    SECTION("or_else runs exactly once after failed")
    {
        auto otherwise_count{ 0 };

        auto token{
            he::run(
                he::ticking_action{
                    [] (const he::ticking_action::context&, std::stop_token) { return he::ticking_action::result::failed; }
                }
                .or_else(
                    he::action{ [&otherwise_count] (const he::action::context&)
                    {
                        ++otherwise_count;
                        return true;
                    } }))
        };

        he::exec::scheduler::instance().tick();

        REQUIRE(otherwise_count == 1);

        he::exec::scheduler::instance().tick();

        REQUIRE(otherwise_count == 1);
    }

    SECTION("cancel mid-tick stops further invocations")
    {
        auto tick_count{ 0 };

        auto token{
            he::run(
                he::ticking_action{
                    [&tick_count] (const he::ticking_action::context&, std::stop_token) -> he::ticking_action::result
                    {
                        ++tick_count;
                        return he::ticking_action::result::keep_going;
                    } })
        };

        he::exec::scheduler::instance().tick();
        REQUIRE(tick_count == 1);

        token.cancel();

        he::exec::scheduler::instance().tick();

        REQUIRE(tick_count == 1);
    }
}
