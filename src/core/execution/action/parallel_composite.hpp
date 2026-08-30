#pragma once

#include "core/execution/action/action_base.hpp"

#include <cstddef>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>


namespace he
{

class parallel_composite final: public exec::action_base<parallel_composite>
{
public:
    template <typename... action_ts>
        requires (sizeof...(action_ts) > 0) && (exec::action_like<std::decay_t<action_ts>> && ...)
    explicit parallel_composite(action_ts&&... steps);

    auto abort() -> void override;

protected:
    auto expand_on_graph(exec::task_graph::node& parent) -> exec::graph_segment override;

private:
    auto setup_join(
        exec::task_graph::node& self_node,
        exec::task_graph::node& join_node,
        exec::task_graph::node* then_child,
        exec::task_graph::node* else_child) -> void;

    auto resolve(const exec::task_graph::node& join_node, exec::task_graph::node* then_child, exec::task_graph::node* else_child) -> void;

private:
    std::vector<std::unique_ptr<basic_action>> _steps;

    std::size_t _pending{ 0 };
    bool _any_failed{ false };
};


template <typename... action_ts>
    requires (sizeof...(action_ts) > 0) && (exec::action_like<std::decay_t<action_ts>> && ...)
parallel_composite::parallel_composite(action_ts&&... steps)
{
    (_steps.push_back(std::make_unique<std::decay_t<action_ts>>(std::forward<action_ts>(steps))), ...);
}

}
