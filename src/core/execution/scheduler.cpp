#include "scheduler.hpp"

#include "core/execution/task/async_task.hpp"
#include "core/execution/task/ticking_task.hpp"

#include <cassert>
#include <ranges>
#include <tuple>


namespace
{

auto invoke_definition(const std::stop_token& token, const he::exec::task_definition& definition) -> he::exec::task_result
{
    if (token.stop_requested())
    {
        return he::exec::task_result::cancelled;
    }

    return definition.try_execute(token).value_or(he::exec::task_result::failed);
}

}


namespace he::exec
{

scheduler::~scheduler()
{
    drain();
}


auto scheduler::create() -> std::shared_ptr<scheduler>
{
    // allows instantiation in third-party while keeping constructor hidden
    struct enabler final: scheduler
    {
    };

    return std::make_shared<enabler>();
}


auto scheduler::set_dispatcher(std::unique_ptr<dispatcher> new_dispatcher) -> void
{
    _dispatcher.store(std::shared_ptr<dispatcher>{ std::move(new_dispatcher) });
}


auto scheduler::get_dispatcher() -> std::shared_ptr<dispatcher>
{
    return _dispatcher.load();
}


auto scheduler::post(sync_task_request request) -> task_id
{
    {
        const auto _{ std::lock_guard{ _mutex } };

        if (_is_shutting_down)
        {
            return invalid_task_id;
        }
    }

    const auto status{ invoke_definition(std::stop_token{}, request.definition) };

    std::ignore = request.on_complete.try_execute(status);

    return invalid_task_id;
}


auto scheduler::post(async_task_request request) -> task_id
{
    assert((!request.repetitions || request.repetitions.value() > 0) && "repetitions{0} has no defined meaning");

    {
        const auto _{ std::lock_guard{ _mutex } };

        if (_is_shutting_down)
        {
            return invalid_task_id;
        }
    }

    scheduled_task new_task{
        .id{ next_task_id() },
        .instance = {},
        .trigger_point = std::chrono::steady_clock::now() + request.delay,
        .interval = request.interval,
        .repetitions_left = request.repetitions,
        .on_complete = std::move(request.on_complete),
        .cancel_requested = false,
        .create = [this, definition{ std::move(request.definition) }]() -> std::shared_ptr<task_base>
        {
            return std::make_shared<async_task>(definition, get_dispatcher());
        }
    };

    if (is_eligible_for_starting(new_task))
    {
        new_task.instance = new_task.create(); // unlocked: dispatch may run synchronously
        new_task.trigger_point.reset();
    }

    {
        const auto _{ std::lock_guard{ _mutex } };

        if (_is_shutting_down)
        {
            return invalid_task_id;
        }

        _tasks.emplace(new_task.id, std::move(new_task));
    }

    return new_task.id;
}


auto scheduler::post(ticking_task_request request) -> task_id
{
    assert((!request.repetitions || request.repetitions.value() > 0) && "repetitions{0} has no defined meaning");

    scheduled_task new_task{
        .id{ next_task_id() },
        .instance = {},
        .trigger_point = std::chrono::steady_clock::now() + request.delay,
        .interval = request.interval,
        .repetitions_left = request.repetitions,
        .on_complete = std::move(request.on_complete),
        .cancel_requested = false,
        .create = [definition{ std::move(request.definition) }]() -> std::shared_ptr<task_base>
        {
            return std::make_shared<ticking_task>(definition);
        }
    };

    {
        const auto _{ std::lock_guard{ _mutex } };

        if (_is_shutting_down)
        {
            return invalid_task_id;
        }

        _tasks.emplace(new_task.id, std::move(new_task));
    }

    return new_task.id;
}


auto scheduler::tick() -> void
{
    _snapshot.clear();

    {
        const auto _{ std::lock_guard{ _mutex } };

        std::ranges::copy(_tasks | std::views::keys, std::back_inserter(_snapshot));
    }

    for (const auto id : _snapshot)
    {
        if (is_cancelled_while_idle(id))
        {
            finalize_task_instance(id, task_result::cancelled);

            continue;
        }

        if (const auto instance{ acquire_task_instance(id) })
        {
            instance->tick();

            if (const auto result{ instance->get_result() }; result.has_value())
            {
                finalize_task_instance(id, result.value());
            }
        }
    }
}


auto scheduler::cancel(task_id id) -> bool
{
    const auto _{ std::lock_guard{ _mutex } };

    const auto it{ _tasks.find(id) };
    if (it == _tasks.end())
    {
        return false;
    }

    it->second.cancel_requested = true;

    if (it->second.instance)
    {
        it->second.instance->cancel(); // in-flight: flag it, tick() resolves it on the next cycle
    }

    return true;
}


auto scheduler::next_task_id() -> task_id
{
    return _next_id.fetch_add(1, std::memory_order_relaxed);
}


auto scheduler::acquire_task_instance(task_id id) -> std::shared_ptr<task_base>
{
    {
        const auto _{ std::lock_guard{ _mutex } };

        const auto it{ _tasks.find(id) };
        if (it == _tasks.end())
        {
            return nullptr;
        }

        auto& task = it->second;

        if (!task.instance && is_eligible_for_starting(task))
        {
            task.instance = task.create();
            task.trigger_point.reset();
        }

        return task.instance;
    }

    return nullptr;
}


auto scheduler::finalize_task_instance(task_id id, task_result result) -> void
{
    task_completion on_complete;


    {
        const auto _{ std::lock_guard{ _mutex } };

        const auto it{ _tasks.find(id) };
        if (it == _tasks.end())
        {
            return;
        }

        on_complete = it->second.on_complete;

        if (it->second.repetitions_left.has_value())
        {
            --it->second.repetitions_left.value();
        };
    }

    // instance still set during execution, so self-cancel won't erase
    std::ignore = on_complete.try_execute(result);

    {
        const auto _{ std::lock_guard{ _mutex } };

        const auto it{ _tasks.find(id) };
        if (it == _tasks.end())
        {
            return;
        }

        const auto exhausted{ it->second.repetitions_left.has_value() && it->second.repetitions_left.value() == 0 };
        if (exhausted || result == task_result::cancelled || it->second.cancel_requested)
        {
            _tasks.erase(it);
        }
        else
        {
            it->second.instance = nullptr;
            it->second.trigger_point = std::chrono::steady_clock::now() + it->second.interval;
        }
    }
}


auto scheduler::is_eligible_for_starting(const scheduled_task& task) const -> bool
{
    return std::chrono::steady_clock::now() >= task.trigger_point.value();
}


auto scheduler::is_cancelled_while_idle(task_id id) -> bool
{
    const auto _{ std::lock_guard{ _mutex } };

    const auto it{ _tasks.find(id) };

    return it != _tasks.end() && !it->second.instance && it->second.cancel_requested;
}


auto scheduler::drain() -> void
{
    std::vector<scheduled_task> snapshot;

    {
        const auto _{ std::lock_guard{ _mutex } };

        _is_shutting_down = true;

        snapshot.reserve(_tasks.size());

        for (auto& entry : _tasks | std::ranges::views::values)
        {
            snapshot.push_back(std::move(entry));
        }

        _tasks.clear();
    }

    for (auto& entry : snapshot)
    {
        if (!entry.instance)
        {
            std::ignore = entry.on_complete.try_execute(task_result::cancelled);

            continue;
        }

        if (const auto result{ entry.instance->get_result() })
        {
            std::ignore = entry.on_complete.try_execute(result.value());
        }
        else
        {
            entry.instance->cancel();
        }
    }
}

}
