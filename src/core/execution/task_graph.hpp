#pragma once

#include "core/delegate/delegate.hpp"
#include "core/delegate/multicast_delegate.hpp"
#include "core/execution/defs.hpp"

#include <memory>
#include <mutex>
#include <optional>
#include <vector>


namespace he::exec
{

class task_graph final: public std::enable_shared_from_this<task_graph>
{
public:
    class node final
    {
    public:
        explicit node(task_graph& graph, node* parent = nullptr);

        auto add_child() -> node&;
        auto activate() -> void;

        auto parent() const -> node*;
        auto children() const -> const std::vector<std::unique_ptr<node>>&;

    public:
        launch_policy mode{ launch_policy::sync };
        task_definition definition;
        delegate<bool()> pre_condition;
        multicast_delegate<execution_status> post_condition;

        task_id id{ invalid_task_id };

        action_state state{ action_state::dormant };
        bool cancel_requested{ false };
        std::optional<action_context> context;

        // set once, on the root node, by basic_action::run(): pins the action tree alive for
        // as long as the graph is (which in-flight async work already guarantees via run_node's
        // on_complete capturing shared_from_this() on the graph). execution_token::_root alone
        // isn't enough — it only covers the token's own lifetime, not a worker thread mid-flight
        // when the token is dropped, so a cancelled-but-still-running async task would otherwise
        // reach into an already-destroyed action instance through its raw `this` capture.
        std::shared_ptr<void> anchor;

    private:
        task_graph& _graph;
        node* _parent;
        std::vector<std::unique_ptr<node>> _children;
    };

public:
    task_graph();

    auto root() -> node&;
    auto activate(node& target) -> void;
    auto cancel() -> void;

private:
    auto advance() -> void;
    auto pop_next() -> node*;
    auto run_node(node& current) -> void;
    auto cancel_subtree(node& current) -> void;

private:
    node _root;

    std::vector<node*> _stack;
    // guards _stack/_running: activate() may be called cross-thread from an async completion
    std::mutex _mutex;

    bool _running{ false };
};

}
