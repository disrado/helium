#include "core/execution/scheduler.hpp"

#include <catch2/catch_test_macros.hpp>

#include <atomic>
#include <thread>


TEST_CASE("scheduler sync task")
{
    SECTION("waits for process")
    {
        auto ran{ false };

        auto instance{ he::exec::scheduler{} };

        instance.post(
            he::exec::task{
                .mode = he::exec::task::type::sync,
                .definition = [&ran] { ran = true; },
                .on_complete = [] {}
            });

        REQUIRE_FALSE(ran);
    }

    SECTION("runs definition and on_complete")
    {
        auto definition_ran{ false };
        auto on_complete_ran{ false };

        auto instance{ he::exec::scheduler{} };

        instance.post(
            he::exec::task{
                .mode = he::exec::task::type::sync,
                .definition = [&definition_ran] { definition_ran = true; },
                .on_complete = [&on_complete_ran] { on_complete_ran = true; }
            });

        instance.process();

        REQUIRE(definition_ran);
        REQUIRE(on_complete_ran);
    }

    SECTION("runs definition before on_complete")
    {
        auto order{ std::string{} };

        auto instance{ he::exec::scheduler{} };

        instance.post(
            he::exec::task{
                .mode = he::exec::task::type::sync,
                .definition = [&order] { order += "a"; },
                .on_complete = [&order] { order += "b"; }
            });

        instance.process();

        REQUIRE(order == "ab");
    }

    SECTION("runs in submission order")
    {
        auto order{ std::string{} };

        auto instance{ he::exec::scheduler{} };

        instance.post(
            he::exec::task{
                .mode = he::exec::task::type::sync,
                .definition = [&order] { order += "1"; },
                .on_complete = [] {}
            });
        instance.post(
            he::exec::task{
                .mode = he::exec::task::type::sync,
                .definition = [&order] { order += "2"; },
                .on_complete = [] {}
            });

        instance.process();

        REQUIRE(order == "12");
    }

    SECTION("re-queued task resolves same call")
    {
        auto order{ std::string{} };

        auto instance{ he::exec::scheduler{} };

        instance.post(
            he::exec::task{
                .mode = he::exec::task::type::sync,
                .definition =
                [&order, &instance]
                {
                    order += "a";
                    instance.post(
                        he::exec::task{
                            .mode = he::exec::task::type::sync,
                            .definition = [&order] { order += "b"; },
                            .on_complete = [] {}
                        });
                },
                .on_complete = [] {}
            });

        instance.process();

        REQUIRE(order == "ab");
    }
}


TEST_CASE("scheduler async task")
{
    SECTION("waits for process")
    {
        auto work_done{ std::atomic<bool>{ false } };
        auto on_complete_ran{ false };

        auto instance{ he::exec::scheduler{} };

        instance.post(
            he::exec::task{
                .mode = he::exec::task::type::async,
                .definition = [&work_done] { work_done = true; },
                .on_complete = [&on_complete_ran] { on_complete_ran = true; }
            });

        while (!work_done) {}

        REQUIRE_FALSE(on_complete_ran);
    }

    SECTION("runs on_complete after process")
    {
        auto work_done{ std::atomic<bool>{ false } };
        auto on_complete_ran{ false };

        auto instance{ he::exec::scheduler{} };

        instance.post(
            he::exec::task{
                .mode = he::exec::task::type::async,
                .definition = [&work_done] { work_done = true; },
                .on_complete = [&on_complete_ran] { on_complete_ran = true; }
            });

        while (!work_done) {}
        instance.process();

        REQUIRE(on_complete_ran);
    }

    SECTION("runs on a worker thread")
    {
        auto work_done{ std::atomic<bool>{ false } };
        auto worker_thread_id{ std::thread::id{} };

        auto instance{ he::exec::scheduler{} };

        instance.post(
            he::exec::task{
                .mode = he::exec::task::type::async,
                .definition =
                [&worker_thread_id, &work_done]
                {
                    worker_thread_id = std::this_thread::get_id();
                    work_done = true;
                },
                .on_complete = [] {}
            });

        while (!work_done) {}
        instance.process();

        REQUIRE(worker_thread_id != std::this_thread::get_id());
    }

    SECTION("runs on_complete on the calling thread")
    {
        auto work_done{ std::atomic<bool>{ false } };
        auto on_complete_thread_id{ std::thread::id{} };

        auto instance{ he::exec::scheduler{} };

        instance.post(
            he::exec::task{
                .mode = he::exec::task::type::async,
                .definition = [&work_done] { work_done = true; },
                .on_complete = [&on_complete_thread_id] { on_complete_thread_id = std::this_thread::get_id(); }
            });

        while (!work_done) {}
        instance.process();

        REQUIRE(on_complete_thread_id == std::this_thread::get_id());
    }

    SECTION("multiple tasks all complete")
    {
        auto work_done_count{ std::atomic<int>{ 0 } };
        auto completed_count{ 0 };

        auto instance{ he::exec::scheduler{} };

        instance.post(
            he::exec::task{
                .mode = he::exec::task::type::async,
                .definition = [&work_done_count] { work_done_count++; },
                .on_complete = [&completed_count] { completed_count++; }
            });
        instance.post(
            he::exec::task{
                .mode = he::exec::task::type::async,
                .definition = [&work_done_count] { work_done_count++; },
                .on_complete = [&completed_count] { completed_count++; }
            });
        instance.post(
            he::exec::task{
                .mode = he::exec::task::type::async,
                .definition = [&work_done_count] { work_done_count++; },
                .on_complete = [&completed_count] { completed_count++; }
            });

        while (work_done_count != 3) {}
        instance.process();

        REQUIRE(completed_count == 3);
    }
}


TEST_CASE("scheduler process")
{
    SECTION("returns when empty")
    {
        auto instance{ he::exec::scheduler{} };

        instance.process();
    }

    SECTION("mixed tasks resolve in one call")
    {
        auto work_done{ std::atomic<bool>{ false } };
        auto sync_ran{ false };
        auto async_completed{ false };

        auto instance{ he::exec::scheduler{} };

        instance.post(
            he::exec::task{
                .mode = he::exec::task::type::async,
                .definition = [&work_done] { work_done = true; },
                .on_complete = [&async_completed] { async_completed = true; }
            });
        instance.post(
            he::exec::task{
                .mode = he::exec::task::type::sync,
                .definition = [&sync_ran] { sync_ran = true; },
                .on_complete = [] {}
            });

        while (!work_done) {}
        instance.process();

        REQUIRE(sync_ran);
        REQUIRE(async_completed);
    }
}


TEST_CASE("scheduler cancel")
{
    SECTION("skips cancelled sync task")
    {
        auto ran{ false };

        auto instance{ he::exec::scheduler{} };

        const auto id{
            instance.post(
                he::exec::task{
                    .mode = he::exec::task::type::sync,
                    .definition = [&ran] { ran = true; },
                    .on_complete = [] {}
                })
        };

        instance.cancel(id);
        instance.process();

        REQUIRE_FALSE(ran);
    }

    SECTION("skips cancelled async on_complete")
    {
        auto work_done{ std::atomic<bool>{ false } };
        auto on_complete_ran{ false };

        auto instance{ he::exec::scheduler{} };

        const auto id{
            instance.post(
                he::exec::task{
                    .mode = he::exec::task::type::async,
                    .definition = [&work_done] { work_done = true; },
                    .on_complete = [&on_complete_ran] { on_complete_ran = true; }
                })
        };

        instance.cancel(id);

        while (!work_done) {}
        instance.process();

        REQUIRE_FALSE(on_complete_ran);
    }

    SECTION("cancel is isolated per task")
    {
        auto first_ran{ false };
        auto second_ran{ false };

        auto instance{ he::exec::scheduler{} };

        const auto first_id{
            instance.post(
                he::exec::task{
                    .mode = he::exec::task::type::sync,
                    .definition = [&first_ran] { first_ran = true; },
                    .on_complete = [] {}
                })
        };
        instance.post(
            he::exec::task{
                .mode = he::exec::task::type::sync,
                .definition = [&second_ran] { second_ran = true; },
                .on_complete = [] {}
            });

        instance.cancel(first_id);
        instance.process();

        REQUIRE_FALSE(first_ran);
        REQUIRE(second_ran);
    }
}
