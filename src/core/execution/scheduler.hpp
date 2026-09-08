#pragma once

#include "core/execution/defs.hpp"
#include "core/execution/dispatcher.hpp"
#include "core/execution/thread_dispatcher.hpp"
#include "core/singleton.hpp"

#include <moodycamel/concurrentqueue.h>

#include <array>
#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <stop_token>
#include <unordered_map>


namespace he::exec
{

struct task_request final
{
public:
    launch_policy mode;
    task_definition definition;
    task_completion on_complete;
};


struct task final
{
public:


public:
    launch_policy mode;

    task_definition definition;
    task_completion on_complete;

    std::stop_source stop_source;

    std::atomic<task_phase> phase{ task_phase::queued };

    execution_status result{ execution_status::completed };
};


class scheduler: public he::singleton<scheduler>, public std::enable_shared_from_this<scheduler>
{
private:
    // resolves issue of producing tasks during queue processing without locking
    class double_buffered_queue final
    {
    public:
        auto enqueue(task_id id) -> void;
        auto drain_active(const std::function<void(task_id)>& handler) -> void;
        auto drain(const std::function<void(task_id)>& handler) -> void;

    private:
        std::array<moodycamel::ConcurrentQueue<task_id>, 2> _queues;
        std::atomic<int> _active{ 0 };
    };

protected:
    scheduler() = default;

public:
    static auto create() -> std::shared_ptr<scheduler>;

    ~scheduler() override;

    auto set_dispatcher(std::unique_ptr<dispatcher> new_dispatcher) -> void;

    auto post(task_request request) -> task_id;
    auto cancel(task_id id) -> bool;

    auto process() -> void;

private:
    auto next_task_id() -> task_id;

    auto dispatch_async(task_id id, std::shared_ptr<task> record) -> void;
    auto run_record(task_id id, const std::shared_ptr<task>& record) -> void;
    auto deliver(task_id id, const std::shared_ptr<task>& record) -> void;
    auto process_one(task_id id) -> void;

    auto find_record(task_id id) -> std::shared_ptr<task>;
    auto drain() -> void;

public:
    static constexpr task_id invalid_task_id{ he::exec::invalid_task_id };

private:
    double_buffered_queue _queue;

    std::unordered_map<task_id, std::shared_ptr<task>> _tasks;
    std::mutex _tasks_mutex;
    bool _is_shutting_down{ false };

    std::atomic<std::shared_ptr<dispatcher>> _dispatcher{ std::make_shared<thread_dispatcher>() };

    std::atomic<task_id> _next_id{ invalid_task_id + 1 };
};

}
