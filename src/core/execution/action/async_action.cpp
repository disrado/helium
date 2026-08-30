#include "async_action.hpp"


namespace he
{

auto async_action::expand_on_graph(exec::task_graph::node& parent) -> exec::graph_segment
{
    auto& self_node{ parent.add_child() };

    self_node.mode = exec::launch_policy::async;
    self_node.definition = exec::task_definition{ [this] (std::stop_token) { execute(); } };

    auto* const then_child{ _then_action ? &_then_action->translate_into_graph(self_node).begin : nullptr };
    auto* const else_child{ _else_action ? &_else_action->translate_into_graph(self_node).begin : nullptr };

    self_node.post_condition.bind(
        [this, then_child, else_child]
        {
            if (_state == state::succeeded && then_child)
            {
                _then_action->set_context(_context);

                then_child->activate();
            }

            if (_state == state::failed && else_child)
            {
                _else_action->set_context(_context);

                else_child->activate();
            }
        });

    return exec::graph_segment{ .begin{ self_node }, .end{ self_node } };
}

}
