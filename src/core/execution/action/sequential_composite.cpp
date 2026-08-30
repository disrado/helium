#include "sequential_composite.hpp"


namespace he
{

auto sequential_composite::build_graph(exec::task_graph::node& parent) -> exec::task_graph::node&
{
    auto& self_node{ parent.add_child() };

    const auto chain{ setup_sequence(self_node) };

    setup_completion(self_node, chain);

    self_node.post_condition.bind(
        [first_entry{ chain.first }]
        {
            if (first_entry)
            {
                first_entry->activate();
            }
        });

    return self_node;
}


auto sequential_composite::setup_sequence(exec::task_graph::node& self_node) -> graph_sequence
{
    auto sequence{ graph_sequence{} };

    for (auto& step: _steps)
    {
        auto& step_entry{ step->build_graph(self_node) };

        if (!sequence.first)
        {
            sequence.first = &step_entry;
        }

        if (sequence.last)
        {
            auto* previous_step{ sequence.last_action };

            sequence.last->post_condition.bind(
                [previous_step, current_step{ step.get() }, &step_entry]
                {
                    if (previous_step->get_state() == state::succeeded)
                    {
                        current_step->set_context(previous_step->get_context());

                        step_entry.activate();
                    }
                });
        }

        sequence.last = &step_entry;
        sequence.last_action = step.get();
    }

    return sequence;
}


auto sequential_composite::setup_completion(exec::task_graph::node& self_node, const graph_sequence& sequence) -> void
{
    if (!sequence.last)
    {
        return;
    }

    sequence.last->post_condition.bind(
        [
            this,
            last_step{ sequence.last_action },
            then_child{ _then_action ? &_then_action->build_graph(self_node) : nullptr },
            else_child{ _else_action ? &_else_action->build_graph(self_node) : nullptr }
        ]
        {
            set_state(last_step->get_state());

            if (get_state() == state::succeeded && then_child)
            {
                _then_action->set_context(last_step->get_context());

                then_child->activate();
            }
            else if (get_state() == state::failed && else_child)
            {
                _else_action->set_context(last_step->get_context());

                else_child->activate();
            }
        });
}

}
