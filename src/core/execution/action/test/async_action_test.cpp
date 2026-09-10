#include "core/execution/action/action.hpp"
#include "core/execution/action/async_action.hpp"
#include "core/execution/run.hpp"
#include "core/execution/scheduler.hpp"
#include "core/execution/task/task_graph.hpp"

#include <catch2/catch_test_macros.hpp>

#include <atomic>
#include <memory>
#include <stop_token>
#include <thread>
#include <tuple>
#include <variant>


TEST_CASE("async_action")
{
    SECTION("adds itself as a child")
    {
        auto instance{ std::make_shared<he::async_action>( [] (const he::async_action::context&) { return true; } ) };

        auto graph{ std::make_shared<he::exec::task_graph>() };
        auto& node{ instance->translate_into_graph(graph->root()).start };

        REQUIRE(graph->root().get_children().size() == 1);
        REQUIRE(graph->root().get_children().front().get() == &node);
    }

    SECTION("uses async launch policy")
    {
        auto instance{ std::make_shared<he::async_action>( [] (const he::async_action::context&) { return true; } ) };

        auto graph{ std::make_shared<he::exec::task_graph>() };
        auto& node{ instance->translate_into_graph(graph->root()).start };

        REQUIRE(std::holds_alternative<he::exec::async_task_request>(node.request));
    }

    SECTION("wires its own execute() as the definition")
    {
        auto ran{ false };

        auto instance{
            std::make_shared<he::async_action>( [&ran] (const he::async_action::context&)
            {
                ran = true;
                return true;
            } )
        };

        auto graph{ std::make_shared<he::exec::task_graph>() };
        auto& node{ instance->translate_into_graph(graph->root()).start };

        const auto result{ std::get<he::exec::async_task_request>(node.request).definition.try_execute(std::stop_token{}) };

        REQUIRE(ran);
        REQUIRE(result == he::exec::task_result::succeeded);
    }

    SECTION("reports failure")
    {
        auto instance{ std::make_shared<he::async_action>( [] (const he::async_action::context&) { return false; } ) };

        auto graph{ std::make_shared<he::exec::task_graph>() };
        auto& node{ instance->translate_into_graph(graph->root()).start };

        const auto result{ std::get<he::exec::async_task_request>(node.request).definition.try_execute(std::stop_token{}) };

        REQUIRE(result == he::exec::task_result::failed);
    }
}


TEST_CASE("async_action cancel")
{
    SECTION("cancels an in-flight task")
    {
        auto started{ std::atomic<bool>{ false } };
        auto observed_cancel{ std::atomic<bool>{ false } };

        auto instance{
            he::async_action{ [&started, &observed_cancel] (const he::async_action::context&, std::stop_token token)
            {
                started = true;

                while (!token.stop_requested())
                {
                }

                observed_cancel = true;

                return false;
            } }
        };

        auto token{ he::run(std::move(instance)) };

        while (!started)
        {
        }

        token.cancel();

        while (!observed_cancel)
        {
        }

        REQUIRE(observed_cancel);

        he::exec::scheduler::instance().tick();
    }

    SECTION("cancel from another thread")
    {
        auto started{ std::atomic<bool>{ false } };
        auto observed_cancel{ std::atomic<bool>{ false } };

        auto instance{
            he::async_action{ [&started, &observed_cancel] (const he::async_action::context&, std::stop_token stop)
            {
                started = true;

                while (!stop.stop_requested())
                {
                }

                observed_cancel = true;

                return false;
            } }
        };

        auto token{ he::run(std::move(instance)) };

        while (!started)
        {
        }

        auto canceller{ std::thread{ [&token] { token.cancel(); } } };

        while (!observed_cancel)
        {
        }

        canceller.join();

        REQUIRE(observed_cancel);

        he::exec::scheduler::instance().tick();
    }
}


TEST_CASE("async_action chaining")
{
    SECTION("and_then runs on success")
    {
        auto then_ran{ std::atomic<bool>{ false } };

        auto token{
            he::run(
                he::async_action{ [] (const he::async_action::context&) { return true; } }
                    .and_then(
                        he::action{ [&then_ran] (const he::action::context&)
                        {
                            then_ran = true;
                            return true;
                        } }))
        };

        while (!then_ran)
        {
            he::exec::scheduler::instance().tick();
        }

        REQUIRE(then_ran);
    }

    SECTION("or_else runs on failure")
    {
        auto otherwise_ran{ std::atomic<bool>{ false } };

        auto token{
            he::run(
                he::async_action{ [] (const he::async_action::context&) { return false; } }
                    .or_else(
                        he::action{ [&otherwise_ran] (const he::action::context&)
                        {
                            otherwise_ran = true;
                            return true;
                        } }))
        };

        while (!otherwise_ran)
        {
            he::exec::scheduler::instance().tick();
        }

        REQUIRE(otherwise_ran);
    }

    SECTION("and_then skipped on failure")
    {
        auto then_ran{ std::atomic<bool>{ false } };
        auto otherwise_ran{ std::atomic<bool>{ false } };

        auto token{
            he::run(
                he::async_action{ [] (const he::async_action::context&) { return false; } }
                    .and_then(
                        he::action{ [&then_ran] (const he::action::context&)
                        {
                            then_ran = true;
                            return true;
                        } })
                    .or_else(
                        he::action{ [&otherwise_ran] (const he::action::context&)
                        {
                            otherwise_ran = true;
                            return true;
                        } }))
        };

        while (!otherwise_ran)
        {
            he::exec::scheduler::instance().tick();
        }

        REQUIRE_FALSE(then_ran);
        REQUIRE(otherwise_ran);
    }
}
