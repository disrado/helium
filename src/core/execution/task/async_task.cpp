#include "async_task.hpp"

#include "core/execution/dispatcher.hpp"

#include <tuple>
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

auto async_task::dispatch(dispatcher& d) -> void
{
    _phase = phase::running;

    d.dispatch([self{ shared_from_this() }, token{ stop_source.get_token() }] () mutable
    {
        self->result = invoke_definition(token, self->definition);
        self->completed.store(true, std::memory_order_release);
    });
}


auto async_task::tick(dispatcher& d) -> bool
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

            dispatch(d);

            return false;
        }

        case phase::running:
        {
            if (!completed.load(std::memory_order_acquire))
            {
                return false;
            }

            const auto exhausted{ repetitions_left && --repetitions_left.value() == 0 };
            const auto stop_requested_before{ stop_source.get_token().stop_requested() };

            std::ignore = on_complete.try_execute(result);

            // on_complete may self-cancel us reentrantly (calls scheduler::cancel(id)) — that only
            // flags the stop token now, never phase directly, so a freshly-raised flag here means
            // exactly that happened
            const auto cancelled_during_delivery{ !stop_requested_before && stop_source.get_token().stop_requested() };

            if (exhausted || result == task_result::cancelled || cancelled_during_delivery)
            {
                _phase = phase::finished;

                return true;
            }

            trigger_point = std::chrono::steady_clock::now() + interval;
            completed.store(false, std::memory_order_relaxed);   // safe: previous worker already exited before we got here
            _phase = phase::dormant;

            return false;
        }
    }

    std::unreachable();
}


auto async_task::deliver_if_finished() -> void
{
    if (_phase != phase::running || !completed.load(std::memory_order_acquire))
    {
        return;   // still genuinely in flight, or already dealt with — nothing to flush
    }

    std::ignore = on_complete.try_execute(result);

    _phase = phase::finished;   // shutdown: deliver the one known result, never reschedule another cycle;
                                // a reentrant self-cancel from on_complete just flags the stop token —
                                // harmless here, we're already ending this entry unconditionally
}

}
