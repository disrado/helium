#pragma once

#include "core/execution/action/action_base.hpp"
#include "core/execution/task_graph.hpp"

#include <memory>
#include <optional>


namespace he
{

class run final
{
public:
    explicit run(exec::action_like auto target, std::optional<exec::action_context> initial_context = std::nullopt);

    auto cancel() const -> void;

private:
    std::shared_ptr<exec::task_graph> _graph;
};


run::run(exec::action_like auto target, std::optional<exec::action_context> initial_context)
    : _graph{ std::make_shared<exec::task_graph>() }
{
    auto root{ std::make_shared<decltype(target)>(std::move(target)) };
    auto segment{ root->translate_into_graph(_graph->root()) };

    segment.start.set_context(std::move(initial_context));

    _graph->activate(segment.start);
}

}
