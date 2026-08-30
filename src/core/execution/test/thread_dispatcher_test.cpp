#include "core/execution/thread_dispatcher.hpp"

#include <catch2/catch_test_macros.hpp>

#include <condition_variable>
#include <memory>
#include <mutex>


TEST_CASE("thread_dispatcher")
{
    SECTION("runs work")
    {
        auto instance{ he::exec::thread_dispatcher{} };

        auto mutex{ std::mutex{} };
        auto cv{ std::condition_variable{} };
        auto ran{ false };

        instance.dispatch(
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
        auto instance{ he::exec::thread_dispatcher{} };

        auto mutex{ std::mutex{} };
        auto cv{ std::condition_variable{} };
        auto completed{ 0 };
        constexpr auto work_count{ 20 };

        for (auto i{ 0 }; i < work_count; ++i)
        {
            instance.dispatch(
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

    SECTION("dispatches through base pointer")
    {
        auto instance{ std::unique_ptr<he::exec::dispatcher>{ std::make_unique<he::exec::thread_dispatcher>() } };

        auto mutex{ std::mutex{} };
        auto cv{ std::condition_variable{} };
        auto ran{ false };

        instance->dispatch(
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
}
