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
            run_task(id, std::move(new_task));
            break;
        }
        case task::type::next_frame:
        {
            queue_next_frame(id, std::move(new_task));
            break;
        }
        case task::type::async:
        {
            dispatch_async(id, std::move(new_task));
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
        const auto next_frame_processed{ process_next_frame() };

        if (!async_processed && !next_frame_processed)
        {
            break;
        }
    }
}


auto scheduler::next_task_id() -> task_id
{
    return _next_id.fetch_add(1, std::memory_order_relaxed);
}


auto scheduler::dispatch_async(task_id id, task new_task) -> void
{
    auto token{ std::stop_token{} };

    {
        const auto _{ std::lock_guard{ _mutex } };

        token = token_for(id);
    }

    _outstanding_async.fetch_add(1, std::memory_order_relaxed);

    _dispatcher->dispatch(
        [this, id, target = std::move(new_task), token]() mutable
        {
            const auto status{ invoke_definition(token, target.definition) };

            {
                const auto _{ std::lock_guard{ _completed_mutex } };

                _completed.push(completed_task{ .id = id, .status = status, .on_complete = std::move(target.on_complete) });
            }

            signal_async_complete();
        });
}


auto scheduler::queue_next_frame(task_id id, task new_task) -> void
{
    const auto _{ std::lock_guard{ _mutex } };

    _queue.push(queued_task{ .id = id, .target = std::move(new_task) });
}


auto scheduler::process_async() -> bool
{
    const auto _{ std::lock_guard{ _completed_mutex } };

    auto processed{ false };

    while (!_completed.empty())
    {
        auto completed{ std::move(_completed.front()) };
        _completed.pop();

        process_task_completion(completed.id, completed.status, completed.on_complete);

        processed = true;
    }

    return processed;
}


auto scheduler::process_next_frame() -> bool
{
    auto processed{ false };

    while (true)
    {
        auto queued{ queued_task{} };

        {
            const auto _{ std::lock_guard{ _mutex } };

            if (_queue.empty())
            {
                break;
            }

            queued = std::move(_queue.front());
            _queue.pop();
        }

        run_task(queued.id, std::move(queued.target));

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


auto scheduler::run_task(task_id id, task target) -> void
{
    auto token{ std::stop_token{} };

    {
        const auto _{ std::lock_guard{ _mutex } };

        token = token_for(id);
    }

    const auto status{ invoke_definition(token, target.definition) };

    process_task_completion(id, status, target.on_complete);
}


auto scheduler::invoke_definition(const std::stop_token& token, const task_definition& definition) -> task::status
{
    if (token.stop_requested())
    {
        return task::status::cancelled;
    }

    return definition(token);
}


auto scheduler::process_task_completion(task_id id, task::status status, const std::function<void(task::status)>& on_complete) -> void
{
    {
        const auto _{ std::lock_guard{ _mutex } };

        if (const auto found{ _stop_sources.find(id) }; found != _stop_sources.end())
        {
            if (found->second.stop_requested())
            {
                status = task::status::cancelled;
            }

            _stop_sources.erase(found);
        }
    }

    on_complete(status);
}


auto scheduler::signal_async_complete() -> void
{
    if (_outstanding_async.fetch_sub(1, std::memory_order_acq_rel) == 1)
    {
        _outstanding_cv.notify_all();
    }
}

}
