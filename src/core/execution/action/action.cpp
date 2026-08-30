#include "action.hpp"


namespace he
{

auto action::expand_on_graph(exec::task_graph::node& parent) -> exec::task_graph::node&
{
    auto& self_node{ parent.add_child() };

    self_node.definition = exec::task_definition{ [this] (std::stop_token) { execute(); } };

    self_node.post_condition.bind(
        [
            this,
            then_child{ _then_action ? &_then_action->translate_into_graph(self_node) : nullptr },
            else_child{ _else_action ? &_else_action->translate_into_graph(self_node) : nullptr }
        ]
        {
            if (get_state() == state::succeeded && then_child)
            {
                _then_action->set_context(std::move(_context));

                then_child->activate();
            }
            else if (get_state() == state::failed && else_child)
            {
                _else_action->set_context(std::move(_context));

                else_child->activate();
            }
        });

    return self_node;
}

}
