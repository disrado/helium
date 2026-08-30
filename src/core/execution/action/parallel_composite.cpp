#include "parallel_composite.hpp"

#include <tuple>


namespace he
{

auto parallel_composite::abort() -> void
{
    for (auto& step: _steps)
    {
        step->abort();
    }

    basic_action::abort();
}


auto parallel_composite::expand_on_graph(exec::task_graph::node& parent) -> exec::graph_segment
{
    auto& self_node{ parent.add_child() };
    auto& join_node{ self_node.add_child() };

    auto* const then_child{ _then_action ? &_then_action->translate_into_graph(self_node).begin : nullptr };
    auto* const else_child{ _else_action ? &_else_action->translate_into_graph(self_node).begin : nullptr };

    _pending = _steps.size();
    _any_failed = false;

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

    for (std::size_t i{ 0 }; i < _steps.size(); ++i)
    {
        auto* const current_step{ _steps[i].get() };

        entries[i].end.post_condition.bind(
            [this, current_step, &join_node, then_child, else_child] (exec::execution_status)
            {
                if (current_step->get_state() != state::succeeded)
                {
                    _any_failed = true;
                }

                if (--_pending == 0)
                {
                    resolve(join_node, then_child, else_child);
                }
            });
    }

    self_node.post_condition.bind(
        [this, entries] (exec::execution_status)
        {
            for (std::size_t i{ 0 }; i < _steps.size(); ++i)
            {
                if (get_context().has_value())
                {
                    _steps[i]->set_context(get_context());
                }

                entries[i].begin.activate();
            }
        });
}


auto parallel_composite::resolve(
    const exec::task_graph::node& join_node,
    exec::task_graph::node* then_child,
    exec::task_graph::node* else_child) -> void
{
    set_state(_any_failed ? state::failed : state::succeeded);

    auto merged{ context{} };

    for (auto& step: _steps)
    {
        if (const auto& ctx{ step->get_context() }; ctx.has_value())
        {
            for (auto& [key, value]: ctx.value())
            {
                merged[key] = value;
            }
        }
    }

    if (_any_failed && else_child)
    {
        _else_action->set_context(merged);
        else_child->activate();
    }
    else if (!_any_failed && then_child)
    {
        _then_action->set_context(merged);
        then_child->activate();
    }

    std::ignore = join_node.post_condition.execute(exec::execution_status::completed);
}

}
