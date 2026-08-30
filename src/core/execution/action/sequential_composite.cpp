#include "sequential_composite.hpp"


namespace he
{

auto sequential_composite::expand_on_graph(exec::task_graph::node& parent) -> exec::graph_segment
{
    auto& self_node{ parent.add_child() };

    auto* const then_child{ _then_action ? &_then_action->translate_into_graph(self_node).begin : nullptr };
    auto* const else_child{ _else_action ? &_else_action->translate_into_graph(self_node).begin : nullptr };

    auto sequence{ setup_sequence(self_node, then_child, else_child) };

    self_node.post_condition.bind(
        [this, first_step{ _steps.front().get() }, first_entry{ &sequence.begin }]
        (exec::execution_status)
        {
            if (get_context().has_value())
            {
                first_step->set_context(get_context());
            }

            first_entry->activate();
        });

    return exec::graph_segment{ .begin{ self_node }, .end{ sequence.end } };
}


auto sequential_composite::setup_sequence(
    exec::task_graph::node& self_node,
    exec::task_graph::node* then_child,
    exec::task_graph::node* else_child) -> exec::graph_segment
{
    auto entries{ std::vector<exec::graph_segment>{} };
    entries.reserve(_steps.size());

    for (auto& step: _steps)
    {
        entries.push_back(step->translate_into_graph(self_node));
    }

    for (std::size_t i{ 0 }; i < _steps.size(); ++i)
    {
        auto* const current_step{ _steps[i].get() };
        auto* const next_begin{ i + 1 < _steps.size() ? &entries[i + 1].begin : nullptr };
        auto* const next_step{ i + 1 < _steps.size() ? _steps[i + 1].get() : nullptr };

        entries[i].end.post_condition.bind(
            [this, current_step, next_begin, next_step, then_child, else_child]
            (exec::execution_status)
            {
                if (current_step->get_state() == state::succeeded)
                {
                    if (next_begin)
                    {
                        next_step->set_context(current_step->get_context());

                        next_begin->activate();
                    }
                    else
                    {
                        set_state(state::succeeded);

                        if (then_child)
                        {
                            _then_action->set_context(current_step->get_context());

                            then_child->activate();
                        }
                    }
                }
                else if (current_step->get_state() == state::failed)
                {
                    set_state(state::failed);

                    if (else_child)
                    {
                        _else_action->set_context(current_step->get_context());

                        else_child->activate();
                    }
                }
            });
    }

    return exec::graph_segment{ .begin{ entries.front().begin }, .end{ entries.back().end } };
}

}
