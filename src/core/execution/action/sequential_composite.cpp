#include "sequential_composite.hpp"


namespace he
{

auto sequential_composite::translate_into_graph(exec::task_node& parent) -> exec::graph_segment
{
    auto& self_node{ parent.add_child() };
    auto& completion_node{ self_node.add_child() };

    self_node.then_node = _then_action ? &_then_action->translate_into_graph(self_node).start : nullptr;
    self_node.else_node = _else_action ? &_else_action->translate_into_graph(self_node).start : nullptr;

    auto* const first_entry{ setup_sequence(self_node, completion_node) };

    self_node.post_execution.bind(
        [self{ shared_from_this() }, &self_node, first_entry, &completion_node] (exec::execution_status)
        {
            // unused but required in capture list for lifetime prolongation
            std::ignore = self;

            if (self_node.cancel_requested)
            {
                self_node.state = exec::action_state::cancelled;

                std::ignore = completion_node.post_execution.execute(exec::execution_status::completed);

                return;
            }

            first_entry->set_context(self_node.get_context());

            first_entry->activate();
        });

    return exec::graph_segment{ .start{ self_node }, .end{ completion_node } };
}


auto sequential_composite::setup_sequence(
    exec::task_node& self_node,
    exec::task_node& completion_node) -> exec::task_node*
{
    auto segments{ translate_steps(self_node) };

    for (std::size_t i{ 0 }; i < segments.size(); ++i)
    {
        auto* const next_segment_start{ i + 1 < segments.size() ? &segments[i + 1].start : nullptr };

        bind_step_completion(segments[i], self_node, next_segment_start, completion_node);
    }

    return &segments.front().start;
}


auto sequential_composite::translate_steps(exec::task_node& self_node) -> std::vector<exec::graph_segment>
{
    auto segments{ std::vector<exec::graph_segment>{} };
    segments.reserve(_steps.size());

    for (const auto& step : _steps)
    {
        segments.push_back(step->translate_into_graph(self_node));
    }

    return segments;
}


auto sequential_composite::bind_step_completion(
    const exec::graph_segment& step,
    exec::task_node& self_node,
    exec::task_node* next_segment_start,
    exec::task_node& completion_node) -> void
{
    auto* const step_start{ &step.start };

    step.end.post_execution.bind(
        [&self_node, step_start, next_segment_start, &completion_node] (exec::execution_status)
        {
            if (self_node.state == exec::action_state::cancelled)
            {
                return;
            }

            if (self_node.cancel_requested)
            {
                self_node.state = exec::action_state::cancelled;

                std::ignore = completion_node.post_execution.execute(exec::execution_status::completed);

                return;
            }

            const auto advance_to{
                [step_start] (exec::task_node* target)
                {
                    if (target)
                    {
                        target->set_context(step_start->get_context());
                        target->activate();
                    }
                }
            };

            if (step_start->state == exec::action_state::succeeded && next_segment_start)
            {
                advance_to(next_segment_start);

                return;
            }

            if (step_start->state == exec::action_state::succeeded)
            {
                self_node.state = exec::action_state::succeeded;

                advance_to(self_node.then_node);

                std::ignore = completion_node.post_execution.execute(exec::execution_status::completed);
            }
            else if (step_start->state == exec::action_state::failed)
            {
                self_node.state = exec::action_state::failed;

                advance_to(self_node.else_node);

                std::ignore = completion_node.post_execution.execute(exec::execution_status::completed);
            }
        });
}

}
