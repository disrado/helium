#include "scheduler.hpp"


namespace he::exec
{

scheduler::~scheduler()
{
    {
        const auto _{ std::lock_guard{ _mutex } };

        for (auto& [id, source] : _stop_sources)
        {
            source.request_stop();
        }
    }

    auto lock{ std::unique_lock{ _completed_mutex } };

    // give outstanding async work a short chance to notice the cancellation and finish,
    // rather than blocking shutdown indefinitely if something never does
    _outstanding_cv.wait_for(
        lock,
        std::chrono::seconds{ 1 },
        [this] { return _outstanding_async.load(std::memory_order_acquire) == 0; });
}


auto scheduler::set_dispatcher(std::unique_ptr<dispatcher> new_dispatcher) -> void
{
    const auto _{ std::lock_guard{ _mutex } };

    _dispatcher = std::move(new_dispatcher);
}


auto scheduler::post(task new_task) -> task_id
{
    const auto id{ next_task_id() };

    {
        const auto _{ std::lock_guard{ _mutex } };

        _stop_sources.emplace(id, std::stop_source{});
    }

    switch (new_task.mode)
    {
        case task::type::sync:
        {
            queue_sync(id, std::move(new_task));
            break;
        }
        case task::type::async:
        {
            queue_async(id, std::move(new_task));
            break;
        }
    }

    return id;
}


auto scheduler::cancel(task_id id) -> bool
{
    const auto _{ std::lock_guard{ _mutex } };

    if (const auto found{ _stop_sources.find(id) }; found != _stop_sources.end())
    {
        found->second.request_stop();
        return true;
    }

    return false;
}


auto scheduler::process() -> void
{
    while (true)
    {
        const auto async_processed{ process_async() };
        const auto sync_processed{ process_sync() };

        if (!async_processed && !sync_processed)
        {
            break;
        }
    }
}


auto scheduler::next_task_id() -> task_id
{
    return _next_id.fetch_add(1, std::memory_order_relaxed);
}


auto scheduler::queue_sync(task_id id, task new_task) -> void
{
    const auto _{ std::lock_guard{ _mutex } };

    _queue.push(queued_task{ .id = id, .target = std::move(new_task) });
}


auto scheduler::queue_async(task_id id, task new_task) -> void
{
    const auto _{ std::lock_guard{ _mutex } };

    auto token{ token_for(id) };

    _outstanding_async.fetch_add(1, std::memory_order_relaxed);

    _dispatcher->dispatch(
        [this, id, target = std::move(new_task), token]() mutable
        {
            if (!token.stop_requested())
            {
                target.definition(token);
            }

            {
                const auto _{ std::lock_guard{ _completed_mutex } };

                _completed.push(completed_task{ .id = id, .on_complete = std::move(target.on_complete) });
            }

            if (_outstanding_async.fetch_sub(1, std::memory_order_acq_rel) == 1)
            {
                _outstanding_cv.notify_all();
            }
        });
}


auto scheduler::process_async() -> bool
{
    const auto _{ std::lock_guard{ _completed_mutex } };

    auto processed{ false };

    while (!_completed.empty())
    {
        auto completed{ std::move(_completed.front()) };
        _completed.pop();

        run(completed.id, completed.on_complete);

        processed = true;
    }

    return processed;
}


auto scheduler::process_sync() -> bool
{
    auto processed{ false };

    while (true)
    {
        auto queued{ queued_task{} };
        auto token{ std::stop_token{} };

        {
            const auto _{ std::lock_guard{ _mutex } };

            if (_queue.empty())
            {
                break;
            }

            queued = std::move(_queue.front());
            _queue.pop();

            token = token_for(queued.id);
        }

        if (!token.stop_requested())
        {
            queued.target.definition(token);
        }

        run(queued.id, queued.target.on_complete);

        processed = true;
    }

    return processed;
}


auto scheduler::token_for(task_id id) -> std::stop_token
{
    if (const auto found{ _stop_sources.find(id) }; found != _stop_sources.end())
    {
        return found->second.get_token();
    }

    return {};
}


auto scheduler::run(task_id id, const std::function<void()>& action) -> void
{
    auto cancelled{ false };

    {
        const auto _{ std::lock_guard{ _mutex } };

        if (const auto found{ _stop_sources.find(id) }; found != _stop_sources.end())
        {
            cancelled = found->second.stop_requested();
            _stop_sources.erase(found);
        }
    }

    if (!cancelled)
    {
        action();
    }
}

}
