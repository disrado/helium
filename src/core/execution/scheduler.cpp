#include "scheduler.hpp"

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


auto scheduler::create() -> std::shared_ptr<scheduler>
{
    // allows instantiation in third-party while keeping constructor hidden
    struct enabler final: scheduler {};

    return std::make_shared<enabler>();
}


scheduler::~scheduler()
{
    {
        const auto _{ std::lock_guard{ _tasks_mutex } };

        _is_shutting_down = true;

        for (const auto& record : _tasks | std::views::values)
        {
            record->stop_source.request_stop();
        }
    }

    drain();
}


auto scheduler::set_dispatcher(std::unique_ptr<dispatcher> new_dispatcher) -> void
{
    _dispatcher.store(std::shared_ptr<dispatcher>{ std::move(new_dispatcher) });
}


auto scheduler::post(task_request request) -> task_id
{
    const auto id{ next_task_id() };
    const auto mode{ request.mode };

    if (mode == launch_policy::sync)
    {
        {
            const auto _{ std::lock_guard{ _tasks_mutex } };

            if (_is_shutting_down)
            {
                return invalid_task_id;
            }
        }

        // sync's id can't leak to another thread or code path before post() returns — no window
        // where index tracking would buy anything, so skip it entirely: no record, no map entry
        const auto status{ invoke_definition(std::stop_token{}, request.definition) };

        std::ignore = request.on_complete.try_execute(status);

        return id;
    }

    // task holds an atomic member, so it can't be constructed as a temporary and passed
    // into make_shared by value — default-construct in place, then assign fields individually
    auto record{ std::make_shared<task>(mode, std::move(request.definition), std::move(request.on_complete)) };

    {
        const auto _{ std::lock_guard{ _tasks_mutex } };

        if (_is_shutting_down)
        {
            return invalid_task_id;
        }

        _tasks.emplace(id, record);
    }

    switch (mode)
    {
        case launch_policy::sync:
        {
            break;
        }
        case launch_policy::async:
        {
            dispatch_async(id, std::move(record));
            break;
        }
        case launch_policy::next_frame:
        {
            _queue.enqueue(id);
            break;
        }
        case launch_policy::tick:
        {
            _queue.enqueue(id);
            break;
        }

    }

    return id;
}


auto scheduler::cancel(task_id id) -> bool
{
    const auto record{ find_record(id) };

    if (!record)
    {
        return false;
    }

    if (record->phase.load(std::memory_order_acquire) == task_phase::completed)
    {
        return false;
    }

    record->stop_source.request_stop();

    return true;
}


auto scheduler::process() -> void
{
    _queue.drain_active([this] (task_id id) { process_one(id); });
}


auto scheduler::next_task_id() -> task_id
{
    return _next_id.fetch_add(1, std::memory_order_relaxed);
}


auto scheduler::dispatch_async(task_id id, std::shared_ptr<task> record) -> void
{
    record->phase.store(task_phase::running, std::memory_order_relaxed);

    _dispatcher.load()->dispatch(
        [weak{ weak_from_this() }, id, record, stop_token{ record->stop_source.get_token() }]() mutable
        {
            record->result = invoke_definition(stop_token, record->definition);
            record->phase.store(task_phase::completed, std::memory_order_release);

            if (const auto self{ weak.lock() })
            {
                self->_queue.enqueue(id);
            }
        });
}


auto scheduler::run_record(task_id id, const std::shared_ptr<task>& record) -> void
{
    record->phase.store(task_phase::running, std::memory_order_relaxed);

    auto status{ invoke_definition(record->stop_source.get_token(), record->definition) };

    if (status == execution_status::running)
    {
        if (record->mode != launch_policy::tick)
        {
            // running is only a legal outcome for tick — a plain next_frame definition returning
            // it is a bug in that definition, not something to silently re-queue
            status = execution_status::faulted;
        }
        else
        {
            // not done yet, call me again — go back to queued (not completed) so the next
            // process() call runs this same record again instead of delivering it
            record->phase.store(task_phase::queued, std::memory_order_relaxed);
            _queue.enqueue(id);

            return;
        }
    }

    record->result = status;
    record->phase.store(task_phase::completed, std::memory_order_release);

    deliver(id, record);
}


auto scheduler::deliver(task_id id, const std::shared_ptr<task>& record) -> void
{
    // pairs with the release store in run_record/dispatch_async so `result` is visible here
    // even when the two ran on different threads
    std::ignore = record->phase.load(std::memory_order_acquire);

    {
        const auto _{ std::lock_guard{ _tasks_mutex } };

        _tasks.erase(id);
    }

    std::ignore = record->on_complete.try_execute(record->result);
}


auto scheduler::process_one(task_id id) -> void
{
    const auto record{ find_record(id) };

    if (!record)
    {
        return;
    }

    if (record->phase.load(std::memory_order_acquire) == task_phase::completed)
    {
        deliver(id, record);
    }
    else
    {
        run_record(id, record);
    }
}


auto scheduler::find_record(task_id id) -> std::shared_ptr<task>
{
    const auto _{ std::lock_guard{ _tasks_mutex } };

    const auto found{ _tasks.find(id) };

    return found != _tasks.end() ? found->second : nullptr;
}


auto scheduler::drain() -> void
{
    _queue.drain([this] (task_id id) { process_one(id); });
}

}
