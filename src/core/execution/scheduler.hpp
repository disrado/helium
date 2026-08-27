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
        // runs immediately, during post()
        sync,

        // runs on the next process() call
        next_frame,

        // dispatched to a worker thread
        async
    };

    enum class status : uint8_t
    {
        running,
        completed,
        cancelled
    };

public:
    type mode;
    std::function<status(std::stop_token)> definition;
    std::function<void(status)> on_complete;
};


class scheduler final: public he::singleton<scheduler>
{
public:
    using task_id = int64_t;

private:
    struct completed_task final
    {
        task_id id;

        task::status status;
        std::function<void(task::status)> on_complete;
    };

    struct queued_task final
    {
        task_id id;

        task target;
    };

    using task_definition = std::function<task::status(std::stop_token)>;

public:
    ~scheduler() override;

    auto set_dispatcher(std::unique_ptr<dispatcher> new_dispatcher) -> void;

    auto post(task new_task) -> task_id;
    auto cancel(task_id id) -> bool;

    auto process() -> void;

private:
    auto next_task_id() -> task_id;

    auto dispatch_async(task_id id, task new_task) -> void;
    auto queue_next_frame(task_id id, task new_task) -> void;

    auto process_async() -> bool;
    auto process_next_frame() -> bool;

    auto token_for(task_id id) -> std::stop_token;

    auto run_task(task_id id, task target) -> void;
    auto invoke_definition(const std::stop_token& token, const task_definition& definition) -> task::status;
    auto process_task_completion(task_id id, task::status status, const std::function<void(task::status)>& on_complete) -> void;
    auto signal_async_complete() -> void;

public:
    static constexpr task_id invalid_task_id{ 0 };

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
