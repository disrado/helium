#include "core/execution/action/action.hpp"
#include "core/execution/task_graph.hpp"

#include <catch2/catch_test_macros.hpp>


TEST_CASE("action")
{
    SECTION("build_graph adds itself as a child of parent")
    {
        auto instance{ he::action{ [] (const he::action::context&) { return true; } } };

        auto graph{ he::exec::task_graph{} };
        auto& node{ instance.build_graph(graph.root()) };

        REQUIRE(graph.root().children().size() == 1);
        REQUIRE(graph.root().children().front().get() == &node);
    }

    SECTION("build_graph runs the definition when activated")
    {
        auto ran{ false };

        auto instance{
            he::action{ [&ran] (const he::action::context&)
            {
                ran = true;
                return true;
            } }
        };

        auto graph{ he::exec::task_graph{} };
        graph.activate(instance.build_graph(graph.root()));

        REQUIRE(ran);
        REQUIRE(instance.get_state() == he::action::state::succeeded);
    }

    SECTION("build_graph reports failure")
    {
        auto instance{ he::action{ [] (const he::action::context&) { return false; } } };

        auto graph{ he::exec::task_graph{} };
        graph.activate(instance.build_graph(graph.root()));

        REQUIRE(instance.get_state() == he::action::state::failed);
    }
}
