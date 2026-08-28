#pragma once

#include "core/execution/defs.hpp"
#include "core/execution/dispatcher.hpp"
#include "core/singleton.hpp"

#include <moodycamel/concurrentqueue.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <stop_token>
#include <unordered_map>


namespace he::exec
{

struct task final
{
public:
    launch_policy mode;
    std::function<void(std::stop_token)> definition;
    std::function<void(execution_status)> on_complete;

    task_id id{ invalid_task_id };
    execution_status status{ execution_status::completed };
};

struct task_request final
{
public:
    launch_policy mode;
    std::function<void(std::stop_token)> definition;
    std::function<void(execution_status)> on_complete;
};


class scheduler final: public he::singleton<scheduler>
{
private:
    using task_definition = std::function<void(std::stop_token)>;

public:
    ~scheduler() override;

    auto set_dispatcher(std::unique_ptr<dispatcher> new_dispatcher) -> void;

    auto post(task_request request) -> task_id;
    auto cancel(task_id id) -> bool;

    auto process() -> void;

private:
    auto next_task_id() -> task_id;

    auto dispatch_async(task new_task) -> void;
    auto queue_next_frame(task new_task) -> void;

    auto process_queue() -> bool;

    auto token_for(task_id id) -> std::stop_token;

    auto run_task(task target) -> void;
    auto invoke_definition(const std::stop_token& token, const task_definition& definition) -> execution_status;
    auto process_task_completion(task_id id, execution_status status, const std::function<void(execution_status)>& on_complete) -> void;
    auto signal_async_complete() -> void;

public:
    static constexpr task_id invalid_task_id{ he::exec::invalid_task_id };

private:
    moodycamel::ConcurrentQueue<task> _queue;

    std::unordered_map<task_id, std::stop_source> _stop_sources;
    std::mutex _stop_sources_mutex;

    std::atomic<int> _outstanding_async{ 0 };

    std::mutex _shutdown_mutex;
    std::condition_variable _shutdown_cv;

    std::atomic<std::shared_ptr<dispatcher>> _dispatcher{ std::make_shared<thread_dispatcher>() };

    std::atomic<task_id> _next_id{ invalid_task_id + 1 };
};

}
