#include "core/execution/action/async_action.hpp"
#include "core/execution/task_graph.hpp"

#include <catch2/catch_test_macros.hpp>

#include <memory>
#include <stop_token>
#include <tuple>


TEST_CASE("async_action")
{
    SECTION("adds itself as a child")
    {
        auto instance{ he::async_action{ [] (const he::async_action::context&) { return true; } } };

        auto graph{ std::make_shared<he::exec::task_graph>() };
        auto& node{ instance.translate_into_graph(graph->root()) };

        REQUIRE(graph->root().children().size() == 1);
        REQUIRE(graph->root().children().front().get() == &node);
    }

    SECTION("uses async launch policy")
    {
        auto instance{ he::async_action{ [] (const he::async_action::context&) { return true; } } };

        auto graph{ std::make_shared<he::exec::task_graph>() };
        auto& node{ instance.translate_into_graph(graph->root()) };

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
        auto& node{ instance.translate_into_graph(graph->root()) };

        std::ignore = node.definition.try_execute(std::stop_token{});

        REQUIRE(ran);
        REQUIRE(instance.get_state() == he::async_action::state::succeeded);
    }

    SECTION("reports failure")
    {
        auto instance{ he::async_action{ [] (const he::async_action::context&) { return false; } } };

        auto graph{ std::make_shared<he::exec::task_graph>() };
        auto& node{ instance.translate_into_graph(graph->root()) };

        std::ignore = node.definition.try_execute(std::stop_token{});

        REQUIRE(instance.get_state() == he::async_action::state::failed);
    }
}
