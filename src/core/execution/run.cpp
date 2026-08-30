#include "run.hpp"

#include "core/execution/task_graph.hpp"


namespace he
{

auto run::execute() const -> void
{
    auto graph{ std::make_shared<exec::task_graph>() };

    graph->activate(_target->translate_into_graph(graph->root()));
}

auto run::abort() const -> void
{
    _target->abort();
}

}
