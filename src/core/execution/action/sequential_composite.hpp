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

    auto build_graph(exec::task_graph::node& parent) -> exec::task_graph::node& override;

private:
    struct graph_sequence final
    {
    public:
        exec::task_graph::node* first{ nullptr };
        basic_action* last_action{ nullptr };
    };

    auto setup_sequence(exec::task_graph::node& self_node) -> graph_sequence;
    auto setup_completion(exec::task_graph::node& self_node, const graph_sequence& chain) -> void;

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
