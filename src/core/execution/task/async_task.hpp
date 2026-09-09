#pragma once

#include "core/execution/task/task_base.hpp"

#include <atomic>
#include <chrono>
#include <memory>
#include <optional>


namespace he::exec
{

class dispatcher;


class async_task final: public task_base, public std::enable_shared_from_this<async_task>
{
public:
    task_id id;
    task_definition definition;

    std::atomic<bool> completed{ false };     // the only thing that's actually cross-thread — worker
                                               // thread writes it, main thread polls it in tick()
    task_result result{};

    std::chrono::steady_clock::time_point trigger_point{};
    std::chrono::steady_clock::duration interval{};
    std::optional<std::size_t> repetitions_left{ 1 };

public:
    auto dispatch(dispatcher& d) -> void;
    auto tick(dispatcher& d) -> bool override;
    auto deliver_if_finished() -> void override;
};

}
