#pragma once

#include "core/delegate/delegate.hpp"
#include "core/delegate/multicast_delegate.hpp"
#include "core/execution/defs.hpp"
#include "core/execution/scheduler.hpp"

#include <atomic>
#include <memory>
#include <optional>
#include <variant>
#include <vector>


namespace he::exec
{

class task_graph;


class task_node final
{
public:
    struct link final
    {
    public:
        delegate<bool(action_state)> condition;
        task_node* target{ nullptr };
    };

public:
    explicit task_node(task_graph& graph, task_node* parent = nullptr);

    auto resolve_links() -> void;

    auto add_child() -> task_node&;
    auto activate() -> void;

    auto add_link(link entry) -> void;
    auto get_links() const -> const std::vector<link>&;

    auto get_parent() const -> task_node*;

    auto get_children() const -> const std::vector<std::unique_ptr<task_node>>&;

    auto get_context() const -> const action_context&;
    auto set_context(action_context new_context) -> void;
    auto merge_context(action_context source) -> void;

public:
    std::variant<std::monostate, sync_task_request, async_task_request, ticking_task_request> request;
    delegate<bool()> pre_condition;
    multicast_delegate<task_result> post_execution;

    std::atomic<task_id> id{ invalid_task_id };

    std::atomic<action_state> state{ action_state::dormant };

    std::atomic<bool> cancel_requested{ false };

private:
    std::vector<std::unique_ptr<task_node>> _children;
    std::vector<link> _links;

    action_context _context;

    task_graph& _graph;
    task_node* _parent;
};

}
