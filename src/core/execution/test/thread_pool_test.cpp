#include "core/execution/thread_pool.hpp"

#include <catch2/catch_test_macros.hpp>

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>


TEST_CASE("thread_pool")
{
    SECTION("runs work")
    {
        auto pool{ he::exec::thread_pool{ 2 } };

        auto mutex{ std::mutex{} };
        auto cv{ std::condition_variable{} };
        auto ran{ false };

        pool.submit(
            [&]
            {
                const auto _{ std::lock_guard{ mutex } };
                ran = true;
                cv.notify_one();
            });

        auto lock{ std::unique_lock{ mutex } };
        cv.wait(lock, [&] { return ran; });

        REQUIRE(ran);
    }

    SECTION("runs all work")
    {
        auto pool{ he::exec::thread_pool{ 4 } };

        auto mutex{ std::mutex{} };
        auto cv{ std::condition_variable{} };
        auto completed{ 0 };
        constexpr auto work_count{ 20 };

        for (auto i{ 0 }; i < work_count; ++i)
        {
            pool.submit(
                [&]
                {
                    const auto _{ std::lock_guard{ mutex } };
                    ++completed;
                    cv.notify_one();
                });
        }

        auto lock{ std::unique_lock{ mutex } };
        cv.wait(lock, [&] { return completed == work_count; });

        REQUIRE(completed == work_count);
    }

    SECTION("runs off caller thread")
    {
        auto pool{ he::exec::thread_pool{ 1 } };

        auto mutex{ std::mutex{} };
        auto cv{ std::condition_variable{} };
        auto done{ false };
        auto worker_id{ std::thread::id{} };

        pool.submit(
            [&]
            {
                const auto _{ std::lock_guard{ mutex } };
                worker_id = std::this_thread::get_id();
                done = true;
                cv.notify_one();
            });

        auto lock{ std::unique_lock{ mutex } };
        cv.wait(lock, [&] { return done; });

        REQUIRE(worker_id != std::this_thread::get_id());
    }

    SECTION("drains queue on destruction")
    {
        auto completed{ std::atomic<int>{ 0 } };
        constexpr auto work_count{ 50 };

        {
            auto pool{ he::exec::thread_pool{ 2 } };

            for (auto i{ 0 }; i < work_count; ++i)
            {
                pool.submit([&completed] { ++completed; });
            }
        }

        REQUIRE(completed == work_count);
    }
}
