#include "task_graph.hpp"

#include "core/execution/scheduler.hpp"


namespace he::exec
{

task_graph::task_graph()
    : _root{ *this }
{
}


auto task_graph::root() -> task_node&
{
    return _root;
}


auto task_graph::cancel() -> void
{
    cancel_subtree(_root);
}


auto task_graph::cancel_subtree(task_node& current) -> void
{
    const auto terminal{
        current.state == action_state::succeeded
        || current.state == action_state::failed
        || current.state == action_state::cancelled };

    if (!terminal)
    {
        current.cancel_requested = true;

        if (current.id != invalid_task_id)
        {
            scheduler::instance().cancel(current.id);
        }
        else if (current.definition.is_bound())
        {
            current.state = action_state::cancelled;
        }
    }

    for (auto& child: current.children())
    {
        cancel_subtree(*child);
    }
}


auto task_graph::activate(task_node& target) -> void
{
    {
        const auto _{ std::lock_guard{ _mutex } };

        _stack.push_back(&target);
    }

    advance();
}


auto task_graph::advance() -> void
{
    {
        const auto _{ std::lock_guard{ _mutex } };

        if (_running)
        {
            return;
        }

        _running = true;
    }

    while (auto* const current{ pop_next() })
    {
        run_node(*current);
    }
}


auto task_graph::pop_next() -> task_node*
{
    const auto _{ std::lock_guard{ _mutex } };

    if (_stack.empty())
    {
        _running = false;

        return nullptr;
    }

    auto* const current{ _stack.back() };

    _stack.pop_back();

    return current;
}


auto task_graph::run_node(task_node& current) -> void
{
    if (!current.pre_condition.try_execute().value_or(true))
    {
        return;
    }

    if (!current.definition.is_bound())
    {
        std::ignore = current.post_execution.execute(execution_status::completed);

        return;
    }

    current.id = scheduler::instance().post(
        task_request{
            .mode{ current.mode },
            .definition{ current.definition },
            .on_complete{
                [self{ shared_from_this() }, current{ &current }] (execution_status status)
                {
                    std::ignore = current->post_execution.execute(status);

                    self->advance();
                } }
        });
}

}
