#pragma once

#include "core/execution/dispatcher.hpp"
#include "core/singleton.hpp"

#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <unordered_set>


namespace he::exec
{

struct task final
{
public:
    enum class type : uint8_t
    {
        sync,
        async
    };

public:
    type mode;
    std::function<void()> definition;
    std::function<void()> on_complete;
};


class scheduler final: public he::singleton<scheduler>
{
public:
    using task_id = int64_t;

private:
    struct completed_task final
    {
        task_id id;

        std::function<void()> on_complete;
    };

    struct queued_task final
    {
        task_id id;

        task target;
    };

public:
    auto post(task new_task) -> task_id;
    auto cancel(task_id id) -> void;

    auto process() -> void;

    auto set_dispatcher(std::unique_ptr<dispatcher> new_dispatcher) -> void;

private:
    auto next_task_id() -> task_id;

    auto queue_sync(task_id id, task new_task) -> void;
    auto queue_async(task_id id, task new_task) -> void;

    auto process_async() -> bool;
    auto process_sync() -> bool;

    auto run(task_id id, const std::function<void()>& action) -> void;

private:
    std::queue<queued_task> _queue;

    std::queue<completed_task> _completed;
    std::mutex _completed_mutex;

    std::unordered_set<task_id> _cancelled;

    std::atomic<task_id> _next_id{ 0 };

    std::unique_ptr<dispatcher> _dispatcher{ std::make_unique<thread_dispatcher>() };
};

}
