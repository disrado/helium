#include "sequential_composite.hpp"


namespace he
{

auto sequential_composite::translate_into_graph(exec::task_graph::node& parent) -> exec::graph_segment
{
    auto& self_node{ parent.add_child() };
    auto& completion_node{ self_node.add_child() };

    auto* const then_child{ _then_action ? &_then_action->translate_into_graph(self_node).begin : nullptr };
    auto* const else_child{ _else_action ? &_else_action->translate_into_graph(self_node).begin : nullptr };

    auto* const first_entry{ setup_sequence(self_node, completion_node, then_child, else_child) };

    self_node.post_condition.bind(
        [&self_node, first_entry, &completion_node]
        (exec::execution_status)
        {
            if (self_node.cancel_requested)
            {
                self_node.state = exec::action_state::cancelled;

                std::ignore = completion_node.post_condition.execute(exec::execution_status::completed);

                return;
            }

            first_entry->context = self_node.context;

            first_entry->activate();
        });

    return exec::graph_segment{ .begin{ self_node }, .end{ completion_node } };
}


auto sequential_composite::setup_sequence(
    exec::task_graph::node& self_node,
    exec::task_graph::node& completion_node,
    exec::task_graph::node* then_child,
    exec::task_graph::node* else_child) -> exec::task_graph::node*
{
    auto entries{ std::vector<exec::graph_segment>{} };
    entries.reserve(_steps.size());

    for (auto& step: _steps)
    {
        entries.push_back(step->translate_into_graph(self_node));
    }

    for (std::size_t i{ 0 }; i < _steps.size(); ++i)
    {
        auto* const current_begin{ &entries[i].begin };
        auto* const next_begin{ i + 1 < _steps.size() ? &entries[i + 1].begin : nullptr };

        entries[i].end.post_condition.bind(
            [&self_node, current_begin, next_begin, then_child, else_child, &completion_node]
            (exec::execution_status)
            {
                if (self_node.state == exec::action_state::cancelled)
                {
                    return;
                }

                if (self_node.cancel_requested)
                {
                    self_node.state = exec::action_state::cancelled;

                    std::ignore = completion_node.post_condition.execute(exec::execution_status::completed);

                    return;
                }

                if (current_begin->state == exec::action_state::succeeded)
                {
                    if (next_begin)
                    {
                        next_begin->context = current_begin->context;

                        next_begin->activate();
                    }
                    else
                    {
                        self_node.state = exec::action_state::succeeded;

                        if (then_child)
                        {
                            then_child->context = current_begin->context;

                            then_child->activate();
                        }

                        std::ignore = completion_node.post_condition.execute(exec::execution_status::completed);
                    }
                }
                else if (current_begin->state == exec::action_state::failed)
                {
                    self_node.state = exec::action_state::failed;

                    if (else_child)
                    {
                        else_child->context = current_begin->context;

                        else_child->activate();
                    }

                    std::ignore = completion_node.post_condition.execute(exec::execution_status::completed);
                }
            });
    }

    return &entries.front().begin;
}

}
