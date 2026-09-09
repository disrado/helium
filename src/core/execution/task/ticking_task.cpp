#include "ticking_task.hpp"

#include <utility>


namespace
{

auto invoke_definition(const std::stop_token& token, const he::exec::ticking_definition& definition) -> he::exec::tick_result
{
    if (token.stop_requested())
    {
        return he::exec::tick_result::cancelled;
    }

    return definition.try_execute(token).value_or(he::exec::tick_result::failed);
}

}


namespace he::exec
{

ticking_task::ticking_task(ticking_definition definition)
    : definition{ std::move(definition) }
{
}


auto ticking_task::tick() -> void
{
    const auto status{ invoke_definition(stop_source.get_token(), definition) };

    if (status == tick_result::running)
    {
        return;   // _result stays nullopt — next tick() call retries immediately, no delay
    }

    switch (status)
    {
        case tick_result::succeeded: _result = task_result::succeeded; break;
        case tick_result::failed:    _result = task_result::failed;    break;
        case tick_result::cancelled: _result = task_result::cancelled; break;
        default: std::unreachable();   // tick_result::running already handled above
    }
}


auto ticking_task::get_status() -> std::optional<task_result>
{
    return _result;
}


auto ticking_task::cancel() -> void
{
    stop_source.request_stop();
}

}
