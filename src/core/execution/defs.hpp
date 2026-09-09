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


enum class task_result : uint8_t
{
    succeeded,
    failed,
    cancelled
};


// The one place "not done yet" is a legal value — ticking definitions only. Every other boundary in
// the system (sync/async definitions, all three request types' on_complete, task_graph's broadcast
// type) uses task_result. Keeping `running` scoped to exactly this one type is deliberate: an earlier
// version let a completion-delivery type also carry `running`, which is dead weight everywhere it's
// actually consumed (nothing ever calls on_complete mid-flight) and reopens the exact "illegal state is
// representable" hole this redesign exists to close. Don't widen task_result to include `running`, and
// don't let tick_result leak past the one definition signature that needs it.
enum class tick_result : uint8_t
{
    running,
    succeeded,
    failed,
    cancelled
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


using task_definition = he::delegate<task_result(std::stop_token)>;
using task_completion = he::delegate<void(task_result)>; // shared by all three request types
using ticking_definition = he::delegate<tick_result(std::stop_token)>;

}
