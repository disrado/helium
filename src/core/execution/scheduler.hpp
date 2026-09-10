#pragma once

#include "core/execution/defs.hpp"
#include "core/execution/dispatcher.hpp"
#include "core/execution/task/task_base.hpp"
#include "core/execution/task_request.hpp"
#include "core/execution/thread_dispatcher.hpp"
#include "core/singleton.hpp"

#include <atomic>
#include <chrono>
#include <functional>
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

class scheduler: public he::singleton<scheduler>
{
private:
    struct scheduled_task
    {
        task_id id;

        std::shared_ptr<task_base> instance;

        std::optional<std::chrono::steady_clock::time_point> trigger_point;
        std::chrono::steady_clock::duration interval{};
        std::optional<std::size_t> repetitions_left{ 1 };

        task_completion on_complete;

        bool cancel_requested{ false };

        std::function<std::shared_ptr<task_base>()> create;
    };

public:
    ~scheduler() override;

    static auto create() -> std::shared_ptr<scheduler>;

    auto set_dispatcher(std::unique_ptr<dispatcher> new_dispatcher) -> void;
    auto get_dispatcher() -> std::shared_ptr<dispatcher>;

    auto post(sync_task_request request) -> task_id;
    auto post(async_task_request request) -> task_id;
    auto post(ticking_task_request request) -> task_id;

    auto tick() -> void;

    auto cancel(task_id id) -> bool;

protected:
    scheduler() = default;

private:
    auto next_task_id() -> task_id;

    auto acquire_task_instance(task_id id) -> std::shared_ptr<task_base>;
    auto finalize_task_instance(task_id id, task_result result) -> void;

    auto is_eligible_for_starting(const scheduled_task& task) const -> bool;
    auto is_cancelled_while_idle(task_id id) -> bool;

    auto drain() -> void;

private:
    std::unordered_map<task_id, scheduled_task> _tasks;
    std::mutex _mutex;

    bool _is_shutting_down{ false };

    std::atomic<std::shared_ptr<dispatcher>> _dispatcher{ std::make_shared<thread_dispatcher>() };
    std::atomic<task_id> _next_id{ invalid_task_id + 1 };

    // reusable buffer for tick(), because tasks can add/cancel themselves/other during tick()
    // cleared, not reconstructed, so capacity stabilizes over time
    std::vector<task_id> _snapshot;
};

}
