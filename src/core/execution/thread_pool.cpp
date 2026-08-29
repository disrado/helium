#include "thread_pool.hpp"


namespace he::exec
{

thread_pool::thread_pool(std::size_t worker_count)
{
    for (std::size_t i{ 0 }; i < worker_count; ++i)
    {
        _workers.emplace_back([this] (std::stop_token token) { worker_loop(token); });
    }
}


auto thread_pool::submit(std::function<void()> work) -> void
{
    {
        const auto _{ std::lock_guard{ _mutex } };

        _queue.push_back(std::move(work));
    }

    _cv.notify_one();
}


auto thread_pool::worker_loop(std::stop_token token) -> void
{
    while (true)
    {
        auto work{ std::function<void()>{} };

        {
            auto lock{ std::unique_lock{ _mutex } };

            if (!_cv.wait(lock, token, [this] { return !_queue.empty(); }))
            {
                return;
            }

            work = std::move(_queue.front());

            _queue.pop_front();
        }

        work();
    }
}

}
