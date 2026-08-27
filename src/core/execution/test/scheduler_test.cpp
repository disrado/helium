#include "core/execution/scheduler.hpp"

#include <catch2/catch_test_macros.hpp>

#include <atomic>
#include <thread>


namespace
{

class recording_dispatcher final: public he::exec::dispatcher
{
public:
    auto dispatch(std::function<void()> work) -> void override
    {
        dispatched = true;

        work();
    }

    bool dispatched{ false };
};

}


TEST_CASE("scheduler sync task")
{
    SECTION("waits")
    {
        auto ran{ false };

        auto instance{ he::exec::scheduler{} };

        instance.post(
            he::exec::task{
                .mode = he::exec::task::type::sync,
                .definition = [&ran](std::stop_token) { ran = true; },
                .on_complete = [] {}
            });

        REQUIRE_FALSE(ran);
    }

    SECTION("runs both")
    {
        auto definition_ran{ false };
        auto on_complete_ran{ false };

        auto instance{ he::exec::scheduler{} };

        instance.post(
            he::exec::task{
                .mode = he::exec::task::type::sync,
                .definition = [&definition_ran](std::stop_token) { definition_ran = true; },
                .on_complete = [&on_complete_ran] { on_complete_ran = true; }
            });

        instance.process();

        REQUIRE(definition_ran);
        REQUIRE(on_complete_ran);
    }

    SECTION("definition before on_complete")
    {
        auto order{ std::string{} };

        auto instance{ he::exec::scheduler{} };

        instance.post(
            he::exec::task{
                .mode = he::exec::task::type::sync,
                .definition = [&order](std::stop_token) { order += "a"; },
                .on_complete = [&order] { order += "b"; }
            });

        instance.process();

        REQUIRE(order == "ab");
    }

    SECTION("submission order")
    {
        auto order{ std::string{} };

        auto instance{ he::exec::scheduler{} };

        instance.post(
            he::exec::task{
                .mode = he::exec::task::type::sync,
                .definition = [&order](std::stop_token) { order += "1"; },
                .on_complete = [] {}
            });
        instance.post(
            he::exec::task{
                .mode = he::exec::task::type::sync,
                .definition = [&order](std::stop_token) { order += "2"; },
                .on_complete = [] {}
            });

        instance.process();

        REQUIRE(order == "12");
    }

    SECTION("re-queue resolves same call")
    {
        auto order{ std::string{} };

        auto instance{ he::exec::scheduler{} };

        instance.post(
            he::exec::task{
                .mode = he::exec::task::type::sync,
                .definition =
                    [&order, &instance](std::stop_token)
                {
                    order += "a";
                    instance.post(
                        he::exec::task{
                            .mode = he::exec::task::type::sync,
                            .definition = [&order](std::stop_token) { order += "b"; },
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
    SECTION("waits")
    {
        auto work_done{ std::atomic<bool>{ false } };
        auto on_complete_ran{ false };

        auto instance{ he::exec::scheduler{} };

        instance.post(
            he::exec::task{
                .mode = he::exec::task::type::async,
                .definition = [&work_done](std::stop_token) { work_done = true; },
                .on_complete = [&on_complete_ran] { on_complete_ran = true; }
            });

        while (!work_done) {}

        REQUIRE_FALSE(on_complete_ran);
    }

    SECTION("on_complete after process")
    {
        auto on_complete_ran{ false };

        auto instance{ he::exec::scheduler{} };

        instance.post(
            he::exec::task{
                .mode = he::exec::task::type::async,
                .definition = [](std::stop_token) {},
                .on_complete = [&on_complete_ran] { on_complete_ran = true; }
            });

        while (!on_complete_ran)
        {
            instance.process();
        }

        REQUIRE(on_complete_ran);
    }

    SECTION("worker thread")
    {
        auto work_done{ std::atomic<bool>{ false } };
        auto worker_thread_id{ std::thread::id{} };

        auto instance{ he::exec::scheduler{} };

        instance.post(
            he::exec::task{
                .mode = he::exec::task::type::async,
                .definition =
                    [&worker_thread_id, &work_done](std::stop_token)
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

    SECTION("on_complete on calling thread")
    {
        auto on_complete_ran{ false };
        auto on_complete_thread_id{ std::thread::id{} };

        auto instance{ he::exec::scheduler{} };

        instance.post(
            he::exec::task{
                .mode = he::exec::task::type::async,
                .definition = [](std::stop_token) {},
                .on_complete =
                    [&on_complete_ran, &on_complete_thread_id]
                {
                    on_complete_thread_id = std::this_thread::get_id();
                    on_complete_ran = true;
                }
            });

        while (!on_complete_ran)
        {
            instance.process();
        }

        REQUIRE(on_complete_thread_id == std::this_thread::get_id());
    }

    SECTION("multiple complete")
    {
        auto completed_count{ 0 };

        auto instance{ he::exec::scheduler{} };

        instance.post(he::exec::task{ .mode = he::exec::task::type::async, .definition = [](std::stop_token) {}, .on_complete = [&completed_count] { completed_count++; } });
        instance.post(he::exec::task{ .mode = he::exec::task::type::async, .definition = [](std::stop_token) {}, .on_complete = [&completed_count] { completed_count++; } });
        instance.post(he::exec::task{ .mode = he::exec::task::type::async, .definition = [](std::stop_token) {}, .on_complete = [&completed_count] { completed_count++; } });

        while (completed_count != 3)
        {
            instance.process();
        }

        REQUIRE(completed_count == 3);
    }
}


TEST_CASE("scheduler set_dispatcher")
{
    SECTION("used for async tasks")
    {
        auto instance{ he::exec::scheduler{} };

        auto owned_dispatcher{ std::make_unique<recording_dispatcher>() };
        const auto* dispatcher{ owned_dispatcher.get() };

        instance.set_dispatcher(std::move(owned_dispatcher));

        auto on_complete_ran{ false };

        instance.post(
            he::exec::task{
                .mode = he::exec::task::type::async,
                .definition = [](std::stop_token) {},
                .on_complete = [&on_complete_ran] { on_complete_ran = true; }
            });

        while (!on_complete_ran)
        {
            instance.process();
        }

        REQUIRE(dispatcher->dispatched);
    }
}


TEST_CASE("scheduler process")
{
    SECTION("empty is a no-op")
    {
        auto instance{ he::exec::scheduler{} };

        instance.process();
    }

    SECTION("mixed resolve in one call")
    {
        auto sync_ran{ false };
        auto async_completed{ false };

        auto instance{ he::exec::scheduler{} };

        instance.post(
            he::exec::task{
                .mode = he::exec::task::type::async,
                .definition = [](std::stop_token) {},
                .on_complete = [&async_completed] { async_completed = true; }
            });
        instance.post(
            he::exec::task{
                .mode = he::exec::task::type::sync,
                .definition = [&sync_ran](std::stop_token) { sync_ran = true; },
                .on_complete = [] {}
            });

        while (!async_completed)
        {
            instance.process();
        }

        REQUIRE(sync_ran);
        REQUIRE(async_completed);
    }
}


TEST_CASE("scheduler cancel")
{
    SECTION("false for unposted id")
    {
        auto instance{ he::exec::scheduler{} };

        REQUIRE_FALSE(instance.cancel(he::exec::scheduler::invalid_task_id));
        REQUIRE_FALSE(instance.cancel(he::exec::scheduler::task_id{ 12345 }));
    }

    SECTION("true before it runs")
    {
        auto instance{ he::exec::scheduler{} };

        const auto id{
            instance.post(
                he::exec::task{
                    .mode = he::exec::task::type::sync,
                    .definition = [](std::stop_token) {},
                    .on_complete = [] {}
                })
        };

        REQUIRE(instance.cancel(id));
    }

    SECTION("false after processed")
    {
        auto instance{ he::exec::scheduler{} };

        const auto id{
            instance.post(
                he::exec::task{
                    .mode = he::exec::task::type::sync,
                    .definition = [](std::stop_token) {},
                    .on_complete = [] {}
                })
        };

        instance.process();

        REQUIRE_FALSE(instance.cancel(id));
    }

    SECTION("skips sync task")
    {
        auto ran{ false };

        auto instance{ he::exec::scheduler{} };

        const auto id{
            instance.post(
                he::exec::task{
                    .mode = he::exec::task::type::sync,
                    .definition = [&ran](std::stop_token) { ran = true; },
                    .on_complete = [] {}
                })
        };

        instance.cancel(id);
        instance.process();

        REQUIRE_FALSE(ran);
    }

    SECTION("skips async on_complete")
    {
        auto work_done{ std::atomic<bool>{ false } };
        auto on_complete_ran{ false };

        auto instance{ he::exec::scheduler{} };

        const auto id{
            instance.post(
                he::exec::task{
                    .mode = he::exec::task::type::async,
                    .definition = [&work_done](std::stop_token) { work_done = true; },
                    .on_complete = [&on_complete_ran] { on_complete_ran = true; }
                })
        };

        while (!work_done) {}
        instance.cancel(id);

        instance.process();

        REQUIRE_FALSE(on_complete_ran);
    }

    SECTION("isolated per task")
    {
        auto first_ran{ false };
        auto second_ran{ false };

        auto instance{ he::exec::scheduler{} };

        const auto first_id{
            instance.post(
                he::exec::task{
                    .mode = he::exec::task::type::sync,
                    .definition = [&first_ran](std::stop_token) { first_ran = true; },
                    .on_complete = [] {}
                })
        };
        instance.post(
            he::exec::task{
                .mode = he::exec::task::type::sync,
                .definition = [&second_ran](std::stop_token) { second_ran = true; },
                .on_complete = [] {}
            });

        instance.cancel(first_id);
        instance.process();

        REQUIRE_FALSE(first_ran);
        REQUIRE(second_ran);
    }

    SECTION("observed mid-flight")
    {
        auto started{ std::atomic<bool>{ false } };
        auto observed_cancel{ std::atomic<bool>{ false } };
        auto on_complete_ran{ false };

        auto instance{ he::exec::scheduler{} };

        const auto id{
            instance.post(
                he::exec::task{
                    .mode = he::exec::task::type::async,
                    .definition =
                        [&started, &observed_cancel](std::stop_token token)
                    {
                        started = true;

                        while (!token.stop_requested()) {}

                        observed_cancel = true;
                    },
                    .on_complete = [&on_complete_ran] { on_complete_ran = true; }
                })
        };

        while (!started) {}
        instance.cancel(id);

        while (!observed_cancel) {}
        instance.process();

        REQUIRE(observed_cancel);
        REQUIRE_FALSE(on_complete_ran);
    }
}
