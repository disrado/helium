#pragma once

#include "core/delegate/delegate.hpp"
#include "core/delegate/multicast_delegate.hpp"
#include "core/execution/defs.hpp"

#include <memory>
#include <vector>


namespace he::exec
{

class task_graph final
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
        multicast_delegate<> post_condition;

    private:
        task_graph& _graph;
        node* _parent;
        std::vector<std::unique_ptr<node>> _children;
    };

public:
    task_graph();

    auto root() -> node&;
    auto activate(node& target) -> void;

private:
    auto advance() -> void;

private:
    node _root;
    std::vector<node*> _stack;
    bool _running{ false };
};

}
