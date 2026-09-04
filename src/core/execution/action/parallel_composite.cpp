#include "parallel_composite.hpp"

#include <memory>
#include <tuple>


namespace he
{

namespace
{

struct join_state final
{
public:
    std::size_t pending;
    bool any_failed{ false };
    std::vector<exec::task_node*> step_starts;
};


auto resolve_join(exec::task_node& self_node, exec::task_node& join_node, const join_state& state) -> void
{
    self_node.state = state.any_failed ? exec::action_state::failed : exec::action_state::succeeded;

    if (auto* const target{ state.any_failed ? self_node.else_node : self_node.then_node })
    {
        for (auto* begin : state.step_starts)
        {
            target->merge_context(begin->get_context());
        }

        target->activate();
    }

    std::ignore = join_node.post_execution.execute(exec::execution_status::completed);
}

}


auto parallel_composite::translate_into_graph(exec::task_node& parent) -> exec::graph_segment
{
    auto& self_node{ parent.add_child() };
    auto& join_node{ self_node.add_child() };

    self_node.then_node = _then_action ? &_then_action->translate_into_graph(self_node).start : nullptr;
    self_node.else_node = _else_action ? &_else_action->translate_into_graph(self_node).start : nullptr;

    setup_join(self_node, join_node);

    return exec::graph_segment{ .start{ self_node }, .end{ join_node } };
}


auto parallel_composite::setup_join(exec::task_node& self_node, exec::task_node& join_node) -> void
{
    auto entries{ std::vector<exec::graph_segment>{} };
    entries.reserve(_steps.size());

    for (auto& step : _steps)
    {
        entries.push_back(step->translate_into_graph(self_node));
    }

    auto state{ std::make_shared<join_state>() };
    state->pending = _steps.size();
    state->step_starts.reserve(entries.size());

    for (auto& entry : entries)
    {
        state->step_starts.push_back(&entry.start);
    }

    for (std::size_t i{ 0 }; i < _steps.size(); ++i)
    {
        auto* const current_begin{ &entries[i].start };

        entries[i].end.post_execution.bind(
            [&self_node, current_begin, &join_node, state] (exec::execution_status)
            {
                if (self_node.state == exec::action_state::cancelled)
                {
                    return;
                }

                if (!self_node.cancel_requested && current_begin->state != exec::action_state::succeeded)
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

                    std::ignore = join_node.post_execution.execute(exec::execution_status::completed);
                }
                else
                {
                    resolve_join(self_node, join_node, *state);
                }
            });
    }

    self_node.post_execution.bind(
        [self{ std::static_pointer_cast<parallel_composite>(shared_from_this()) }, &self_node, entries, &join_node] (exec::execution_status)
        {
            if (self_node.cancel_requested)
            {
                self_node.state = exec::action_state::cancelled;

                std::ignore = join_node.post_execution.execute(exec::execution_status::completed);

                return;
            }

            for (std::size_t i{ 0 }; i < self->_steps.size(); ++i)
            {
                entries[i].start.set_context(self_node.get_context());
                entries[i].start.activate();
            }
        });
}

}
