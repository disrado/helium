#include "run.hpp"


namespace he
{

auto run::execute() -> void
{
    _graph = std::make_shared<exec::task_graph>();

    _graph->activate(_target->translate_into_graph(_graph->root()).begin);
}

auto run::cancel() const -> void
{
    _target->cancel();
}

}
