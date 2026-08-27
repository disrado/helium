#pragma once

#include "core/execution/dispatcher.hpp"
#include "core/singleton.hpp"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <stop_token>
#include <unordered_map>


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
    std::function<void(std::stop_token)> definition;
    std::function<void()> on_complete;
};


class scheduler final: public he::singleton<scheduler>
{
public:
    using task_id = int64_t;

    static constexpr task_id invalid_task_id{ 0 };

    ~scheduler() override;

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
    auto set_dispatcher(std::unique_ptr<dispatcher> new_dispatcher) -> void;

    auto post(task new_task) -> task_id;
    auto cancel(task_id id) -> bool;

    auto process() -> void;

private:
    auto next_task_id() -> task_id;

    auto queue_sync(task_id id, task new_task) -> void;
    auto queue_async(task_id id, task new_task) -> void;

    auto process_async() -> bool;
    auto process_sync() -> bool;

    auto token_for(task_id id) -> std::stop_token;

    auto run(task_id id, const std::function<void()>& action) -> void;

private:
    std::mutex _mutex;

    std::queue<queued_task> _queue;
    std::unordered_map<task_id, std::stop_source> _stop_sources;

    std::queue<completed_task> _completed;
    std::mutex _completed_mutex;

    std::atomic<int> _outstanding_async{ 0 };
    std::condition_variable _outstanding_cv;

    std::unique_ptr<dispatcher> _dispatcher{ std::make_unique<thread_dispatcher>() };

    std::atomic<task_id> _next_id{ invalid_task_id + 1 };
};

}
