#include "sequential_composite.hpp"


namespace
{

auto to_task_result(he::exec::action_state state) -> he::exec::task_result
{
    switch (state)
    {
        case he::exec::action_state::succeeded: return he::exec::task_result::succeeded;
        case he::exec::action_state::cancelled: return he::exec::task_result::cancelled;
        default: return he::exec::task_result::failed;
    }
}

}


namespace he
{

auto sequential_composite::setup_node(exec::task_node& self_node) -> exec::task_node&
{
    auto& completion_node{ self_node.add_child() };

    auto segments{ translate_steps(self_node) };

    for (std::size_t i{ 0 }; i < segments.size(); ++i)
    {
        auto* const next_step_start{ i + 1 < segments.size() ? &segments[i + 1].start : nullptr };

        setup_step_node(self_node, completion_node, segments[i], next_step_start);
    }

    completion_node.post_execution.bind([&completion_node] (exec::task_result) { completion_node.resolve_links(); });

    self_node.post_execution.bind(
        [&self_node, first{ &segments.front().start }] (exec::task_result)
        {
            if (self_node.cancel_requested)
            {
                self_node.state = exec::action_state::cancelled;

                return;
            }

            first->set_context(self_node.get_context());
            first->activate();
        });

    return completion_node;
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


auto sequential_composite::setup_step_node(
    exec::task_node& self_node,
    exec::task_node& completion_node,
    const exec::graph_segment& step,
    exec::task_node* next_step_start) -> void
{
    step.end.post_execution.bind(
        [&self_node, &completion_node, step_start{ &step.start }, next_step_start] (exec::task_result)
        {
            if (self_node.cancel_requested)
            {
                self_node.state = exec::action_state::cancelled;
                completion_node.state = exec::action_state::cancelled;

                std::ignore = completion_node.post_execution.execute(exec::task_result::cancelled);

                return;
            }

            const auto step_result{ step_start->state.load() };

            if (next_step_start && step_result == exec::action_state::succeeded)
            {
                next_step_start->set_context(step_start->get_context());
                next_step_start->activate();
            }
            else
            {
                self_node.state = step_result;
                completion_node.state = step_result;
                completion_node.set_context(step_start->get_context());

                std::ignore = completion_node.post_execution.execute(to_task_result(step_result));
            }
        });
}

}
