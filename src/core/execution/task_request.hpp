#pragma once

#include "core/execution/defs.hpp"

#include <chrono>
#include <optional>


namespace he::exec
{

struct sync_task_request final
{
public:
    task_definition definition;
    task_completion on_complete;
};


struct async_task_request final
{
public:
    task_definition definition;
    task_completion on_complete;

    std::chrono::steady_clock::duration delay{};
    std::chrono::steady_clock::duration interval{};
    std::optional<std::size_t> repetitions{ 1 }; // nullopt for perpetual repeats
};


struct ticking_task_request final
{
public:
    ticking_definition definition;
    task_completion on_complete;

    std::chrono::steady_clock::duration delay{};
    std::chrono::steady_clock::duration interval{};
    std::optional<std::size_t> repetitions{ 1 };
};

}
