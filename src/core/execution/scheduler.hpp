#pragma once

#include "core/execution/defs.hpp"
#include "core/execution/dispatcher.hpp"
#include "core/execution/task/task_base.hpp"
#include "core/execution/thread_dispatcher.hpp"
#include "core/singleton.hpp"

#include <atomic>
#include <chrono>
#include <memory>
#include <mutex>
#include <optional>
#include <stop_token>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>


namespace he::exec
{

template <typename callable_t>
    requires std::is_invocable_r_v<bool, callable_t> || std::is_invocable_r_v<task_result, callable_t, std::stop_token>
auto make_task_definition(callable_t fn) -> task_definition
{
    if constexpr (std::is_invocable_r_v<task_result, callable_t, std::stop_token>)
    {
        return task_definition{ std::move(fn) };
    }
    else
    {
        return task_definition{ [fn{ std::move(fn) }] (std::stop_token token) mutable -> task_result
        {
            if (fn())
            {
                return task_result::succeeded;
            }

            return token.stop_requested() ? task_result::cancelled : task_result::failed;
        } };
    }
}


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

    std::chrono::steady_clock::duration initial_delay{};
    std::chrono::steady_clock::duration interval{};
    std::optional<std::size_t> repetitions{ 1 }; // nullopt for perpetual repeats
};


struct ticking_task_request final
{
public:
    ticking_definition definition;
    task_completion on_complete;

    std::chrono::steady_clock::duration initial_delay{};
    std::chrono::steady_clock::duration interval{};
    std::optional<std::size_t> repetitions{ 1 };
};


class scheduler: public he::singleton<scheduler>
{
public:
    ~scheduler() override;

    static auto create() -> std::shared_ptr<scheduler>;

    auto set_dispatcher(std::unique_ptr<dispatcher> new_dispatcher) -> void;

    auto post(sync_task_request request) -> task_id;
    auto post(async_task_request request) -> task_id;
    auto post(ticking_task_request request) -> task_id;

    auto cancel(task_id id) -> bool;
    auto process() -> void;

protected:
    scheduler() = default;

private:
    auto next_task_id() -> task_id;

private:
    std::unordered_map<task_id, std::shared_ptr<task_base>> _tasks;
    std::mutex _mutex;

    bool _is_shutting_down{ false };

    std::atomic<std::shared_ptr<dispatcher>> _dispatcher{ std::make_shared<thread_dispatcher>() };
    std::atomic<task_id> _next_id{ invalid_task_id + 1 };

    // reusable buffer for task processing, because tasks can add/cancel themselves/other tasks
    // cleared, not reconstructed, so capacity stabilizes over time
    std::vector<std::pair<task_id, std::shared_ptr<task_base>>> _snapshot;
};

}
