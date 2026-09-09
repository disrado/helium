#pragma once

#include "core/execution/task/task_base.hpp"

#include <chrono>
#include <optional>


namespace he::exec
{

class dispatcher;


class ticking_task final: public task_base
{
public:
    task_id id;
    ticking_definition definition;

    std::chrono::steady_clock::time_point trigger_point{};
    std::chrono::steady_clock::duration interval{};
    std::optional<std::size_t> repetitions_left{ 1 };

public:
    auto tick(dispatcher&) -> bool override;
};

}
