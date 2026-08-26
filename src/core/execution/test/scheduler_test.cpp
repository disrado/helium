#include "core/execution/scheduler.hpp"

#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <thread>


TEST_CASE("scheduler sync task")
{
    SECTION("does not run before process")
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

    SECTION("definition and on_complete both run on process")
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

    SECTION("definition runs before on_complete")
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

    SECTION("multiple sync tasks run in submission order")
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

    SECTION("a task queued during process runs within the same process call")
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
    SECTION("on_complete does not run before process, even after work finishes")
    {
        auto on_complete_ran{ false };

        auto instance{ he::exec::scheduler{} };

        instance.post(
            he::exec::task{
                .mode = he::exec::task::type::async,
                .definition = [] {},
                .on_complete = [&on_complete_ran] { on_complete_ran = true; }
            });

        std::this_thread::sleep_for(std::chrono::milliseconds(50));

        REQUIRE_FALSE(on_complete_ran);
    }

    SECTION("on_complete runs after process once work has finished")
    {
        auto on_complete_ran{ false };

        auto instance{ he::exec::scheduler{} };

        instance.post(
            he::exec::task{
                .mode = he::exec::task::type::async,
                .definition = [] {},
                .on_complete = [&on_complete_ran] { on_complete_ran = true; }
            });

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        instance.process();

        REQUIRE(on_complete_ran);
    }

    SECTION("definition runs on a different thread than the caller")
    {
        auto worker_thread_id{ std::thread::id{} };

        auto instance{ he::exec::scheduler{} };

        instance.post(
            he::exec::task{
                .mode = he::exec::task::type::async,
                .definition = [&worker_thread_id] { worker_thread_id = std::this_thread::get_id(); },
                .on_complete = [] {}
            });

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        instance.process();

        REQUIRE(worker_thread_id != std::this_thread::get_id());
    }

    SECTION("on_complete runs on the calling thread, not the worker thread")
    {
        auto on_complete_thread_id{ std::thread::id{} };

        auto instance{ he::exec::scheduler{} };

        instance.post(
            he::exec::task{
                .mode = he::exec::task::type::async,
                .definition = [] {},
                .on_complete = [&on_complete_thread_id] { on_complete_thread_id = std::this_thread::get_id(); }
            });

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        instance.process();

        REQUIRE(on_complete_thread_id == std::this_thread::get_id());
    }

    SECTION("multiple async tasks all complete")
    {
        auto completed_count{ 0 };

        auto instance{ he::exec::scheduler{} };

        instance.post(he::exec::task{ .mode = he::exec::task::type::async, .definition = [] {}, .on_complete = [&completed_count] { completed_count++; } });
        instance.post(he::exec::task{ .mode = he::exec::task::type::async, .definition = [] {}, .on_complete = [&completed_count] { completed_count++; } });
        instance.post(he::exec::task{ .mode = he::exec::task::type::async, .definition = [] {}, .on_complete = [&completed_count] { completed_count++; } });

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        instance.process();

        REQUIRE(completed_count == 3);
    }
}


TEST_CASE("scheduler process")
{
    SECTION("returns immediately when nothing is queued")
    {
        auto instance{ he::exec::scheduler{} };

        instance.process();
    }

    SECTION("mixed sync and async tasks both resolve within one process call")
    {
        auto sync_ran{ false };
        auto async_completed{ false };

        auto instance{ he::exec::scheduler{} };

        instance.post(
            he::exec::task{
                .mode = he::exec::task::type::async,
                .definition = [] {},
                .on_complete = [&async_completed] { async_completed = true; }
            });
        instance.post(
            he::exec::task{
                .mode = he::exec::task::type::sync,
                .definition = [&sync_ran] { sync_ran = true; },
                .on_complete = [] {}
            });

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        instance.process();

        REQUIRE(sync_ran);
        REQUIRE(async_completed);
    }
}


TEST_CASE("scheduler cancel")
{
    SECTION("cancelled sync task does not run")
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

    SECTION("cancelled async task's on_complete does not run")
    {
        auto on_complete_ran{ false };

        auto instance{ he::exec::scheduler{} };

        const auto id{
            instance.post(
                he::exec::task{
                    .mode = he::exec::task::type::async,
                    .definition = [] {},
                    .on_complete = [&on_complete_ran] { on_complete_ran = true; }
                })
        };

        instance.cancel(id);

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        instance.process();

        REQUIRE_FALSE(on_complete_ran);
    }

    SECTION("cancelling one task does not affect another")
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
