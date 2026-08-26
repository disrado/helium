#include "scheduler.hpp"


namespace he::exec
{

auto scheduler::post(task new_task) -> task_id
{
    const auto id{ next_task_id() };

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


auto scheduler::cancel(task_id id) -> void
{
    _cancelled.insert(id);
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


auto scheduler::set_dispatcher(std::unique_ptr<dispatcher> new_dispatcher) -> void
{
    _dispatcher = std::move(new_dispatcher);
}


auto scheduler::next_task_id() -> task_id
{
    return _next_id.fetch_add(1, std::memory_order_relaxed);
}


auto scheduler::queue_sync(task_id id, task new_task) -> void
{
    _queue.push(queued_task{ .id = id, .target = std::move(new_task) });
}


auto scheduler::queue_async(task_id id, task new_task) -> void
{
    _dispatcher->dispatch(
        [this, id, target = std::move(new_task)]() mutable
        {
            target.definition();

            const auto _{ std::lock_guard{ _completed_mutex } };

            _completed.push(completed_task{ .id = id, .on_complete = std::move(target.on_complete) });
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

    while (!_queue.empty())
    {
        auto queued{ std::move(_queue.front()) };
        _queue.pop();

        run(
            queued.id, [&queued]
            {
                queued.target.definition();
                queued.target.on_complete();
            });

        processed = true;
    }

    return processed;
}


auto scheduler::run(task_id id, const std::function<void()>& action) -> void
{
    if (_cancelled.contains(id))
    {
        _cancelled.erase(id);
    }
    else
    {
        action();
    }
}

}
