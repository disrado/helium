#include "scheduler.hpp"

#include <cassert>
#include <ranges>
#include <tuple>


namespace
{

auto invoke_definition(const std::stop_token& token, const he::exec::task_definition& definition) -> he::exec::execution_status
{
    if (token.stop_requested())
    {
        return he::exec::execution_status::cancelled;
    }

    return definition.try_execute(token).value_or(he::exec::execution_status::faulted);
}

}


namespace he::exec
{

auto scheduler::double_buffered_queue::enqueue(task_id id) -> void
{
    _queues[_active.load(std::memory_order_acquire)].enqueue(id);
}


auto scheduler::double_buffered_queue::drain_active(const std::function<void(task_id)>& handler) -> void
{
    const auto draining{ _active.load(std::memory_order_acquire) };

    _active.store(1 - draining, std::memory_order_release);

    auto id{ invalid_task_id };

    while (_queues[draining].try_dequeue(id))
    {
        handler(id);
    }
}


auto scheduler::double_buffered_queue::drain(const std::function<void(task_id)>& handler) -> void
{
    for (auto& queue : _queues)
    {
        auto id{ invalid_task_id };

        while (queue.try_dequeue(id))
        {
            handler(id);
        }
    }
}


scheduler::~scheduler()
{
    {
        const auto _{ std::lock_guard{ _tasks_mutex } };

        _is_shutting_down = true;

        for (const auto& task : _tasks | std::views::values)
        {
            task->stop_source.request_stop();
        }
    }

    drain();
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


auto scheduler::post(task_request request) -> task_id
{
    switch (request.mode)
    {
        case launch_policy::sync:
        {
            run_inline(std::move(request));
            return invalid_task_id;
        }
        case launch_policy::async:
        {
            if (const auto task{ allocate_task(std::move(request)) })
            {
                dispatch_async(task);
                return task->id;
            }

            break;
        }
        case launch_policy::next_frame:
        {
            [[fallthrough]];
        }
        case launch_policy::tick:
        {
            if (const auto task{ allocate_task(std::move(request)) })
            {
                _queue.enqueue(task->id);
                return task->id;
            }

            break;
        }

    }

    return invalid_task_id;
}


auto scheduler::cancel(task_id id) -> bool
{
    const auto task{ find_task(id) };

    if (!task)
    {
        return false;
    }

    if (task->phase.load(std::memory_order_acquire) == task_phase::completed)
    {
        return false;
    }

    task->stop_source.request_stop();

    return true;
}


auto scheduler::process() -> void
{
    _queue.drain_active([this] (task_id id) { process_task(id); });
}


auto scheduler::next_task_id() -> task_id
{
    return _next_id.fetch_add(1, std::memory_order_relaxed);
}


auto scheduler::allocate_task(task_request request) -> std::shared_ptr<task>
{
    auto instance{ std::make_shared<task>() };

    instance->id = next_task_id();
    instance->mode = request.mode;
    instance->definition = std::move(request.definition);
    instance->on_complete = std::move(request.on_complete);

    {
        const auto _{ std::lock_guard{ _tasks_mutex } };

        if (_is_shutting_down)
        {
            instance = nullptr;
        }
        else
        {
            _tasks.emplace(instance->id, instance);
        }
    }

    return instance;
}


auto scheduler::dispatch_async(std::shared_ptr<task> target_task) -> void
{
    target_task->phase.store(task_phase::running, std::memory_order_relaxed);

    _dispatcher.load()->dispatch(
        [weak{ weak_from_this() }, target_task, stop_token{ target_task->stop_source.get_token() }]() mutable
        {
            target_task->result = invoke_definition(stop_token, target_task->definition);
            target_task->phase.store(task_phase::completed, std::memory_order_release);

            if (const auto self{ weak.lock() })
            {
                self->_queue.enqueue(target_task->id);
            }
        });
}


auto scheduler::run_inline(task_request request) -> void
{
    {
        const auto _{ std::lock_guard{ _tasks_mutex } };

        if (_is_shutting_down)
        {
            return;
        }
    }

    const auto status{ invoke_definition(std::stop_token{}, request.definition) };

    std::ignore = request.on_complete.try_execute(status);
}


auto scheduler::run_sync(std::shared_ptr<task> target_task) -> void
{
    target_task->phase.store(task_phase::running, std::memory_order_relaxed);

    target_task->result = invoke_definition(target_task->stop_source.get_token(), target_task->definition);

    target_task->phase.store(task_phase::completed, std::memory_order_relaxed);

    run_completion(target_task);
}


auto scheduler::run_tick(std::shared_ptr<task> target_task) -> void
{
    target_task->phase.store(task_phase::running, std::memory_order_relaxed);

    auto status{ invoke_definition(target_task->stop_source.get_token(), target_task->definition) };

    if (status == execution_status::running)
    {
        target_task->phase.store(task_phase::queued, std::memory_order_relaxed);
        _queue.enqueue(target_task->id);

        return;
    }

    target_task->result = status;
    target_task->phase.store(task_phase::completed, std::memory_order_relaxed);

    run_completion(target_task);
}


auto scheduler::run_completion(std::shared_ptr<task> target_task) -> void
{
    {
        const auto _{ std::lock_guard{ _tasks_mutex } };

        _tasks.erase(target_task->id);
    }

    std::ignore = target_task->on_complete.try_execute(target_task->result);
}


auto scheduler::process_task(task_id id) -> void
{
    const auto task{ find_task(id) };
    if (!task)
    {
        return;
    }

    // acquire pairs with dispatch_async's release store - makes 'result' (yes, result) written
    // on the worker thread visible to run_completion()
    switch (task->phase.load(std::memory_order_acquire))
    {
        case task_phase::running:
        {
            // nothing to do
            return;
        }
        case task_phase::completed:
        {
            run_completion(task);
            break;
        }
        case task_phase::queued:
        {
            process_queued(task);
            break;
        }
        default:
        {
            std::unreachable();
        }
    }
}


auto scheduler::process_queued(std::shared_ptr<task> target_task) -> void
{
    switch (target_task->mode)
    {
        case launch_policy::sync:
        {
            // nothing to do
            break;
        }
        case launch_policy::async:
        {
            // nothing to do
            break;
        }
        case launch_policy::next_frame:
        {
            run_sync(target_task);
            break;
        }
        case launch_policy::tick:
        {
            run_tick(target_task);
            break;
        }
        default:
        {
            std::unreachable();
        }
    }
}


auto scheduler::find_task(task_id id) -> std::shared_ptr<task>
{
    const auto _{ std::lock_guard{ _tasks_mutex } };

    const auto found{ _tasks.find(id) };

    return found != _tasks.end() ? found->second : nullptr;
}


auto scheduler::drain() -> void
{
    _queue.drain([this] (task_id id) { process_task(id); });
}

}
