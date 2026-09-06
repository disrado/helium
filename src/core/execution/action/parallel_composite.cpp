#include "parallel_composite.hpp"


namespace he
{

auto parallel_composite::setup_node(exec::task_node& self_node) -> exec::task_node&
{
    auto& join_node{ self_node.add_child() };

    auto branches{ translate_steps(self_node) };

    auto state{ std::make_shared<join_state>() };
    state->pending = branches.size();
    state->step_starts.reserve(branches.size());

    for (auto& branch : branches)
    {
        state->step_starts.push_back(&branch.start);
    }

    for (auto& branch : branches)
    {
        setup_branch_node(self_node, join_node, branch, state);
    }

    join_node.post_execution.bind([&join_node] (exec::execution_status) { join_node.resolve_links(); });

    self_node.post_execution.bind(
        [&self_node, &join_node, branches] (exec::execution_status)
        {
            if (self_node.cancel_requested)
            {
                self_node.state = exec::action_state::cancelled;
                join_node.state = exec::action_state::cancelled;

                std::ignore = join_node.post_execution.execute(exec::execution_status::completed);

                return;
            }

            for (auto& branch : branches)
            {
                branch.start.set_context(self_node.get_context());
                branch.start.activate();
            }
        });

    return join_node;
}


auto parallel_composite::translate_steps(exec::task_node& self_node) -> std::vector<exec::graph_segment>
{
    auto segments{ std::vector<exec::graph_segment>{} };
    segments.reserve(_steps.size());

    for (const auto& step : _steps)
    {
        segments.push_back(step->translate_into_graph(self_node));
    }

    return segments;
}


auto parallel_composite::setup_branch_node(
    exec::task_node& self_node,
    exec::task_node& join_node,
    const exec::graph_segment& branch,
    const std::shared_ptr<join_state>& state) -> void
{
    branch.end.post_execution.bind(
        [&self_node, &join_node, branch_start{ &branch.start }, state] (exec::execution_status)
        {
            if (!self_node.cancel_requested && branch_start->state != exec::action_state::succeeded)
            {
                state->any_failed = true;
            }

            if (--state->pending != 0)
            {
                return;
            }

            if (self_node.cancel_requested)
            {
                self_node.state = exec::action_state::cancelled;
                join_node.state = exec::action_state::cancelled;

                std::ignore = join_node.post_execution.execute(exec::execution_status::completed);
            }
            else
            {
                resolve_join(self_node, join_node, *state);
            }
        });
}


auto parallel_composite::resolve_join(exec::task_node& self_node, exec::task_node& join_node, const join_state& state) -> void
{
    self_node.state = state.any_failed ? exec::action_state::failed : exec::action_state::succeeded;
    join_node.state = self_node.state;

    for (auto* begin : state.step_starts)
    {
        join_node.merge_context(begin->get_context());
    }

    std::ignore = join_node.post_execution.execute(exec::execution_status::completed);
}

}
