#include "ticking_task.hpp"

#include <tuple>
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

auto ticking_task::tick(dispatcher&) -> bool
{
    switch (_phase)
    {
        case phase::finished:
            return true;

        case phase::dormant:
        {
            if (std::chrono::steady_clock::now() < trigger_point)
            {
                return false;
            }

            _phase = phase::running;

            break;
        }

        case phase::running:
            break;
    }

    const auto status{ invoke_definition(stop_source.get_token(), definition) };

    if (status == tick_result::running)
    {
        return false;   // stays `running` — next process() call retries immediately, no delay; interval
                        // only applies between a terminal result and the next cycle
    }

    task_result terminal{};

    switch (status)
    {
        case tick_result::succeeded: terminal = task_result::succeeded; break;
        case tick_result::failed:    terminal = task_result::failed;    break;
        case tick_result::cancelled: terminal = task_result::cancelled; break;
        default: std::unreachable();   // tick_result::running already handled above
    }

    const auto exhausted{ repetitions_left && --repetitions_left.value() == 0 };
    const auto stop_requested_before{ stop_source.get_token().stop_requested() };

    std::ignore = on_complete.try_execute(terminal);

    // on_complete may self-cancel us reentrantly (calls scheduler::cancel(id)) — that only flags the
    // stop token now, never phase directly, so a freshly-raised flag here means exactly that happened
    const auto cancelled_during_delivery{ !stop_requested_before && stop_source.get_token().stop_requested() };

    if (exhausted || terminal == task_result::cancelled || cancelled_during_delivery)
    {
        _phase = phase::finished;

        return true;
    }

    trigger_point = std::chrono::steady_clock::now() + interval;
    _phase = phase::dormant;

    return false;
}

}
