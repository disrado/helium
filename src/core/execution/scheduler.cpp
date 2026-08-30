#include "scheduler.hpp"

#include <ranges>
#include <tuple>


namespace he::exec
{

scheduler::~scheduler()
{
    {
        const auto _{ std::lock_guard{ _stop_sources_mutex } };

        for (auto& source : _stop_sources | std::views::values)
        {
            source.request_stop();
        }
    }

    auto lock{ std::unique_lock{ _shutdown_mutex } };

    // give outstanding async work a short chance to notice the cancellation and finish,
    // rather than blocking shutdown indefinitely if something never does
    _shutdown_cv.wait_for(
        lock,
        std::chrono::seconds{ 1 },
        [this] { return _outstanding_async.load(std::memory_order_acquire) == 0; });
}


auto scheduler::set_dispatcher(std::unique_ptr<dispatcher> new_dispatcher) -> void
{
    _dispatcher.store(std::shared_ptr<dispatcher>{ std::move(new_dispatcher) });
}


auto scheduler::post(task_request request) -> task_id
{
    const auto id{ next_task_id() };

    auto new_task{
        task{
            .mode{ request.mode },
            .definition{ std::move(request.definition) },
            .on_complete{ std::move(request.on_complete) },
            .id{ id }
        }
    };

    {
        const auto _{ std::lock_guard{ _stop_sources_mutex } };

        _stop_sources.emplace(id, std::stop_source{});
    }

    switch (new_task.mode)
    {
        case launch_policy::sync:
        {
            run_task(std::move(new_task));
            break;
        }
        case launch_policy::next_frame:
        {
            queue_next_frame(std::move(new_task));
            break;
        }
        case launch_policy::async:
        {
            dispatch_async(std::move(new_task));
            break;
        }
    }

    return id;
}


auto scheduler::cancel(task_id id) -> bool
{
    const auto _{ std::lock_guard{ _stop_sources_mutex } };

    if (const auto found{ _stop_sources.find(id) }; found != _stop_sources.end())
    {
        found->second.request_stop();
        return true;
    }

    return false;
}


auto scheduler::process() -> void
{
    while (process_queue())
    {
    }
}


auto scheduler::next_task_id() -> task_id
{
    return _next_id.fetch_add(1, std::memory_order_relaxed);
}


auto scheduler::dispatch_async(task new_task) -> void
{
    auto token{ std::stop_token{} };

    {
        const auto _{ std::lock_guard{ _stop_sources_mutex } };

        token = token_for(new_task.id);
    }

    _outstanding_async.fetch_add(1, std::memory_order_relaxed);

    const auto dispatcher{ _dispatcher.load() };

    dispatcher->dispatch(
        [this, target{ std::move(new_task) }, token]() mutable
        {
            target.status = invoke_definition(token, target.definition);

            _queue.enqueue(std::move(target));

            signal_async_complete();
        });
}


auto scheduler::queue_next_frame(task new_task) -> void
{
    _queue.enqueue(std::move(new_task));
}


auto scheduler::process_queue() -> bool
{
    auto processed{ false };
    auto item{ task{} };

    while (_queue.try_dequeue(item))
    {
        if (item.mode == launch_policy::async)
        {
            process_task_completion(item.id, item.status, item.on_complete);
        }
        else
        {
            run_task(std::move(item));
        }

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


auto scheduler::run_task(task target) -> void
{
    auto token{ std::stop_token{} };

    {
        const auto _{ std::lock_guard{ _stop_sources_mutex } };

        token = token_for(target.id);
    }

    const auto status{ invoke_definition(token, target.definition) };

    process_task_completion(target.id, status, target.on_complete);
}


auto scheduler::invoke_definition(const std::stop_token& token, const task_definition& definition) -> execution_status
{
    if (token.stop_requested())
    {
        return execution_status::cancelled;
    }

    return definition.try_execute(token) ? execution_status::completed : execution_status::faulted;
}


auto scheduler::process_task_completion(task_id id, execution_status status, const task_completion& on_complete) -> void
{
    {
        const auto _{ std::lock_guard{ _stop_sources_mutex } };

        if (const auto found{ _stop_sources.find(id) }; found != _stop_sources.end())
        {
            if (found->second.stop_requested())
            {
                status = execution_status::cancelled;
            }

            _stop_sources.erase(found);
        }
    }

    std::ignore = on_complete.try_execute(status);
}


auto scheduler::signal_async_complete() -> void
{
    if (_outstanding_async.fetch_sub(1, std::memory_order_acq_rel) == 1)
    {
        _shutdown_cv.notify_all();
    }
}

}
