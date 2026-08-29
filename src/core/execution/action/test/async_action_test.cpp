#include "core/execution/action/action.hpp"
#include "core/execution/action/async_action.hpp"
#include "core/execution/scheduler.hpp"
#include "core/execution/task_graph.hpp"

#include <catch2/catch_test_macros.hpp>

#include <memory>
#include <thread>


TEST_CASE("async_action")
{
    SECTION("adds itself as a child")
    {
        auto instance{ he::async_action{ [] (const he::async_action::context&) { return true; } } };

        auto graph{ std::make_shared<he::exec::task_graph>() };
        auto& node{ instance.build_graph(graph->root()) };

        REQUIRE(graph->root().children().size() == 1);
        REQUIRE(graph->root().children().front().get() == &node);
    }

    SECTION("runs on a worker thread")
    {
        auto worker_thread_id{ std::thread::id{} };
        auto completed{ false };

        auto instance{
            he::async_action{ [&worker_thread_id] (const he::async_action::context&)
            {
                worker_thread_id = std::this_thread::get_id();
                return true;
            } }
        };

        instance.then(
            he::action{ [&completed] (const he::action::context&)
            {
                completed = true;
                return true;
            } });

        auto graph{ std::make_shared<he::exec::task_graph>() };
        graph->activate(instance.build_graph(graph->root()));

        while (!completed)
        {
            he::exec::scheduler::instance().process();
        }

        REQUIRE(worker_thread_id != std::this_thread::get_id());
        REQUIRE(instance.get_state() == he::async_action::state::succeeded);
    }

    SECTION("reports failure")
    {
        auto completed{ false };

        auto instance{ he::async_action{ [] (const he::async_action::context&) { return false; } } };

        instance.otherwise(
            he::action{ [&completed] (const he::action::context&)
            {
                completed = true;
                return true;
            } });

        auto graph{ std::make_shared<he::exec::task_graph>() };
        graph->activate(instance.build_graph(graph->root()));

        while (!completed)
        {
            he::exec::scheduler::instance().process();
        }

        REQUIRE(instance.get_state() == he::async_action::state::failed);
    }
}
