#include "core/execution/action/action.hpp"
#include "core/execution/task_graph.hpp"

#include <catch2/catch_test_macros.hpp>

#include <any>
#include <memory>
#include <stop_token>
#include <string>
#include <tuple>


TEST_CASE("action")
{
    SECTION("adds itself as a child")
    {
        auto instance{ std::make_shared<he::action>( [] (const he::action::context&) { return true; } ) };

        auto graph{ std::make_shared<he::exec::task_graph>() };
        auto& node{ instance->translate_into_graph(graph->root()).start };

        REQUIRE(graph->root().children().size() == 1);
        REQUIRE(graph->root().children().front().get() == &node);
    }

    SECTION("wires its own execute() as the definition")
    {
        auto ran{ false };

        auto instance{
            std::make_shared<he::action>( [&ran] (const he::action::context&)
            {
                ran = true;
                return true;
            } )
        };

        auto graph{ std::make_shared<he::exec::task_graph>() };
        auto& node{ instance->translate_into_graph(graph->root()).start };

        std::ignore = node.definition.try_execute(std::stop_token{});

        REQUIRE(ran);
        REQUIRE(node.state == he::action::state::succeeded);
    }

    SECTION("reports failure")
    {
        auto instance{ std::make_shared<he::action>( [] (const he::action::context&) { return false; } ) };

        auto graph{ std::make_shared<he::exec::task_graph>() };
        auto& node{ instance->translate_into_graph(graph->root()).start };

        std::ignore = node.definition.try_execute(std::stop_token{});

        REQUIRE(node.state == he::action::state::failed);
    }
}


TEST_CASE("action chaining")
{
    SECTION("and_then runs on success")
    {
        auto then_ran{ false };

        auto root{ std::make_shared<he::action>( [] (const he::action::context&) { return true; } ) };

        root->and_then(
            he::action{ [&then_ran] (const he::action::context&)
            {
                then_ran = true;
                return true;
            } });

        auto graph{ std::make_shared<he::exec::task_graph>() };
        graph->activate(root->translate_into_graph(graph->root()).start);

        REQUIRE(then_ran);
    }

    SECTION("or_else runs on failure")
    {
        auto otherwise_ran{ false };

        auto root{ std::make_shared<he::action>( [] (const he::action::context&) { return false; } ) };

        root->or_else(
            he::action{ [&otherwise_ran] (const he::action::context&)
            {
                otherwise_ran = true;
                return true;
            } });

        auto graph{ std::make_shared<he::exec::task_graph>() };
        graph->activate(root->translate_into_graph(graph->root()).start);

        REQUIRE(otherwise_ran);
    }

    SECTION("and_then skipped on failure")
    {
        auto then_ran{ false };

        auto root{ std::make_shared<he::action>( [] (const he::action::context&) { return false; } ) };

        root->and_then(
            he::action{ [&then_ran] (const he::action::context&)
            {
                then_ran = true;
                return true;
            } });

        auto graph{ std::make_shared<he::exec::task_graph>() };
        graph->activate(root->translate_into_graph(graph->root()).start);

        REQUIRE_FALSE(then_ran);
    }

    SECTION("or_else skipped on success")
    {
        auto otherwise_ran{ false };

        auto root{ std::make_shared<he::action>( [] (const he::action::context&) { return true; } ) };

        root->or_else(
            he::action{ [&otherwise_ran] (const he::action::context&)
            {
                otherwise_ran = true;
                return true;
            } });

        auto graph{ std::make_shared<he::exec::task_graph>() };
        graph->activate(root->translate_into_graph(graph->root()).start);

        REQUIRE_FALSE(otherwise_ran);
    }

    SECTION("only the branch matching the outcome runs")
    {
        auto then_ran{ false };
        auto otherwise_ran{ false };

        auto root{ std::make_shared<he::action>( [] (const he::action::context&) { return true; } ) };

        root->and_then(
            he::action{ [&then_ran] (const he::action::context&)
            {
                then_ran = true;
                return true;
            } });
        root->or_else(
            he::action{ [&otherwise_ran] (const he::action::context&)
            {
                otherwise_ran = true;
                return true;
            } });

        auto graph{ std::make_shared<he::exec::task_graph>() };
        graph->activate(root->translate_into_graph(graph->root()).start);

        REQUIRE(then_ran);
        REQUIRE_FALSE(otherwise_ran);
    }

    SECTION("nested chain in order")
    {
        auto order{ std::string{} };

        auto graph{ std::make_shared<he::exec::task_graph>() };

        graph->activate(
            std::make_shared<he::action>( [&order] (const he::action::context&)
            {
                order += "a";
                return true;
            } )
            ->and_then(
                he::action{ [&order] (const he::action::context&)
                {
                    order += "b";
                    return true;
                } }
                .and_then(
                    he::action{ [&order] (const he::action::context&)
                    {
                        order += "c";
                        return true;
                    } }))
            .translate_into_graph(graph->root()).start);

        REQUIRE(order == "abc");
    }

    SECTION("preserves subclass override")
    {
        static auto custom_execute_ran{ false };
        custom_execute_ran = false;

        class custom_action final: public he::action
        {
        public:
            auto execute(he::exec::task_node& self, std::stop_token) -> void override
            {
                custom_execute_ran = true;

                self.state = state::succeeded;
            }
        };

        auto graph{ std::make_shared<he::exec::task_graph>() };

        graph->activate(
            std::make_shared<he::action>( [] (const he::action::context&) { return true; } )
            ->and_then(custom_action{})
            .translate_into_graph(graph->root()).start);

        REQUIRE(custom_execute_ran);
    }

    SECTION("context propagates to branch")
    {
        auto received{ false };

        auto context{ he::action::context{ { "flag", true } } };

        auto root{
            std::make_shared<he::action>( [] (const he::action::context&) { return true; } )
        };

        root->and_then(
            he::action{
                [&received] (const he::action::context& ctx)
                {
                    received = std::any_cast<bool>(ctx.at("flag"));
                    return true;
                }
            });

        auto graph{ std::make_shared<he::exec::task_graph>() };
        auto segment{ root->translate_into_graph(graph->root()) };

        segment.start.set_context(std::move(context));

        graph->activate(segment.start);

        REQUIRE(received);
    }
}


TEST_CASE("action cancel")
{
    SECTION("harmless after completion")
    {
        auto ran{ false };

        auto root{ std::make_shared<he::action>( [&ran] (const he::action::context&)
        {
            ran = true;
            return true;
        } ) };

        auto graph{ std::make_shared<he::exec::task_graph>() };
        auto& node{ root->translate_into_graph(graph->root()).start };

        graph->activate(node);

        REQUIRE(ran);

        graph->cancel();

        REQUIRE(node.state == he::action::state::succeeded);
    }
}
