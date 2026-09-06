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
private:
    struct join_state final
    {
        std::size_t pending;
        bool any_failed{ false };
        std::vector<exec::task_node*> step_starts;
    };

public:
    template <typename... action_ts>
        requires (sizeof...(action_ts) > 0) && (exec::action_like<std::decay_t<action_ts>> && ...)
    explicit parallel_composite(action_ts&&... steps);

private:
    auto setup_node(exec::task_node& self_node) -> exec::task_node& override;

    auto setup_join_node(exec::task_node& self_node, exec::task_node& join_node) -> void;

    auto on_step_finished(
        exec::task_node& self_node,
        const exec::task_node* current_begin,
        exec::task_node& join_node,
        const std::shared_ptr<join_state>& state) -> void;

    auto on_self_finished(
        exec::task_node& self_node,
        const std::vector<exec::graph_segment>& entries,
        exec::task_node& join_node) -> void;

    auto resolve_join(exec::task_node& self_node, exec::task_node& join_node, const join_state& state) -> void;

private:
    std::vector<std::shared_ptr<basic_action>> _steps;
};


template <typename... action_ts>
    requires (sizeof...(action_ts) > 0) && (exec::action_like<std::decay_t<action_ts>> && ...)
parallel_composite::parallel_composite(action_ts&&... steps)
{
    (_steps.push_back(std::make_shared<std::decay_t<action_ts>>(std::forward<action_ts>(steps))), ...);
}

}
