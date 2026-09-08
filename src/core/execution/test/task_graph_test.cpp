#include "core/delegate/delegate.hpp"
#include "core/execution/task_graph.hpp"

#include <catch2/catch_test_macros.hpp>

#include <any>
#include <memory>
#include <optional>
#include <string>


TEST_CASE("task_graph tree")
{
    SECTION("root has no get_parent")
    {
        auto graph{ std::make_shared<he::exec::task_graph>() };

        REQUIRE(graph->root().get_parent() == nullptr);
    }

    SECTION("root returns same node instance")
    {
        auto graph{ std::make_shared<he::exec::task_graph>() };

        REQUIRE(&graph->root() == &graph->root());
    }

    SECTION("add_child appends to get_children")
    {
        auto graph{ std::make_shared<he::exec::task_graph>() };
        auto& child{ graph->root().add_child() };

        REQUIRE(graph->root().get_children().size() == 1);
        REQUIRE(graph->root().get_children().front().get() == &child);
    }

    SECTION("add_child sets get_parent")
    {
        auto graph{ std::make_shared<he::exec::task_graph>() };
        auto& child{ graph->root().add_child() };

        REQUIRE(child.get_parent() == &graph->root());
    }
}


TEST_CASE("task_graph activation")
{
    SECTION("runs bound definition")
    {
        auto ran{ false };

        auto graph{ std::make_shared<he::exec::task_graph>() };

        graph->root().definition.bind(
            [&ran] (std::stop_token)
            {
                ran = true;
                return he::exec::execution_status::completed;
            });

        graph->activate(graph->root());

        REQUIRE(ran);
    }

    SECTION("fires post_execution after definition")
    {
        auto order{ std::string{} };

        auto graph{ std::make_shared<he::exec::task_graph>() };

        graph->root().definition.bind(
            [&order] (std::stop_token)
            {
                order += "d";
                return he::exec::execution_status::completed;
            });
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

        graph->root().definition.bind(
            [&ran] (std::stop_token)
            {
                ran = true;
                return he::exec::execution_status::completed;
            });
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

        graph->root().definition.bind(
            [&ran] (std::stop_token)
            {
                ran = true;
                return he::exec::execution_status::completed;
            });

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
        child.definition.bind(
            [&ran] (std::stop_token)
            {
                ran = true;
                return he::exec::execution_status::completed;
            });

        graph->root().post_execution.bind([&child] (he::exec::execution_status) { child.activate(); });

        graph->activate(graph->root());

        REQUIRE(ran);
    }

    SECTION("post_execution activates multiple get_children")
    {
        auto first_ran{ false };
        auto second_ran{ false };

        auto graph{ std::make_shared<he::exec::task_graph>() };

        auto& first{ graph->root().add_child() };
        first.definition.bind(
            [&first_ran] (std::stop_token)
            {
                first_ran = true;
                return he::exec::execution_status::completed;
            });

        auto& second{ graph->root().add_child() };
        second.definition.bind(
            [&second_ran] (std::stop_token)
            {
                second_ran = true;
                return he::exec::execution_status::completed;
            });

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
        taken.definition.bind(
            [&taken_ran] (std::stop_token)
            {
                taken_ran = true;
                return he::exec::execution_status::completed;
            });

        auto& skipped{ graph->root().add_child() };
        skipped.definition.bind(
            [&skipped_ran] (std::stop_token)
            {
                skipped_ran = true;
                return he::exec::execution_status::completed;
            });

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

        graph->root().definition.bind(
            [&ran] (std::stop_token)
            {
                ran = true;
                return he::exec::execution_status::completed;
            });

        graph->root().activate();

        REQUIRE(ran);
    }
}


TEST_CASE("task_node context")
{
    SECTION("empty by default")
    {
        auto graph{ std::make_shared<he::exec::task_graph>() };

        REQUIRE_FALSE(graph->root().get_context().has_value());
    }

    SECTION("set_context stores")
    {
        auto graph{ std::make_shared<he::exec::task_graph>() };

        graph->root().set_context(he::exec::action_context{ { "key", std::string{ "value" } } });

        REQUIRE(graph->root().get_context().has_value());
        REQUIRE(std::any_cast<std::string>(graph->root().get_context().value().at("key")) == "value");
    }

    SECTION("merge nullopt no-op")
    {
        auto graph{ std::make_shared<he::exec::task_graph>() };

        graph->root().set_context(he::exec::action_context{ { "key", std::string{ "value" } } });
        graph->root().merge_context(std::nullopt);

        REQUIRE(std::any_cast<std::string>(graph->root().get_context().value().at("key")) == "value");
    }

    SECTION("merge into empty")
    {
        auto graph{ std::make_shared<he::exec::task_graph>() };

        graph->root().merge_context(he::exec::action_context{ { "key", std::string{ "value" } } });

        REQUIRE(graph->root().get_context().has_value());
        REQUIRE(std::any_cast<std::string>(graph->root().get_context().value().at("key")) == "value");
    }

    SECTION("merge adds key")
    {
        auto graph{ std::make_shared<he::exec::task_graph>() };

        graph->root().set_context(he::exec::action_context{ { "first", std::string{ "a" } } });
        graph->root().merge_context(he::exec::action_context{ { "second", std::string{ "b" } } });

        REQUIRE(std::any_cast<std::string>(graph->root().get_context().value().at("first")) == "a");
        REQUIRE(std::any_cast<std::string>(graph->root().get_context().value().at("second")) == "b");
    }

    SECTION("merge overwrites key")
    {
        auto graph{ std::make_shared<he::exec::task_graph>() };

        graph->root().set_context(he::exec::action_context{ { "key", std::string{ "old" } } });
        graph->root().merge_context(he::exec::action_context{ { "key", std::string{ "new" } } });

        REQUIRE(std::any_cast<std::string>(graph->root().get_context().value().at("key")) == "new");
    }
}


TEST_CASE("task_node links")
{
    SECTION("get_links empty by default")
    {
        auto graph{ std::make_shared<he::exec::task_graph>() };

        REQUIRE(graph->root().get_links().empty());
    }

    SECTION("add_link stores entry")
    {
        auto graph{ std::make_shared<he::exec::task_graph>() };
        auto& target{ graph->root().add_child() };

        graph->root().add_link(
            {
                .condition{ he::delegate<bool(he::exec::action_state)>{ [] (he::exec::action_state) { return true; } } },
                .target{ &target }
            });

        REQUIRE(graph->root().get_links().size() == 1);
        REQUIRE(graph->root().get_links().front().target == &target);
    }

    SECTION("resolve_links activates match")
    {
        auto graph{ std::make_shared<he::exec::task_graph>() };
        auto& target{ graph->root().add_child() };

        auto activated{ false };
        target.definition.bind(
            [&activated] (std::stop_token)
            {
                activated = true;
                return he::exec::execution_status::completed;
            });

        graph->root().add_link(
            {
                .condition{ he::delegate<bool(he::exec::action_state)>{
                    [] (he::exec::action_state s) { return s == he::exec::action_state::succeeded; } } },
                .target{ &target }
            });

        graph->root().state = he::exec::action_state::succeeded;
        graph->root().resolve_links();

        REQUIRE(activated);
    }

    SECTION("resolve_links no match is no-op")
    {
        auto graph{ std::make_shared<he::exec::task_graph>() };
        auto& target{ graph->root().add_child() };

        auto activated{ false };
        target.definition.bind(
            [&activated] (std::stop_token)
            {
                activated = true;
                return he::exec::execution_status::completed;
            });

        graph->root().add_link(
            {
                .condition{ he::delegate<bool(he::exec::action_state)>{
                    [] (he::exec::action_state s) { return s == he::exec::action_state::succeeded; } } },
                .target{ &target }
            });

        graph->root().state = he::exec::action_state::failed;
        graph->root().resolve_links();

        REQUIRE_FALSE(activated);
    }

    SECTION("resolve_links: first match wins")
    {
        auto graph{ std::make_shared<he::exec::task_graph>() };
        auto& first_target{ graph->root().add_child() };
        auto& second_target{ graph->root().add_child() };

        auto first_activated{ false };
        auto second_activated{ false };
        first_target.definition.bind(
            [&first_activated] (std::stop_token)
            {
                first_activated = true;
                return he::exec::execution_status::completed;
            });
        second_target.definition.bind(
            [&second_activated] (std::stop_token)
            {
                second_activated = true;
                return he::exec::execution_status::completed;
            });

        const auto always{ he::delegate<bool(he::exec::action_state)>{ [] (he::exec::action_state) { return true; } } };

        graph->root().add_link({ .condition{ always }, .target{ &first_target } });
        graph->root().add_link({ .condition{ always }, .target{ &second_target } });

        graph->root().resolve_links();

        REQUIRE(first_activated);
        REQUIRE_FALSE(second_activated);
    }

    SECTION("resolve_links propagates context")
    {
        auto graph{ std::make_shared<he::exec::task_graph>() };
        auto& target{ graph->root().add_child() };

        graph->root().set_context(he::exec::action_context{ { "key", std::string{ "value" } } });

        auto received{ std::optional<std::string>{} };
        target.definition.bind(
            [&target, &received] (std::stop_token)
            {
                if (target.get_context().has_value())
                {
                    received = std::any_cast<std::string>(target.get_context().value().at("key"));
                }

                return he::exec::execution_status::completed;
            });

        graph->root().add_link(
            {
                .condition{ he::delegate<bool(he::exec::action_state)>{ [] (he::exec::action_state) { return true; } } },
                .target{ &target }
            });

        graph->root().resolve_links();

        REQUIRE(received.has_value());
        REQUIRE(received.value() == "value");
    }
}
