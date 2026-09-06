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

protected:
    auto setup_node(exec::task_node& self_node) -> exec::task_node& override;

private:
    auto translate_steps(exec::task_node& self_node) -> std::vector<exec::graph_segment>;

    static auto setup_step_node(
        exec::task_node& self_node,
        exec::task_node& completion_node,
        const exec::graph_segment& step,
        exec::task_node* next_step_start) -> void;

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
