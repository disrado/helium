#include "sequential_composite.hpp"


namespace he
{

auto sequential_composite::expand_on_graph(exec::task_graph::node& parent) -> exec::task_graph::node&
{
    auto& self_node{ parent.add_child() };

    const auto chain{ setup_sequence(self_node) };

    setup_completion(self_node, chain);

    self_node.post_condition.bind(
        [this, first_step{ _steps.front().get() }, first_entry{ chain.first }]
        {
            if (first_entry)
            {
                if (get_context().has_value())
                {
                    first_step->set_context(get_context());
                }

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
        auto& step_entry{ step->translate_into_graph(self_node) };

        if (!sequence.first)
        {
            sequence.first = &step_entry;
        }

        if (sequence.last_action)
        {
            auto* previous_step{ sequence.last_action };

            // keyed on state, not post_condition: a composite's node fires post_condition on
            // activation (just its kickoff), state only becomes succeeded/failed once truly done
            previous_step->on(
                state::succeeded,
                [previous_step, current_step{ step.get() }, &step_entry]
                {
                    current_step->set_context(previous_step->get_context());

                    step_entry.activate();
                });
        }

        sequence.last_action = step.get();
    }

    return sequence;
}


auto sequential_composite::setup_completion(exec::task_graph::node& self_node, const graph_sequence& sequence) -> void
{
    if (!sequence.last_action)
    {
        return;
    }

    auto* last_step{ sequence.last_action };

    auto* then_child{ _then_action ? &_then_action->translate_into_graph(self_node) : nullptr };
    auto* else_child{ _else_action ? &_else_action->translate_into_graph(self_node) : nullptr };

    last_step->on(
        state::succeeded,
        [this, last_step, then_child]
        {
            set_state(state::succeeded);

            if (then_child)
            {
                _then_action->set_context(last_step->get_context());

                then_child->activate();
            }
        });

    last_step->on(
        state::failed,
        [this, last_step, else_child]
        {
            set_state(state::failed);

            if (else_child)
            {
                _else_action->set_context(last_step->get_context());

                else_child->activate();
            }
        });
}

}
