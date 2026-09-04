#include "core/execution/task_graph.hpp"

#include <catch2/catch_test_macros.hpp>

#include <memory>
#include <string>


TEST_CASE("task_graph tree")
{
    SECTION("root has no parent")
    {
        auto graph{ std::make_shared<he::exec::task_graph>() };

        REQUIRE(graph->root().parent() == nullptr);
    }

    SECTION("root returns same node instance")
    {
        auto graph{ std::make_shared<he::exec::task_graph>() };

        REQUIRE(&graph->root() == &graph->root());
    }

    SECTION("add_child appends to children")
    {
        auto graph{ std::make_shared<he::exec::task_graph>() };
        auto& child{ graph->root().add_child() };

        REQUIRE(graph->root().children().size() == 1);
        REQUIRE(graph->root().children().front().get() == &child);
    }

    SECTION("add_child sets parent")
    {
        auto graph{ std::make_shared<he::exec::task_graph>() };
        auto& child{ graph->root().add_child() };

        REQUIRE(child.parent() == &graph->root());
    }
}


TEST_CASE("task_graph activation")
{
    SECTION("runs bound definition")
    {
        auto ran{ false };

        auto graph{ std::make_shared<he::exec::task_graph>() };

        graph->root().definition.bind([&ran] (std::stop_token) { ran = true; });

        graph->activate(graph->root());

        REQUIRE(ran);
    }

    SECTION("fires post_execution after definition")
    {
        auto order{ std::string{} };

        auto graph{ std::make_shared<he::exec::task_graph>() };

        graph->root().definition.bind([&order] (std::stop_token) { order += "d"; });
        graph->root().post_execution.bind([&order] (he::exec::execution_status) { order += "p"; });

        graph->activate(graph->root());

        REQUIRE(order == "dp");
    }

    SECTION("fires post_execution when no definition bound")
    {
        auto fired{ false };

        auto graph{ std::make_shared<he::exec::task_graph>() };

        graph->root().post_execution.bind([&fired] (he::exec::execution_status) { fired = true; });

        graph->activate(graph->root());

        REQUIRE(fired);
    }

    SECTION("skips node when pre_condition false")
    {
        auto ran{ false };
        auto fired{ false };

        auto graph{ std::make_shared<he::exec::task_graph>() };

        graph->root().definition.bind([&ran] (std::stop_token) { ran = true; });
        graph->root().post_execution.bind([&fired] (he::exec::execution_status) { fired = true; });
        graph->root().pre_condition.bind([] { return false; });

        graph->activate(graph->root());

        REQUIRE_FALSE(ran);
        REQUIRE_FALSE(fired);
    }

    SECTION("runs when pre_condition unbound")
    {
        auto ran{ false };

        auto graph{ std::make_shared<he::exec::task_graph>() };

        graph->root().definition.bind([&ran] (std::stop_token) { ran = true; });

        graph->activate(graph->root());

        REQUIRE(ran);
    }
}


TEST_CASE("task_graph traversal")
{
    SECTION("post_execution activates child")
    {
        auto ran{ false };

        auto graph{ std::make_shared<he::exec::task_graph>() };

        auto& child{ graph->root().add_child() };
        child.definition.bind([&ran] (std::stop_token) { ran = true; });

        graph->root().post_execution.bind([&child] (he::exec::execution_status) { child.activate(); });

        graph->activate(graph->root());

        REQUIRE(ran);
    }

    SECTION("post_execution activates multiple children")
    {
        auto first_ran{ false };
        auto second_ran{ false };

        auto graph{ std::make_shared<he::exec::task_graph>() };

        auto& first{ graph->root().add_child() };
        first.definition.bind([&first_ran] (std::stop_token) { first_ran = true; });

        auto& second{ graph->root().add_child() };
        second.definition.bind([&second_ran] (std::stop_token) { second_ran = true; });

        graph->root().post_execution.bind(
            [&first, &second]
            (he::exec::execution_status)
            {
                first.activate();
                second.activate();
            });

        graph->activate(graph->root());

        REQUIRE(first_ran);
        REQUIRE(second_ran);
    }

    SECTION("only activated child runs")
    {
        auto taken_ran{ false };
        auto skipped_ran{ false };

        auto graph{ std::make_shared<he::exec::task_graph>() };

        auto& taken{ graph->root().add_child() };
        taken.definition.bind([&taken_ran] (std::stop_token) { taken_ran = true; });

        auto& skipped{ graph->root().add_child() };
        skipped.definition.bind([&skipped_ran] (std::stop_token) { skipped_ran = true; });

        graph->root().post_execution.bind([&taken] (he::exec::execution_status) { taken.activate(); });

        graph->activate(graph->root());

        REQUIRE(taken_ran);
        REQUIRE_FALSE(skipped_ran);
    }
}


TEST_CASE("task_graph node activate")
{
    SECTION("forwards to graph activation")
    {
        auto ran{ false };

        auto graph{ std::make_shared<he::exec::task_graph>() };

        graph->root().definition.bind([&ran] (std::stop_token) { ran = true; });

        graph->root().activate();

        REQUIRE(ran);
    }
}
