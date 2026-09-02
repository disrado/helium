#pragma once

#include "defs.hpp"
#include "action/action.hpp"
#include "task_graph.hpp"

#include <memory>


namespace he
{

class run final
{
public:
    explicit run(exec::action_like auto target);

    auto execute() -> void;
    auto cancel() const -> void;

private:
    std::shared_ptr<exec::basic_action> _root_action;

    std::shared_ptr<exec::task_graph> _graph;
};

run::run(exec::action_like auto target)
    : _root_action{ std::make_shared<decltype(target)>(std::move(target)) }
{
}

}
