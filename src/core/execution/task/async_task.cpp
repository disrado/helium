#include "async_task.hpp"

#include <utility>


namespace he::exec
{

async_task::async_task(task_definition definition, std::shared_ptr<dispatcher> dispatcher_ptr)
{
    // shared_ptr, not by-value - dispatch() takes std::function, which requires a copyable
    const auto promise{ std::make_shared<std::promise<task_result>>() };
    _future = promise->get_future();

    dispatcher_ptr->dispatch(
        [definition{ std::move(definition) }, token{ stop_source.get_token() }, promise] () mutable
        {
            promise->set_value(token.stop_requested() ? task_result::cancelled : definition.execute(token));
        });
}


auto async_task::get_result() -> std::optional<task_result>
{
    if (!_future.valid() || _future.wait_for(std::chrono::seconds{ 0 }) != std::future_status::ready)
    {
        return std::nullopt;
    }

    return _future.get();
}


auto async_task::cancel() -> void
{
    stop_source.request_stop();
}

}
