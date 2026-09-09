#include "task_base.hpp"

#include <tuple>
#include <utility>


namespace he::exec
{

auto task_base::request_cancel() -> bool
{
    switch (_phase)
    {
        case phase::finished:
            return true;

        case phase::running:
            stop_source.request_stop();

            return false;   // in-flight, mid-cycle, or a reentrant self-cancel from inside our own
                            // on_complete/definition: defer, let tick()/deliver_if_finished() observe
                            // the stop and settle the phase themselves

        case phase::dormant:
        {
            _phase = phase::finished;
            stop_source.request_stop();
            std::ignore = on_complete.try_execute(task_result::cancelled);

            return true;   // genuinely idle — nothing in-progress to interrupt, safe synchronously
        }
    }

    std::unreachable();
}

}
