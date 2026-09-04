#include "action.hpp"


namespace he
{

auto action::translate_into_graph(exec::task_node& parent) -> exec::graph_segment
{
    auto& self_node{ parent.add_child() };

    self_node.mode = exec::launch_policy::sync;
    self_node.definition = exec::task_definition{
        [self{ shared_from_this() }, &self_node] (std::stop_token token) { self->execute(self_node, std::move(token)); }
    };

    self_node.then_node = _then_action ? &_then_action->translate_into_graph(self_node).start : nullptr;
    self_node.else_node = _else_action ? &_else_action->translate_into_graph(self_node).start : nullptr;

    self_node.post_execution.bind(
        [&self_node] (exec::execution_status)
        {
            if (self_node.cancel_requested)
            {
                self_node.state = exec::action_state::cancelled;

                return;
            }

            if (self_node.state == exec::action_state::succeeded && self_node.then_node)
            {
                self_node.then_node->set_context(self_node.get_context());
                self_node.then_node->activate();
            }
            else if (self_node.state == exec::action_state::failed && self_node.else_node)
            {
                self_node.else_node->set_context(self_node.get_context());
                self_node.else_node->activate();
            }
        });

    return exec::graph_segment{ .start{ self_node }, .end{ self_node } };
}

}
