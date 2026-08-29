#include "async_action.hpp"


namespace he
{

auto async_action::build_graph(exec::task_graph::node& parent) -> exec::task_graph::node&
{
    auto& self_node{ parent.add_child() };

    self_node.mode = exec::launch_policy::async;
    self_node.definition = exec::task_definition{ [this] (std::stop_token) { execute(); } };

    exec::task_graph::node* then_child{ nullptr };
    exec::task_graph::node* else_child{ nullptr };

    if (_then_action)
    {
        then_child = &_then_action->build_graph(self_node);
    }

    if (_else_action)
    {
        else_child = &_else_action->build_graph(self_node);
    }

    self_node.post_condition.bind(
        [this, then_child, else_child]
        {
            if (_state == state::succeeded && then_child)
            {
                propagate_context_to(*_then_action);

                then_child->activate();
            }

            if (_state == state::failed && else_child)
            {
                propagate_context_to(*_else_action);

                else_child->activate();
            }
        });

    return self_node;
}

}
