#include "async_task.hpp"

#include <utility>


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

async_task::async_task(task_definition definition, std::shared_ptr<dispatcher> dispatcher_ptr)
{
    // shared_ptr, not by-value dispatch() takes std::function, which requires a copyable
    const auto promise{ std::make_shared<std::promise<task_result>>() };
    _future = promise->get_future();

    dispatcher_ptr->dispatch(
        [definition{ std::move(definition) }, token{ stop_source.get_token() }, promise] () mutable
        {
            promise->set_value(invoke_definition(token, definition));
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
