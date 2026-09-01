#include "action.hpp"

#include "core/execution/scheduler.hpp"


namespace he
{

auto action::expand_on_graph(exec::task_graph::node& parent) -> exec::graph_segment
{
    auto& self_node{ parent.add_child() };

    self_node.definition = exec::task_definition{ [this] (std::stop_token token) { execute(std::move(token)); } };

    self_node.post_condition.bind(
        [
            this,
            then_child{ _then_action ? &_then_action->translate_into_graph(self_node).begin : nullptr },
            else_child{ _else_action ? &_else_action->translate_into_graph(self_node).begin : nullptr }
        ]
        (exec::execution_status)
        {
            if (_cancel_requested)
            {
                set_state(state::cancelled);

                return;
            }

            if (get_state() == state::succeeded && then_child)
            {
                _then_action->set_context(_context);

                then_child->activate();
            }
            else if (get_state() == state::failed && else_child)
            {
                _else_action->set_context(_context);

                else_child->activate();
            }
        });

    _graph_segment.emplace(exec::graph_segment{ .begin{ self_node }, .end{ self_node } });

    return exec::graph_segment{ .begin{ self_node }, .end{ self_node } };
}


auto action::cancel() -> void
{
    _cancel_requested = true;

    if (_graph_segment.has_value() && _graph_segment.value().begin.id != exec::invalid_task_id)
    {
        exec::scheduler::instance().cancel(_graph_segment.value().begin.id);

        return;
    }

    set_state(state::cancelled);
}

}
