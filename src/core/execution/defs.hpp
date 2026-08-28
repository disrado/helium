#pragma once

#include <cstdint>


namespace he::exec
{

using task_id = int64_t;

inline constexpr task_id invalid_task_id{ 0 };

enum class launch_policy : uint8_t
{
    // runs immediately, during post()
    sync,

    // runs on the next process() call
    next_frame,

    // dispatched to a worker thread
    async
};

enum class execution_status : uint8_t
{
    completed,
    cancelled
};

}
