#include "scheduler.hpp"

#include "core/execution/task/async_task.hpp"
#include "core/execution/task/ticking_task.hpp"

#include <cassert>
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
    std::vector<scheduled_task> snapshot;

    {
        const auto _{ std::lock_guard{ _mutex } };

        _is_shutting_down = true;

        snapshot.reserve(_tasks.size());

        for (auto& [id, entry] : _tasks)
        {
            snapshot.push_back(std::move(entry));
        }

        _tasks.clear();
    }

    for (auto& entry : snapshot)
    {
        if (!entry.cycle_instance)
        {
            std::ignore = entry.on_complete.try_execute(task_result::cancelled);

            continue;
        }

        // get_status() only, never tick() — a ticking_task's work runs inside tick() itself, and
        // shutdown must not re-execute it; get_status() just flushes an already-ready result
        if (const auto result{ entry.cycle_instance->get_status() })
        {
            std::ignore = entry.on_complete.try_execute(result.value());
        }
        else
        {
            entry.cycle_instance->cancel();   // genuinely still in flight: flag it and drop it
        }
    }
}


auto scheduler::create() -> std::shared_ptr<scheduler>
{
    // allows instantiation in third-party while keeping constructor hidden
    struct enabler final: scheduler {};

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


auto scheduler::next_task_id() -> task_id
{
    return _next_id.fetch_add(1, std::memory_order_relaxed);
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

    const auto now{ std::chrono::steady_clock::now() };

    scheduled_task entry;
    entry.interval = request.interval;
    entry.repetitions_left = request.repetitions;
    entry.trigger_point = now + request.initial_delay;
    entry.on_complete = std::move(request.on_complete);
    entry.make_cycle = [this, definition{ std::move(request.definition) }] () -> std::shared_ptr<task_base>
    {
        return std::make_shared<async_task>(definition, get_dispatcher());   // dispatches in its constructor
    };

    if (now >= entry.trigger_point.value())
    {
        entry.cycle_instance = entry.make_cycle();   // unlocked: dispatch may run synchronously
        entry.trigger_point.reset();
    }

    const auto id{ next_task_id() };
    const auto _{ std::lock_guard{ _mutex } };

    if (_is_shutting_down)   // re-check: shutdown may have started while we were constructing above
    {
        return invalid_task_id;
    }

    _tasks.emplace(id, std::move(entry));

    return id;
}


auto scheduler::post(ticking_task_request request) -> task_id
{
    assert((!request.repetitions || request.repetitions.value() > 0) && "repetitions{0} has no defined meaning");

    scheduled_task entry;
    entry.interval = request.interval;
    entry.repetitions_left = request.repetitions;
    entry.trigger_point = std::chrono::steady_clock::now() + request.initial_delay;
    entry.on_complete = std::move(request.on_complete);
    entry.make_cycle = [definition{ std::move(request.definition) }] () -> std::shared_ptr<task_base>
    {
        return std::make_shared<ticking_task>(definition);
    };

    const auto id{ next_task_id() };
    const auto _{ std::lock_guard{ _mutex } };

    if (_is_shutting_down)
    {
        return invalid_task_id;
    }

    _tasks.emplace(id, std::move(entry));

    return id;
}


auto scheduler::cancel(task_id id) -> bool
{
    std::shared_ptr<task_base> cycle;
    task_completion on_complete;
    auto idle{ false };

    {
        const auto _{ std::lock_guard{ _mutex } };

        const auto it{ _tasks.find(id) };

        if (it == _tasks.end())
        {
            return false;
        }

        auto& entry{ it->second };

        if (entry.cycle_instance)
        {
            entry.cancel_requested = true;
            cycle = entry.cycle_instance;
        }
        else
        {
            idle = true;
            on_complete = std::move(entry.on_complete);
            _tasks.erase(it);
        }
    }

    if (idle)
    {
        std::ignore = on_complete.try_execute(task_result::cancelled);   // idle: safe synchronously
    }
    else
    {
        cycle->cancel();   // in flight: defer, let the next tick()/get_status() observe it
    }

    return true;
}


auto scheduler::acquire_cycle(task_id id) -> std::shared_ptr<task_base>
{
    {
        const auto _{ std::lock_guard{ _mutex } };
        const auto it{ _tasks.find(id) };

        if (it == _tasks.end())
        {
            return nullptr;
        }

        if (it->second.cycle_instance)
        {
            return it->second.cycle_instance;
        }
    }

    std::function<std::shared_ptr<task_base>()> make_cycle;

    {
        const auto _{ std::lock_guard{ _mutex } };
        const auto it{ _tasks.find(id) };

        if (it == _tasks.end() || std::chrono::steady_clock::now() < it->second.trigger_point.value())
        {
            return nullptr;
        }

        make_cycle = it->second.make_cycle;
    }

    auto cycle{ make_cycle() };   // unlocked: dispatch may run synchronously

    const auto _{ std::lock_guard{ _mutex } };
    const auto it{ _tasks.find(id) };

    if (it == _tasks.end())
    {
        return nullptr;   // cancelled while we were constructing — drop the orphan cycle, safe
    }

    it->second.cycle_instance = std::move(cycle);
    it->second.trigger_point.reset();

    return it->second.cycle_instance;
}


auto scheduler::finalize_cycle(task_id id, task_result result) -> void
{
    task_completion on_complete;
    auto exhausted{ false };

    {
        const auto _{ std::lock_guard{ _mutex } };
        const auto it{ _tasks.find(id) };

        if (it == _tasks.end())
        {
            return;
        }

        on_complete = it->second.on_complete;
        exhausted = it->second.repetitions_left && --it->second.repetitions_left.value() == 0;
    }

    // delivered with cycle_instance still intact — a reentrant self-cancel from inside this call
    // sees an in-flight entry, not idle, so it can't double-deliver
    std::ignore = on_complete.try_execute(result);

    const auto _{ std::lock_guard{ _mutex } };
    const auto it{ _tasks.find(id) };

    if (it == _tasks.end())
    {
        return;
    }

    if (exhausted || result == task_result::cancelled || it->second.cancel_requested)
    {
        _tasks.erase(it);
    }
    else
    {
        it->second.cycle_instance = nullptr;
        it->second.trigger_point = std::chrono::steady_clock::now() + it->second.interval;
    }
}


auto scheduler::process() -> void
{
    _snapshot.clear();

    {
        const auto _{ std::lock_guard{ _mutex } };

        for (const auto& [id, entry] : _tasks)
        {
            _snapshot.push_back(id);
        }
    }

    for (const auto id : _snapshot)
    {
        const auto cycle{ acquire_cycle(id) };

        if (!cycle)
        {
            continue;
        }

        cycle->tick();

        if (const auto result{ cycle->get_status() })
        {
            finalize_cycle(id, result.value());
        }
    }
}

}
