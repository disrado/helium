#include "task_graph.hpp"

#include "core/execution/scheduler.hpp"

#include <tuple>
#include <utility>
#include <variant>


namespace
{

template <class... Ts>
struct overloaded: Ts...
{
    using Ts::operator()...;
};

template <class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

}


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
        || current.state == action_state::cancelled
    };

    if (!terminal)
    {
        current.cancel_requested = true;

        if (current.id != invalid_task_id)
        {
            scheduler::instance().cancel(current.id);
        }
        else if (!std::holds_alternative<std::monostate>(current.request))
        {
            current.state = action_state::cancelled;
        }
    }

    for (auto& child : current.get_children())
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

    if (std::holds_alternative<std::monostate>(current.request))
    {
        std::ignore = current.post_execution.execute(task_result::succeeded);

        return;
    }

    current.id = std::visit(overloaded{
        [] (std::monostate) -> task_id { return invalid_task_id; },   // unreachable given the early-out above
        [&] (sync_task_request req) -> task_id
        {
            req.on_complete.bind(
                [self{ shared_from_this() }, current{ &current }] (task_result status)
                {
                    std::ignore = current->post_execution.execute(status);

                    self->advance();
                });

            return scheduler::instance().post(std::move(req));
        },
        [&] (async_task_request req) -> task_id
        {
            req.on_complete.bind(
                [self{ shared_from_this() }, current{ &current }] (task_result status)
                {
                    std::ignore = current->post_execution.execute(status);

                    self->advance();
                });

            return scheduler::instance().post(std::move(req));
        },
        [&] (ticking_task_request req) -> task_id
        {
            req.on_complete.bind(
                [self{ shared_from_this() }, current{ &current }] (task_result status)
                {
                    std::ignore = current->post_execution.execute(status);

                    self->advance();
                });

            return scheduler::instance().post(std::move(req));
        }
    }, current.request);
}

}
