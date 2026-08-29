#pragma once

#include <condition_variable>
#include <deque>
#include <functional>
#include <mutex>
#include <stop_token>
#include <thread>
#include <vector>


namespace he::exec
{

class thread_pool final
{
public:
    explicit thread_pool(std::size_t worker_count);

    auto submit(std::function<void()> work) -> void;

private:
    auto worker_loop(std::stop_token token) -> void;

private:
    std::deque<std::function<void()>> _queue;
    std::mutex _mutex;

    std::condition_variable_any _cv;

    // must be declared last: jthread's destructor calls request_stop() then joins, and the
    // stop_callback that wakes a sleeping worker needs _mutex/_cv to still be alive when it fires
    std::vector<std::jthread> _workers;
};

}
