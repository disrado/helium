#include "core/execution/action/action.hpp"
#include "core/execution/task_graph.hpp"

#include <catch2/catch_test_macros.hpp>

#include <memory>
#include <stop_token>
#include <tuple>


TEST_CASE("action")
{
    SECTION("adds itself as a child")
    {
        auto instance{ he::action{ [] (const he::action::context&) { return true; } } };

        auto graph{ std::make_shared<he::exec::task_graph>() };
        auto& node{ instance.translate_into_graph(graph->root()).begin };

        REQUIRE(graph->root().children().size() == 1);
        REQUIRE(graph->root().children().front().get() == &node);
    }

    SECTION("wires its own execute() as the definition")
    {
        auto ran{ false };

        auto instance{
            he::action{ [&ran] (const he::action::context&)
            {
                ran = true;
                return true;
            } }
        };

        auto graph{ std::make_shared<he::exec::task_graph>() };
        auto& node{ instance.translate_into_graph(graph->root()).begin };

        std::ignore = node.definition.try_execute(std::stop_token{});

        REQUIRE(ran);
        REQUIRE(instance.get_state() == he::action::state::succeeded);
    }

    SECTION("reports failure")
    {
        auto instance{ he::action{ [] (const he::action::context&) { return false; } } };

        auto graph{ std::make_shared<he::exec::task_graph>() };
        auto& node{ instance.translate_into_graph(graph->root()).begin };

        std::ignore = node.definition.try_execute(std::stop_token{});

        REQUIRE(instance.get_state() == he::action::state::failed);
    }
}


TEST_CASE("action chaining")
{
    SECTION("then runs on success")
    {
        auto then_ran{ false };

        auto root{ he::action{ [] (const he::action::context&) { return true; } } };

        root.then(
            he::action{ [&then_ran] (const he::action::context&)
            {
                then_ran = true;
                return true;
            } });

        auto graph{ std::make_shared<he::exec::task_graph>() };
        graph->activate(root.translate_into_graph(graph->root()).begin);

        REQUIRE(then_ran);
    }

    SECTION("otherwise runs on failure")
    {
        auto otherwise_ran{ false };

        auto root{ he::action{ [] (const he::action::context&) { return false; } } };

        root.otherwise(
            he::action{ [&otherwise_ran] (const he::action::context&)
            {
                otherwise_ran = true;
                return true;
            } });

        auto graph{ std::make_shared<he::exec::task_graph>() };
        graph->activate(root.translate_into_graph(graph->root()).begin);

        REQUIRE(otherwise_ran);
    }

    SECTION("then skipped on failure")
    {
        auto then_ran{ false };

        auto root{ he::action{ [] (const he::action::context&) { return false; } } };

        root.then(
            he::action{ [&then_ran] (const he::action::context&)
            {
                then_ran = true;
                return true;
            } });

        auto graph{ std::make_shared<he::exec::task_graph>() };
        graph->activate(root.translate_into_graph(graph->root()).begin);

        REQUIRE_FALSE(then_ran);
    }

    SECTION("otherwise skipped on success")
    {
        auto otherwise_ran{ false };

        auto root{ he::action{ [] (const he::action::context&) { return true; } } };

        root.otherwise(
            he::action{ [&otherwise_ran] (const he::action::context&)
            {
                otherwise_ran = true;
                return true;
            } });

        auto graph{ std::make_shared<he::exec::task_graph>() };
        graph->activate(root.translate_into_graph(graph->root()).begin);

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

        auto graph{ std::make_shared<he::exec::task_graph>() };
        graph->activate(root.translate_into_graph(graph->root()).begin);

        REQUIRE(then_ran);
        REQUIRE_FALSE(otherwise_ran);
    }

    SECTION("nested chain in order")
    {
        auto order{ std::string{} };

        auto graph{ std::make_shared<he::exec::task_graph>() };

        graph->activate(
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
            .translate_into_graph(graph->root()).begin);

        REQUIRE(order == "abc");
    }

    SECTION("preserves subclass override")
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

        auto graph{ std::make_shared<he::exec::task_graph>() };

        graph->activate(
            he::action{ [] (const he::action::context&) { return true; } }
            .then(custom_action{})
            .translate_into_graph(graph->root()).begin);

        REQUIRE(custom_execute_ran);
    }

    SECTION("context propagates to branch")
    {
        auto received{ false };

        auto context{ he::action::context{ { "flag", true } } };

        auto graph{ std::make_shared<he::exec::task_graph>() };

        graph->activate(
            he::action{ [] (const he::action::context&) { return true; }, std::move(context) }
            .then(
                he::action{
                    [&received] (const he::action::context& ctx)
                    {
                        received = std::any_cast<bool>(ctx.at("flag"));
                        return true;
                    }
                })
            .translate_into_graph(graph->root()).begin);

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
