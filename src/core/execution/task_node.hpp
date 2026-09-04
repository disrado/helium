#pragma once

#include "core/delegate/delegate.hpp"
#include "core/delegate/multicast_delegate.hpp"
#include "core/execution/defs.hpp"

#include <memory>
#include <optional>
#include <vector>


namespace he::exec
{

class task_graph;


class task_node final
{
public:
    explicit task_node(task_graph& graph, task_node* parent = nullptr);

    auto add_child() -> task_node&;
    auto activate() -> void;

    auto parent() const -> task_node*;
    auto children() const -> const std::vector<std::unique_ptr<task_node>>&;

    auto get_context() const -> const std::optional<action_context>&;
    auto set_context(std::optional<action_context> new_context) -> void;

    auto merge_context(std::optional<action_context> new_entries) -> void;

public:
    launch_policy mode{ launch_policy::sync };
    task_definition definition;
    delegate<bool()> pre_condition;
    multicast_delegate<execution_status> post_execution;

    task_id id{ invalid_task_id };

    action_state state{ action_state::dormant };

    bool cancel_requested{ false };

    task_node* then_node{ nullptr };
    task_node* else_node{ nullptr };

private:
    std::optional<action_context> _context;

    task_graph& _graph;
    task_node* _parent;
    std::vector<std::unique_ptr<task_node>> _children;
};

}
