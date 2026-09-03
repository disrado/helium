#include "parallel_composite.hpp"

#include <cstddef>
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
    std::vector<exec::task_graph::node*> step_begins;
};


auto resolve_join(
    exec::task_graph::node& self_node,
    exec::task_graph::node& join_node,
    const join_state& state,
    exec::task_graph::node* then_child,
    exec::task_graph::node* else_child) -> void
{
    self_node.state = state.any_failed ? exec::action_state::failed : exec::action_state::succeeded;

    auto merged{ exec::action_context{} };

    for (auto* begin: state.step_begins)
    {
        if (begin->context.has_value())
        {
            for (auto& [key, value]: begin->context.value())
            {
                merged[key] = value;
            }
        }
    }

    if (state.any_failed && else_child)
    {
        else_child->context = merged;

        else_child->activate();
    }
    else if (!state.any_failed && then_child)
    {
        then_child->context = merged;

        then_child->activate();
    }

    std::ignore = join_node.post_condition.execute(exec::execution_status::completed);
}

}


auto parallel_composite::translate_into_graph(exec::task_graph::node& parent) -> exec::graph_segment
{
    auto& self_node{ parent.add_child() };
    auto& join_node{ self_node.add_child() };

    auto* const then_child{ _then_action ? &_then_action->translate_into_graph(self_node).begin : nullptr };
    auto* const else_child{ _else_action ? &_else_action->translate_into_graph(self_node).begin : nullptr };

    setup_join(self_node, join_node, then_child, else_child);

    return exec::graph_segment{ .begin{ self_node }, .end{ join_node } };
}


auto parallel_composite::setup_join(
    exec::task_graph::node& self_node,
    exec::task_graph::node& join_node,
    exec::task_graph::node* then_child,
    exec::task_graph::node* else_child) -> void
{
    auto entries{ std::vector<exec::graph_segment>{} };
    entries.reserve(_steps.size());

    for (auto& step: _steps)
    {
        entries.push_back(step->translate_into_graph(self_node));
    }

    auto state{ std::make_shared<join_state>() };
    state->pending = _steps.size();
    state->step_begins.reserve(entries.size());

    for (auto& entry: entries)
    {
        state->step_begins.push_back(&entry.begin);
    }

    for (std::size_t i{ 0 }; i < _steps.size(); ++i)
    {
        auto* const current_begin{ &entries[i].begin };

        entries[i].end.post_condition.bind(
            [&self_node, current_begin, &join_node, then_child, else_child, state]
            (exec::execution_status)
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

                    std::ignore = join_node.post_condition.execute(exec::execution_status::completed);
                }
                else
                {
                    resolve_join(self_node, join_node, *state, then_child, else_child);
                }
            });
    }

    self_node.post_condition.bind(
        [this, &self_node, entries, &join_node]
        (exec::execution_status)
        {
            if (self_node.cancel_requested)
            {
                self_node.state = exec::action_state::cancelled;

                std::ignore = join_node.post_condition.execute(exec::execution_status::completed);

                return;
            }

            for (std::size_t i{ 0 }; i < _steps.size(); ++i)
            {
                entries[i].begin.context = self_node.context;

                entries[i].begin.activate();
            }
        });
}

}
