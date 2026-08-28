#include "core/execution/scheduler.hpp"

#include <catch2/catch_test_macros.hpp>

#include <atomic>
#include <optional>
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
    SECTION("runs immediately")
    {
        auto definition_ran{ false };
        auto on_complete_ran{ false };

        auto instance{ he::exec::scheduler{} };

        instance.post(
            he::exec::task_request{
                .mode = he::exec::launch_policy::sync,
                .definition = [&definition_ran](std::stop_token) { definition_ran = true; },
                .on_complete = [&on_complete_ran](he::exec::execution_status) { on_complete_ran = true; }
            });

        REQUIRE(definition_ran);
        REQUIRE(on_complete_ran);
    }

    SECTION("definition before on_complete")
    {
        auto order{ std::string{} };

        auto instance{ he::exec::scheduler{} };

        instance.post(
            he::exec::task_request{
                .mode = he::exec::launch_policy::sync,
                .definition = [&order](std::stop_token) { order += "a"; },
                .on_complete = [&order](he::exec::execution_status) { order += "b"; }
            });

        REQUIRE(order == "ab");
    }

    SECTION("cannot be cancelled once posted")
    {
        auto instance{ he::exec::scheduler{} };

        const auto id{
            instance.post(
                he::exec::task_request{
                    .mode = he::exec::launch_policy::sync,
                    .definition = [](std::stop_token) {},
                    .on_complete = [](he::exec::execution_status) {}
                })
        };

        REQUIRE_FALSE(instance.cancel(id));
    }
}


TEST_CASE("scheduler next_frame task")
{
    SECTION("waits")
    {
        auto ran{ false };

        auto instance{ he::exec::scheduler{} };

        instance.post(
            he::exec::task_request{
                .mode = he::exec::launch_policy::next_frame,
                .definition = [&ran](std::stop_token) { ran = true; },
                .on_complete = [](he::exec::execution_status) {}
            });

        REQUIRE_FALSE(ran);
    }

    SECTION("runs both")
    {
        auto definition_ran{ false };
        auto on_complete_ran{ false };

        auto instance{ he::exec::scheduler{} };

        instance.post(
            he::exec::task_request{
                .mode = he::exec::launch_policy::next_frame,
                .definition = [&definition_ran](std::stop_token) { definition_ran = true; },
                .on_complete = [&on_complete_ran](he::exec::execution_status) { on_complete_ran = true; }
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
            he::exec::task_request{
                .mode = he::exec::launch_policy::next_frame,
                .definition = [&order](std::stop_token) { order += "a"; },
                .on_complete = [&order](he::exec::execution_status) { order += "b"; }
            });

        instance.process();

        REQUIRE(order == "ab");
    }

    SECTION("submission order")
    {
        auto order{ std::string{} };

        auto instance{ he::exec::scheduler{} };

        instance.post(
            he::exec::task_request{
                .mode = he::exec::launch_policy::next_frame,
                .definition = [&order](std::stop_token) { order += "1"; },
                .on_complete = [](he::exec::execution_status) {}
            });
        instance.post(
            he::exec::task_request{
                .mode = he::exec::launch_policy::next_frame,
                .definition = [&order](std::stop_token) { order += "2"; },
                .on_complete = [](he::exec::execution_status) {}
            });

        instance.process();

        REQUIRE(order == "12");
    }

    SECTION("re-queue resolves same call")
    {
        auto order{ std::string{} };

        auto instance{ he::exec::scheduler{} };

        instance.post(
            he::exec::task_request{
                .mode = he::exec::launch_policy::next_frame,
                .definition =
                    [&order, &instance](std::stop_token)
                {
                    order += "a";
                    instance.post(
                        he::exec::task_request{
                            .mode = he::exec::launch_policy::next_frame,
                            .definition = [&order](std::stop_token) { order += "b"; },
                            .on_complete = [](he::exec::execution_status) {}
                        });
                },
                .on_complete = [](he::exec::execution_status) {}
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
            he::exec::task_request{
                .mode = he::exec::launch_policy::async,
                .definition = [&work_done](std::stop_token) { work_done = true; },
                .on_complete = [&on_complete_ran](he::exec::execution_status) { on_complete_ran = true; }
            });

        while (!work_done) {}

        REQUIRE_FALSE(on_complete_ran);
    }

    SECTION("on_complete after process")
    {
        auto on_complete_ran{ false };

        auto instance{ he::exec::scheduler{} };

        instance.post(
            he::exec::task_request{
                .mode = he::exec::launch_policy::async,
                .definition = [](std::stop_token) {},
                .on_complete = [&on_complete_ran](he::exec::execution_status) { on_complete_ran = true; }
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
            he::exec::task_request{
                .mode = he::exec::launch_policy::async,
                .definition =
                    [&worker_thread_id, &work_done](std::stop_token)
                {
                    worker_thread_id = std::this_thread::get_id();
                    work_done = true;
                },
                .on_complete = [](he::exec::execution_status) {}
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
            he::exec::task_request{
                .mode = he::exec::launch_policy::async,
                .definition = [](std::stop_token) {},
                .on_complete =
                    [&on_complete_ran, &on_complete_thread_id](he::exec::execution_status)
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

        instance.post(he::exec::task_request{ .mode = he::exec::launch_policy::async, .definition = [](std::stop_token) {}, .on_complete = [&completed_count](he::exec::execution_status) { completed_count++; } });
        instance.post(he::exec::task_request{ .mode = he::exec::launch_policy::async, .definition = [](std::stop_token) {}, .on_complete = [&completed_count](he::exec::execution_status) { completed_count++; } });
        instance.post(he::exec::task_request{ .mode = he::exec::launch_policy::async, .definition = [](std::stop_token) {}, .on_complete = [&completed_count](he::exec::execution_status) { completed_count++; } });

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
            he::exec::task_request{
                .mode = he::exec::launch_policy::async,
                .definition = [](std::stop_token) {},
                .on_complete = [&on_complete_ran](he::exec::execution_status) { on_complete_ran = true; }
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
        auto next_frame_ran{ false };
        auto async_completed{ false };

        auto instance{ he::exec::scheduler{} };

        instance.post(
            he::exec::task_request{
                .mode = he::exec::launch_policy::async,
                .definition = [](std::stop_token) {},
                .on_complete = [&async_completed](he::exec::execution_status) { async_completed = true; }
            });
        instance.post(
            he::exec::task_request{
                .mode = he::exec::launch_policy::next_frame,
                .definition = [&next_frame_ran](std::stop_token) { next_frame_ran = true; },
                .on_complete = [](he::exec::execution_status) {}
            });

        while (!async_completed)
        {
            instance.process();
        }

        REQUIRE(next_frame_ran);
        REQUIRE(async_completed);
    }
}


TEST_CASE("scheduler cancel")
{
    SECTION("false for unposted id")
    {
        auto instance{ he::exec::scheduler{} };

        REQUIRE_FALSE(instance.cancel(he::exec::scheduler::invalid_task_id));
        REQUIRE_FALSE(instance.cancel(he::exec::task_id{ 12345 }));
    }

    SECTION("true before it runs")
    {
        auto instance{ he::exec::scheduler{} };

        const auto id{
            instance.post(
                he::exec::task_request{
                    .mode = he::exec::launch_policy::next_frame,
                    .definition = [](std::stop_token) {},
                    .on_complete = [](he::exec::execution_status) {}
                })
        };

        REQUIRE(instance.cancel(id));
    }

    SECTION("false after processed")
    {
        auto instance{ he::exec::scheduler{} };

        const auto id{
            instance.post(
                he::exec::task_request{
                    .mode = he::exec::launch_policy::next_frame,
                    .definition = [](std::stop_token) {},
                    .on_complete = [](he::exec::execution_status) {}
                })
        };

        instance.process();

        REQUIRE_FALSE(instance.cancel(id));
    }

    SECTION("skips next_frame task")
    {
        auto ran{ false };

        auto instance{ he::exec::scheduler{} };

        const auto id{
            instance.post(
                he::exec::task_request{
                    .mode = he::exec::launch_policy::next_frame,
                    .definition = [&ran](std::stop_token) { ran = true; },
                    .on_complete = [](he::exec::execution_status) {}
                })
        };

        instance.cancel(id);
        instance.process();

        REQUIRE_FALSE(ran);
    }

    SECTION("async on_complete reports cancelled")
    {
        auto work_done{ std::atomic<bool>{ false } };
        auto on_complete_status{ std::optional<he::exec::execution_status>{} };

        auto instance{ he::exec::scheduler{} };

        const auto id{
            instance.post(
                he::exec::task_request{
                    .mode = he::exec::launch_policy::async,
                    .definition = [&work_done](std::stop_token) { work_done = true; },
                    .on_complete = [&on_complete_status](he::exec::execution_status status) { on_complete_status = status; }
                })
        };

        while (!work_done) {}
        instance.cancel(id);

        while (!on_complete_status.has_value())
        {
            instance.process();
        }

        REQUIRE(on_complete_status == he::exec::execution_status::cancelled);
    }

    SECTION("isolated per task")
    {
        auto first_ran{ false };
        auto second_ran{ false };

        auto instance{ he::exec::scheduler{} };

        const auto first_id{
            instance.post(
                he::exec::task_request{
                    .mode = he::exec::launch_policy::next_frame,
                    .definition = [&first_ran](std::stop_token) { first_ran = true; },
                    .on_complete = [](he::exec::execution_status) {}
                })
        };
        instance.post(
            he::exec::task_request{
                .mode = he::exec::launch_policy::next_frame,
                .definition = [&second_ran](std::stop_token) { second_ran = true; },
                .on_complete = [](he::exec::execution_status) {}
            });

        instance.cancel(first_id);
        instance.process();

        REQUIRE_FALSE(first_ran);
        REQUIRE(second_ran);
    }

    SECTION("observed mid-flight reports cancelled")
    {
        auto started{ std::atomic<bool>{ false } };
        auto observed_cancel{ std::atomic<bool>{ false } };
        auto on_complete_status{ std::optional<he::exec::execution_status>{} };

        auto instance{ he::exec::scheduler{} };

        const auto id{
            instance.post(
                he::exec::task_request{
                    .mode = he::exec::launch_policy::async,
                    .definition =
                        [&started, &observed_cancel](std::stop_token token)
                    {
                        started = true;

                        while (!token.stop_requested()) {}

                        observed_cancel = true;
                    },
                    .on_complete = [&on_complete_status](he::exec::execution_status status) { on_complete_status = status; }
                })
        };

        while (!started) {}
        instance.cancel(id);

        while (!observed_cancel) {}

        while (!on_complete_status.has_value())
        {
            instance.process();
        }

        REQUIRE(observed_cancel);
        REQUIRE(on_complete_status == he::exec::execution_status::cancelled);
    }
}
