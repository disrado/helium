#include "async_action.hpp"


namespace he
{

auto async_action::translate_into_graph(exec::task_graph::node& parent) -> exec::graph_segment
{
    auto& self_node{ parent.add_child() };

    self_node.mode = exec::launch_policy::async;
    self_node.definition = exec::task_definition{ [this, &self_node] (std::stop_token token) { execute(self_node, std::move(token)); } };

    auto* const then_child{ _then_action ? &_then_action->translate_into_graph(self_node).begin : nullptr };
    auto* const else_child{ _else_action ? &_else_action->translate_into_graph(self_node).begin : nullptr };

    self_node.post_condition.bind(
        [&self_node, then_child, else_child]
        (exec::execution_status)
        {
            if (self_node.cancel_requested)
            {
                self_node.state = exec::action_state::cancelled;

                return;
            }

            if (self_node.state == exec::action_state::succeeded && then_child)
            {
                then_child->context = self_node.context;

                then_child->activate();
            }

            if (self_node.state == exec::action_state::failed && else_child)
            {
                else_child->context = self_node.context;

                else_child->activate();
            }
        });

    return exec::graph_segment{ .begin{ self_node }, .end{ self_node } };
}

}
