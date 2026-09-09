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
    std::vector<std::shared_ptr<task_base>> snapshot;

    {
        const auto _{ std::lock_guard{ _mutex } };

        _is_shutting_down = true;

        snapshot.reserve(_tasks.size());

        for (const auto& [id, entry] : _tasks)
        {
            snapshot.push_back(entry);
        }
    }

    for (auto& entry : snapshot)
    {
        entry->deliver_if_finished();   // flush already-completed-but-unpolled async work first — closes
                                        // a real gap where a worker that finished (e.g. a synchronous
                                        // test dispatcher, or one that simply raced ahead of the last
                                        // process() call) would otherwise never be observed and its
                                        // result would be silently lost

        entry->request_cancel();       // no-op if already finished above; otherwise sets the stop flag,
                                        // or delivers synchronously for a genuinely idle entry
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

    auto entry{ std::make_shared<async_task>() };

    entry->id = next_task_id();
    entry->definition = std::move(request.definition);
    entry->on_complete = std::move(request.on_complete);
    entry->trigger_point = std::chrono::steady_clock::now() + request.initial_delay;
    entry->interval = request.interval;
    entry->repetitions_left = request.repetitions;

    {
        const auto _{ std::lock_guard{ _mutex } };

        if (_is_shutting_down)
        {
            return invalid_task_id;
        }

        _tasks.emplace(entry->id, entry);
    }

    if (std::chrono::steady_clock::now() >= entry->trigger_point)
    {
        entry->dispatch(*_dispatcher.load());
    }

    return entry->id;
}


auto scheduler::post(ticking_task_request request) -> task_id
{
    assert((!request.repetitions || request.repetitions.value() > 0) && "repetitions{0} has no defined meaning");

    auto entry{ std::make_shared<ticking_task>() };

    entry->id = next_task_id();
    entry->definition = std::move(request.definition);
    entry->on_complete = std::move(request.on_complete);
    entry->trigger_point = std::chrono::steady_clock::now() + request.initial_delay;
    entry->interval = request.interval;
    entry->repetitions_left = request.repetitions;

    const auto _{ std::lock_guard{ _mutex } };

    if (_is_shutting_down)
    {
        return invalid_task_id;
    }

    _tasks.emplace(entry->id, entry);

    return entry->id;
}


auto scheduler::cancel(task_id id) -> bool
{
    std::shared_ptr<task_base> entry;

    {
        const auto _{ std::lock_guard{ _mutex } };

        const auto it{ _tasks.find(id) };

        if (it == _tasks.end())
        {
            return false;
        }

        entry = it->second;
    }

    if (entry->request_cancel())   // not locked - might execute completion callbacks
    {
        const auto _{ std::lock_guard{ _mutex } };

        _tasks.erase(id);   // no-op if something else already erased it — safe
    }

    return true;
}


auto scheduler::process() -> void
{
    _snapshot.clear();

    {
        const auto _{ std::lock_guard{ _mutex } };

        for (const auto& [id, entry] : _tasks)
        {
            _snapshot.emplace_back(id, entry);
        }
    }

    auto& dispatcher{ *_dispatcher.load() };

    for (auto& [id, entry] : _snapshot)
    {
        if (entry->tick(dispatcher))
        {
            const auto _{ std::lock_guard{ _mutex } };

            _tasks.erase(id);   // no-op if already erased elsewhere
        }
    }
}

}
