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

    auto translate_into_graph(exec::task_node& parent) -> exec::graph_segment override;

private:
    auto setup_sequence(exec::task_node& self_node, exec::task_node& completion_node) -> exec::task_node*;

    auto translate_steps(exec::task_node& self_node) -> std::vector<exec::graph_segment>;

    auto link_steps(
        const exec::graph_segment& step,
        exec::task_node& self_node,
        exec::task_node* next_segment_start,
        exec::task_node& completion_node) -> void;

    auto on_action_finished(
        exec::task_node& self_node,
        exec::task_node* first_entry,
        const exec::task_node& completion_node) -> void;

    auto on_step_finished(
        exec::task_node& self_node,
        exec::task_node* step_start,
        exec::task_node* next_segment_start,
        const exec::task_node& completion_node) -> void;

private:
    std::vector<std::shared_ptr<basic_action>> _steps;
};


template <typename... action_ts>
    requires (sizeof...(action_ts) > 0) && (exec::action_like<std::decay_t<action_ts>> && ...)
sequential_composite::sequential_composite(action_ts&&... steps)
{
    (_steps.push_back(std::make_shared<std::decay_t<action_ts>>(std::forward<action_ts>(steps))), ...);
}

}
