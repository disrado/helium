#pragma once

#include "core/execution/defs.hpp"

#include <cstdint>
#include <stop_token>


namespace he::exec
{

class dispatcher;


class task_base
{
public:
    virtual ~task_base() = default;

    virtual auto tick(dispatcher& d) -> bool = 0;          // true = done, safe to erase from _tasks

    // called only by ~scheduler(), once per entry, before request_cancel() — lets an entry whose work
    // already finished (result computed, just never yet observed by a tick()/process() call) deliver
    // that real result instead of being silently dropped. Default no-op: only async_task has a
    // "done but unobserved" state to begin with — ticking_task's work runs synchronously inside tick()
    // itself, so there's nothing to flush without re-executing the definition, which shutdown must not
    // do (that would run more of the user's actual work at a surprising, undocumented time).
    virtual auto deliver_if_finished() -> void {}

    auto request_cancel() -> bool;                         // shared by every task type; not virtual,
                                                            // nothing overrides it

    task_completion on_complete;
    std::stop_source stop_source;

protected:
    // dormant   — genuinely idle: before the first invocation of a cycle, or waiting out `interval`
    //             between one finished cycle and the next
    // running   — actively executing this cycle's work, not yet terminal: for async_task, dispatched to
    //             a worker and not yet observed complete; for ticking_task, from the first
    //             tick_result::running through to a terminal result — spans however many process()
    //             calls that takes; also covers the window where this entry's own on_complete/definition
    //             is on the stack — a reentrant self-cancel from either just flags the stop token, it
    //             never mutates phase directly, so no cross-thread cancel can race a phase write here
    // finished  — terminal, safe to erase, idempotent no-op for both tick() and request_cancel()
    enum class phase : uint8_t { dormant, running, finished };

    phase _phase{ phase::dormant };
};

}
