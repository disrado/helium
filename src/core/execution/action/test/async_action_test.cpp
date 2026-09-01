#include "core/execution/action/async_action.hpp"
#include "core/execution/run.hpp"
#include "core/execution/scheduler.hpp"
#include "core/execution/task_graph.hpp"

#include <catch2/catch_test_macros.hpp>

#include <atomic>
#include <memory>
#include <stop_token>
#include <tuple>


TEST_CASE("async_action")
{
    SECTION("adds itself as a child")
    {
        auto instance{ he::async_action{ [] (const he::async_action::context&) { return true; } } };

        auto graph{ std::make_shared<he::exec::task_graph>() };
        auto& node{ instance.translate_into_graph(graph->root()).begin };

        REQUIRE(graph->root().children().size() == 1);
        REQUIRE(graph->root().children().front().get() == &node);
    }

    SECTION("uses async launch policy")
    {
        auto instance{ he::async_action{ [] (const he::async_action::context&) { return true; } } };

        auto graph{ std::make_shared<he::exec::task_graph>() };
        auto& node{ instance.translate_into_graph(graph->root()).begin };

        REQUIRE(node.mode == he::exec::launch_policy::async);
    }

    SECTION("wires its own execute() as the definition")
    {
        auto ran{ false };

        auto instance{
            he::async_action{ [&ran] (const he::async_action::context&)
            {
                ran = true;
                return true;
            } }
        };

        auto graph{ std::make_shared<he::exec::task_graph>() };
        auto& node{ instance.translate_into_graph(graph->root()).begin };

        std::ignore = node.definition.try_execute(std::stop_token{});

        REQUIRE(ran);
        REQUIRE(instance.get_state() == he::async_action::state::succeeded);
    }

    SECTION("reports failure")
    {
        auto instance{ he::async_action{ [] (const he::async_action::context&) { return false; } } };

        auto graph{ std::make_shared<he::exec::task_graph>() };
        auto& node{ instance.translate_into_graph(graph->root()).begin };

        std::ignore = node.definition.try_execute(std::stop_token{});

        REQUIRE(instance.get_state() == he::async_action::state::failed);
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

        auto chain{ he::run(std::move(instance)) };

        chain.execute();

        while (!started)
        {
        }

        chain.cancel();

        while (!observed_cancel)
        {
        }

        REQUIRE(observed_cancel);

        he::exec::scheduler::instance().process();
    }
}
