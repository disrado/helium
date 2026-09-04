#pragma once

#include "core/execution/task_node.hpp"

#include <memory>
#include <mutex>
#include <vector>


namespace he::exec
{

class task_graph final: public std::enable_shared_from_this<task_graph>
{
public:
    task_graph();

    auto root() -> task_node&;
    auto activate(task_node& target) -> void;
    auto cancel() -> void;

private:
    auto advance() -> void;
    auto pop_next() -> task_node*;
    auto run_node(task_node& current) -> void;
    auto cancel_subtree(task_node& current) -> void;

private:
    task_node _root;

    std::vector<task_node*> _stack;

    bool _running{ false };

    // guards _stack/_running: activate() may be called cross-thread from an async completion
    std::mutex _mutex;
};

}
