#pragma once

#include "core/execution/action/action_base.hpp"

#include <memory>
#include <type_traits>
#include <utility>
#include <vector>


namespace he
{

class sequential_composite final: public exec::action_base<sequential_composite>
{
public:
    template <typename... action_ts>
        requires (sizeof...(action_ts) > 0) && (exec::action_like<std::decay_t<action_ts>> && ...)
    explicit sequential_composite(action_ts&&... steps);

    auto cancel() -> void override;

protected:
    auto expand_on_graph(exec::task_graph::node& parent) -> exec::graph_segment override;

private:
    auto setup_sequence(
        exec::task_graph::node& self_node,
        exec::task_graph::node& completion_node,
        exec::task_graph::node* then_child,
        exec::task_graph::node* else_child) -> exec::task_graph::node*;

private:
    std::vector<std::unique_ptr<basic_action>> _steps;
};


template <typename... action_ts>
    requires (sizeof...(action_ts) > 0) && (exec::action_like<std::decay_t<action_ts>> && ...)
sequential_composite::sequential_composite(action_ts&&... steps)
{
    (_steps.push_back(std::make_unique<std::decay_t<action_ts>>(std::forward<action_ts>(steps))), ...);
}

}
