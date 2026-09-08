#pragma once

#include "core/delegate/delegate.hpp"

#include <any>
#include <cstdint>
#include <map>
#include <stop_token>
#include <string>
#include <type_traits>


namespace he::exec
{

class basic_action;

}


namespace he::exec
{

template <typename t>
concept action_like = std::is_base_of_v<basic_action, t>;


using task_id = int64_t;


static constexpr task_id invalid_task_id{ 0 };


enum class launch_policy : uint8_t
{
    // runs immediately, during post()
    sync,

    // dispatched to a worker thread
    async,

    // runs on the next process() call
    next_frame,

    // iterates through task every tick
    tick
};


enum class task_phase : uint8_t
{
    queued,
    running,
    completed
};


enum class execution_status : uint8_t
{
    completed,
    cancelled,
    faulted,

    // not done, call me again — only legal for launch_policy::tick
    running
};


enum class action_state : uint8_t
{
    dormant,
    running,
    succeeded,
    failed,
    cancelled
};


using action_context = std::map<std::string, std::any>;


using task_definition = he::delegate<execution_status(std::stop_token)>;
using task_completion = he::delegate<void(execution_status)>;

}
