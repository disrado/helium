#include "sequential_composite.hpp"


namespace he
{

auto sequential_composite::setup_node(exec::task_node& self_node) -> exec::task_node&
{
    auto& completion_node{ self_node.add_child() };

    auto* const first_entry{ setup_sequence(self_node, completion_node) };

    self_node.post_execution.bind(
        [
            self{ std::static_pointer_cast<sequential_composite>(shared_from_this()) },
            &self_node,
            first_entry,
            &completion_node
        ] (exec::execution_status)
        {
            self->on_action_finished(self_node, first_entry, completion_node);
        });

    return completion_node;
}


auto sequential_composite::setup_sequence(exec::task_node& self_node, exec::task_node& completion_node) -> exec::task_node*
{
    auto segments{ translate_steps(self_node) };

    for (std::size_t i{ 0 }; i < segments.size(); ++i)
    {
        auto* const next_segment_start{ i + 1 < segments.size() ? &segments[i + 1].start : nullptr };

        link_steps(segments[i], self_node, next_segment_start, completion_node);
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


auto sequential_composite::link_steps(
    const exec::graph_segment& step,
    exec::task_node& self_node,
    exec::task_node* next_segment_start,
    exec::task_node& completion_node) -> void
{
    auto* const step_start{ &step.start };

    step.end.post_execution.bind(
        [this, &self_node, step_start, next_segment_start, &completion_node] (exec::execution_status)
        {
            on_step_finished(self_node, step_start, next_segment_start, completion_node);
        });
}


auto sequential_composite::on_action_finished(
    exec::task_node& self_node,
    exec::task_node* first_entry,
    const exec::task_node& completion_node) -> void
{
    if (self_node.cancel_requested)
    {
        self_node.state = exec::action_state::cancelled;

        std::ignore = completion_node.post_execution.execute(exec::execution_status::completed);

        return;
    }

    first_entry->set_context(self_node.get_context());
    first_entry->activate();
}


auto sequential_composite::on_step_finished(
    exec::task_node& self_node,
    exec::task_node* step_start,
    exec::task_node* next_segment_start,
    const exec::task_node& completion_node) -> void
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

    if (step_start->state == exec::action_state::succeeded && next_segment_start)
    {
        next_segment_start->set_context(step_start->get_context());
        next_segment_start->activate();

        return;
    }

    self_node.state = step_start->state;

    resolve_link(self_node, step_start);

    std::ignore = completion_node.post_execution.execute(exec::execution_status::completed);
}


auto sequential_composite::resolve_link(exec::task_node& self_node, exec::task_node* step_start) -> void
{
    for (auto& entry : self_node.links)
    {
        if (entry.condition.try_execute(self_node.state).value_or(false))
        {
            entry.target->set_context(step_start->get_context());
            entry.target->activate();

            break;
        }
    }
}

}
