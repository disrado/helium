#include "core/execution/scheduler.hpp"

#include <catch2/catch_test_macros.hpp>

#include <atomic>
#include <chrono>
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

        auto instance{ he::exec::scheduler::create() };

        instance->post(
            he::exec::task_request{
                .mode{ he::exec::launch_policy::sync },
                .definition{
                    [&definition_ran] (std::stop_token)
                    {
                        definition_ran = true;
                        return he::exec::execution_status::completed;
                    } },
                .on_complete{ [&on_complete_ran] (he::exec::execution_status) { on_complete_ran = true; } }
            });

        REQUIRE(definition_ran);
        REQUIRE(on_complete_ran);
    }

    SECTION("definition before on_complete")
    {
        auto order{ std::string{} };

        auto instance{ he::exec::scheduler::create() };

        instance->post(
            he::exec::task_request{
                .mode{ he::exec::launch_policy::sync },
                .definition{
                    [&order] (std::stop_token)
                    {
                        order += "a";
                        return he::exec::execution_status::completed;
                    } },
                .on_complete{ [&order] (he::exec::execution_status) { order += "b"; } }
            });

        REQUIRE(order == "ab");
    }

    SECTION("cannot be cancelled once posted")
    {
        auto instance{ he::exec::scheduler::create() };

        const auto id{
            instance->post(
                he::exec::task_request{
                    .mode{ he::exec::launch_policy::sync },
                    .definition{ [] (std::stop_token) { return he::exec::execution_status::completed; } },
                    .on_complete{ [] (he::exec::execution_status) {} }
                })
        };

        REQUIRE_FALSE(instance->cancel(id));
    }

    SECTION("faulted for unbound definition")
    {
        auto status{ std::optional<he::exec::execution_status>{} };

        auto instance{ he::exec::scheduler::create() };

        instance->post(
            he::exec::task_request{
                .mode{ he::exec::launch_policy::sync },
                .definition{},
                .on_complete{ [&status] (he::exec::execution_status s) { status = s; } }
            });

        REQUIRE(status == he::exec::execution_status::faulted);
    }
}


TEST_CASE("scheduler next_frame task")
{
    SECTION("waits")
    {
        auto ran{ false };

        auto instance{ he::exec::scheduler::create() };

        instance->post(
            he::exec::task_request{
                .mode{ he::exec::launch_policy::next_frame },
                .definition{
                    [&ran] (std::stop_token)
                    {
                        ran = true;
                        return he::exec::execution_status::completed;
                    } },
                .on_complete{ [] (he::exec::execution_status) {} }
            });

        REQUIRE_FALSE(ran);
    }

    SECTION("runs both")
    {
        auto definition_ran{ false };
        auto on_complete_ran{ false };

        auto instance{ he::exec::scheduler::create() };

        instance->post(
            he::exec::task_request{
                .mode{ he::exec::launch_policy::next_frame },
                .definition{
                    [&definition_ran] (std::stop_token)
                    {
                        definition_ran = true;
                        return he::exec::execution_status::completed;
                    } },
                .on_complete{ [&on_complete_ran] (he::exec::execution_status) { on_complete_ran = true; } }
            });

        instance->process();

        REQUIRE(definition_ran);
        REQUIRE(on_complete_ran);
    }

    SECTION("definition before on_complete")
    {
        auto order{ std::string{} };

        auto instance{ he::exec::scheduler::create() };

        instance->post(
            he::exec::task_request{
                .mode{ he::exec::launch_policy::next_frame },
                .definition{
                    [&order] (std::stop_token)
                    {
                        order += "a";
                        return he::exec::execution_status::completed;
                    } },
                .on_complete{ [&order] (he::exec::execution_status) { order += "b"; } }
            });

        instance->process();

        REQUIRE(order == "ab");
    }

    SECTION("submission order")
    {
        auto order{ std::string{} };

        auto instance{ he::exec::scheduler::create() };

        instance->post(
            he::exec::task_request{
                .mode{ he::exec::launch_policy::next_frame },
                .definition{
                    [&order] (std::stop_token)
                    {
                        order += "1";
                        return he::exec::execution_status::completed;
                    } },
                .on_complete{ [] (he::exec::execution_status) {} }
            });
        instance->post(
            he::exec::task_request{
                .mode{ he::exec::launch_policy::next_frame },
                .definition{
                    [&order] (std::stop_token)
                    {
                        order += "2";
                        return he::exec::execution_status::completed;
                    } },
                .on_complete{ [] (he::exec::execution_status) {} }
            });

        instance->process();

        REQUIRE(order == "12");
    }

    SECTION("re-queue resolves next call")
    {
        auto order{ std::string{} };

        auto instance{ he::exec::scheduler::create() };

        instance->post(
            he::exec::task_request{
                .mode{ he::exec::launch_policy::next_frame },
                .definition{
                    [&order, &instance] (std::stop_token)
                    {
                        order += "a";
                        instance->post(
                            he::exec::task_request{
                                .mode{ he::exec::launch_policy::next_frame },
                                .definition{
                                    [&order] (std::stop_token)
                                    {
                                        order += "b";
                                        return he::exec::execution_status::completed;
                                    } },
                                .on_complete{ [] (he::exec::execution_status) {} }
                            });
                        return he::exec::execution_status::completed;
                    } },
                .on_complete{ [] (he::exec::execution_status) {} }
            });

        instance->process();

        REQUIRE(order == "a");

        instance->process();

        REQUIRE(order == "ab");
    }
}


TEST_CASE("scheduler async task")
{
    SECTION("waits")
    {
        auto work_done{ std::atomic<bool>{ false } };
        auto on_complete_ran{ false };

        auto instance{ he::exec::scheduler::create() };

        instance->post(
            he::exec::task_request{
                .mode{ he::exec::launch_policy::async },
                .definition{
                    [&work_done] (std::stop_token)
                    {
                        work_done = true;
                        return he::exec::execution_status::completed;
                    } },
                .on_complete{ [&on_complete_ran] (he::exec::execution_status) { on_complete_ran = true; } }
            });

        while (!work_done) {}

        REQUIRE_FALSE(on_complete_ran);
    }

    SECTION("on_complete after process")
    {
        auto on_complete_ran{ false };

        auto instance{ he::exec::scheduler::create() };

        instance->post(
            he::exec::task_request{
                .mode{ he::exec::launch_policy::async },
                .definition{ [] (std::stop_token) { return he::exec::execution_status::completed; } },
                .on_complete{ [&on_complete_ran] (he::exec::execution_status) { on_complete_ran = true; } }
            });

        while (!on_complete_ran)
        {
            instance->process();
        }

        REQUIRE(on_complete_ran);
    }

    SECTION("worker thread")
    {
        auto work_done{ std::atomic<bool>{ false } };
        auto worker_thread_id{ std::thread::id{} };

        auto instance{ he::exec::scheduler::create() };

        instance->post(
            he::exec::task_request{
                .mode{ he::exec::launch_policy::async },
                .definition{
                    [&worker_thread_id, &work_done] (std::stop_token)
                    {
                        worker_thread_id = std::this_thread::get_id();
                        work_done = true;
                        return he::exec::execution_status::completed;
                    } },
                .on_complete{ [] (he::exec::execution_status) {} }
            });

        while (!work_done) {}
        instance->process();

        REQUIRE(worker_thread_id != std::this_thread::get_id());
    }

    SECTION("on_complete on calling thread")
    {
        auto on_complete_ran{ false };
        auto on_complete_thread_id{ std::thread::id{} };

        auto instance{ he::exec::scheduler::create() };

        instance->post(
            he::exec::task_request{
                .mode{ he::exec::launch_policy::async },
                .definition{ [] (std::stop_token) { return he::exec::execution_status::completed; } },
                .on_complete{
                    [&on_complete_ran, &on_complete_thread_id] (he::exec::execution_status)
                    {
                        on_complete_thread_id = std::this_thread::get_id();
                        on_complete_ran = true;
                    } }
            });

        while (!on_complete_ran)
        {
            instance->process();
        }

        REQUIRE(on_complete_thread_id == std::this_thread::get_id());
    }

    SECTION("reentrant post from on_complete does not deadlock")
    {
        auto second_ran{ false };

        auto instance{ he::exec::scheduler::create() };

        instance->post(
            he::exec::task_request{
                .mode{ he::exec::launch_policy::async },
                .definition{ [] (std::stop_token) { return he::exec::execution_status::completed; } },
                .on_complete{
                    [&instance, &second_ran] (he::exec::execution_status)
                    {
                        instance->post(
                            he::exec::task_request{
                                .mode{ he::exec::launch_policy::sync },
                                .definition{
                                    [&second_ran] (std::stop_token)
                                    {
                                        second_ran = true;
                                        return he::exec::execution_status::completed;
                                    } },
                                .on_complete{ [] (he::exec::execution_status) {} }
                            });
                    } }
            });

        while (!second_ran)
        {
            instance->process();
        }

        REQUIRE(second_ran);
    }

    SECTION("multiple complete")
    {
        auto completed_count{ 0 };

        auto instance{ he::exec::scheduler::create() };

        instance->post(
            he::exec::task_request{ .mode{ he::exec::launch_policy::async },
                                    .definition{ [] (std::stop_token) { return he::exec::execution_status::completed; } },
                                    .on_complete{ [&completed_count] (he::exec::execution_status) { completed_count++; } } });
        instance->post(
            he::exec::task_request{ .mode{ he::exec::launch_policy::async },
                                    .definition{ [] (std::stop_token) { return he::exec::execution_status::completed; } },
                                    .on_complete{ [&completed_count] (he::exec::execution_status) { completed_count++; } } });
        instance->post(
            he::exec::task_request{ .mode{ he::exec::launch_policy::async },
                                    .definition{ [] (std::stop_token) { return he::exec::execution_status::completed; } },
                                    .on_complete{ [&completed_count] (he::exec::execution_status) { completed_count++; } } });

        while (completed_count != 3)
        {
            instance->process();
        }

        REQUIRE(completed_count == 3);
    }
}


TEST_CASE("scheduler set_dispatcher")
{
    SECTION("used for async tasks")
    {
        auto instance{ he::exec::scheduler::create() };

        auto owned_dispatcher{ std::make_unique<recording_dispatcher>() };
        const auto* dispatcher{ owned_dispatcher.get() };

        instance->set_dispatcher(std::move(owned_dispatcher));

        auto on_complete_ran{ false };

        instance->post(
            he::exec::task_request{
                .mode{ he::exec::launch_policy::async },
                .definition{ [] (std::stop_token) { return he::exec::execution_status::completed; } },
                .on_complete{ [&on_complete_ran] (he::exec::execution_status) { on_complete_ran = true; } }
            });

        while (!on_complete_ran)
        {
            instance->process();
        }

        REQUIRE(dispatcher->dispatched);
    }
}


TEST_CASE("scheduler process")
{
    SECTION("empty is a no-op")
    {
        auto instance{ he::exec::scheduler::create() };

        instance->process();
    }

    SECTION("mixed resolve in one call")
    {
        auto next_frame_ran{ false };
        auto async_completed{ false };

        auto instance{ he::exec::scheduler::create() };

        instance->post(
            he::exec::task_request{
                .mode{ he::exec::launch_policy::async },
                .definition{ [] (std::stop_token) { return he::exec::execution_status::completed; } },
                .on_complete{ [&async_completed] (he::exec::execution_status) { async_completed = true; } }
            });
        instance->post(
            he::exec::task_request{
                .mode{ he::exec::launch_policy::next_frame },
                .definition{
                    [&next_frame_ran] (std::stop_token)
                    {
                        next_frame_ran = true;
                        return he::exec::execution_status::completed;
                    } },
                .on_complete{ [] (he::exec::execution_status) {} }
            });

        while (!async_completed)
        {
            instance->process();
        }

        REQUIRE(next_frame_ran);
        REQUIRE(async_completed);
    }
}


TEST_CASE("scheduler cancel")
{
    SECTION("false for unposted id")
    {
        auto instance{ he::exec::scheduler::create() };

        REQUIRE_FALSE(instance->cancel(he::exec::invalid_task_id));
        REQUIRE_FALSE(instance->cancel(he::exec::task_id{ 12345 }));
    }

    SECTION("true before it runs")
    {
        auto instance{ he::exec::scheduler::create() };

        const auto id{
            instance->post(
                he::exec::task_request{
                    .mode{ he::exec::launch_policy::next_frame },
                    .definition{ [] (std::stop_token) { return he::exec::execution_status::completed; } },
                    .on_complete{ [] (he::exec::execution_status) {} }
                })
        };

        REQUIRE(instance->cancel(id));
    }

    SECTION("false after processed")
    {
        auto instance{ he::exec::scheduler::create() };

        const auto id{
            instance->post(
                he::exec::task_request{
                    .mode{ he::exec::launch_policy::next_frame },
                    .definition{ [] (std::stop_token) { return he::exec::execution_status::completed; } },
                    .on_complete{ [] (he::exec::execution_status) {} }
                })
        };

        instance->process();

        REQUIRE_FALSE(instance->cancel(id));
    }

    SECTION("skips next_frame task")
    {
        auto ran{ false };

        auto instance{ he::exec::scheduler::create() };

        const auto id{
            instance->post(
                he::exec::task_request{
                    .mode{ he::exec::launch_policy::next_frame },
                    .definition{
                        [&ran] (std::stop_token)
                        {
                            ran = true;
                            return he::exec::execution_status::completed;
                        } },
                    .on_complete{ [] (he::exec::execution_status) {} }
                })
        };

        instance->cancel(id);
        instance->process();

        REQUIRE_FALSE(ran);
    }

    SECTION("cancel after real completion has no effect")
    {
        auto work_done{ std::atomic<bool>{ false } };
        auto on_complete_status{ std::optional<he::exec::execution_status>{} };

        auto instance{ he::exec::scheduler::create() };

        const auto id{
            instance->post(
                he::exec::task_request{
                    .mode{ he::exec::launch_policy::async },
                    .definition{
                        [&work_done] (std::stop_token)
                        {
                            work_done = true;
                            return he::exec::execution_status::completed;
                        } },
                    .on_complete{ [&on_complete_status] (he::exec::execution_status status) { on_complete_status = status; } }
                })
        };

        while (!work_done) {}
        instance->cancel(id);

        while (!on_complete_status.has_value())
        {
            instance->process();
        }

        // status is decided once, at the point the definition actually finishes — a cancel() that
        // races in afterward, before delivery, must not retroactively flip an already-real outcome
        REQUIRE(on_complete_status == he::exec::execution_status::completed);
    }

    SECTION("false once delivered")
    {
        auto on_complete_ran{ false };

        auto instance{ he::exec::scheduler::create() };

        const auto id{
            instance->post(
                he::exec::task_request{
                    .mode{ he::exec::launch_policy::async },
                    .definition{ [] (std::stop_token) { return he::exec::execution_status::completed; } },
                    .on_complete{ [&on_complete_ran] (he::exec::execution_status) { on_complete_ran = true; } }
                })
        };

        while (!on_complete_ran)
        {
            instance->process();
        }

        REQUIRE_FALSE(instance->cancel(id));
    }

    SECTION("isolated per task")
    {
        auto first_ran{ false };
        auto second_ran{ false };

        auto instance{ he::exec::scheduler::create() };

        const auto first_id{
            instance->post(
                he::exec::task_request{
                    .mode{ he::exec::launch_policy::next_frame },
                    .definition{
                        [&first_ran] (std::stop_token)
                        {
                            first_ran = true;
                            return he::exec::execution_status::completed;
                        } },
                    .on_complete{ [] (he::exec::execution_status) {} }
                })
        };
        instance->post(
            he::exec::task_request{
                .mode{ he::exec::launch_policy::next_frame },
                .definition{
                    [&second_ran] (std::stop_token)
                    {
                        second_ran = true;
                        return he::exec::execution_status::completed;
                    } },
                .on_complete{ [] (he::exec::execution_status) {} }
            });

        instance->cancel(first_id);
        instance->process();

        REQUIRE_FALSE(first_ran);
        REQUIRE(second_ran);
    }

    SECTION("observed mid-flight reports cancelled")
    {
        auto started{ std::atomic<bool>{ false } };
        auto observed_cancel{ std::atomic<bool>{ false } };
        auto on_complete_status{ std::optional<he::exec::execution_status>{} };

        auto instance{ he::exec::scheduler::create() };

        const auto id{
            instance->post(
                he::exec::task_request{
                    .mode{ he::exec::launch_policy::async },
                    .definition{
                        [&started, &observed_cancel] (std::stop_token token)
                        {
                            started = true;

                            while (!token.stop_requested()) {}

                            observed_cancel = true;

                            return he::exec::execution_status::cancelled;
                        } },
                    .on_complete{ [&on_complete_status] (he::exec::execution_status status) { on_complete_status = status; } }
                })
        };

        while (!started) {}
        instance->cancel(id);

        while (!observed_cancel) {}

        while (!on_complete_status.has_value())
        {
            instance->process();
        }

        REQUIRE(observed_cancel);
        REQUIRE(on_complete_status == he::exec::execution_status::cancelled);
    }
}


TEST_CASE("scheduler shutdown")
{
    SECTION("delivers completion on destruction")
    {
        auto on_complete_ran{ false };

        {
            auto instance{ he::exec::scheduler::create() };
            instance->set_dispatcher(std::make_unique<recording_dispatcher>());

            instance->post(
                he::exec::task_request{
                    .mode{ he::exec::launch_policy::async },
                    .definition{ [] (std::stop_token) { return he::exec::execution_status::completed; } },
                    .on_complete{ [&on_complete_ran] (he::exec::execution_status) { on_complete_ran = true; } }
                });

            // deliberately no instance->process() call — relying purely on ~scheduler() to run_completion it
        }

        REQUIRE(on_complete_ran);
    }

    SECTION("rejects post from on_complete during destruction")
    {
        auto observed{ false };
        auto id_during_shutdown{ he::exec::invalid_task_id };

        {
            auto instance{ he::exec::scheduler::create() };
            instance->set_dispatcher(std::make_unique<recording_dispatcher>());

            instance->post(
                he::exec::task_request{
                    .mode{ he::exec::launch_policy::async },
                    .definition{ [] (std::stop_token) { return he::exec::execution_status::completed; } },
                    .on_complete{
                        [&instance, &observed, &id_during_shutdown] (he::exec::execution_status)
                        {
                            observed = true;
                            id_during_shutdown = instance->post(
                                he::exec::task_request{
                                    .mode{ he::exec::launch_policy::sync },
                                    .definition{ [] (std::stop_token) { return he::exec::execution_status::completed; } },
                                    .on_complete{ [] (he::exec::execution_status) {} }
                                });
                        } }
                });

            // deliberately no instance->process() call — ~scheduler()'s drain() delivers on_complete,
            // which reenters post() while the object is already shutting down
        }

        REQUIRE(observed);
        REQUIRE(id_during_shutdown == he::exec::invalid_task_id);
    }

    SECTION("task outliving a non-joining dispatcher's scheduler is dropped, not delivered")
    {
        // mimics gd_dispatcher's shape: hands work to an independent thread with zero lifetime
        // coupling to the scheduler — no join, no wait, unlike thread_dispatcher's jthread pool
        class detaching_dispatcher final: public he::exec::dispatcher
        {
        public:
            auto dispatch(std::function<void()> work) -> void override
            {
                std::thread{ std::move(work) }.detach();
            }
        };

        const auto started{ std::make_shared<std::atomic<bool>>(false) };
        const auto finished{ std::make_shared<std::atomic<bool>>(false) };
        const auto on_complete_ran{ std::make_shared<std::atomic<bool>>(false) };

        {
            auto instance{ he::exec::scheduler::create() };
            instance->set_dispatcher(std::make_unique<detaching_dispatcher>());

            instance->post(
                he::exec::task_request{
                    .mode{ he::exec::launch_policy::async },
                    .definition{
                        [started, finished] (std::stop_token) -> he::exec::execution_status
                        {
                            started->store(true);
                            std::this_thread::sleep_for(std::chrono::milliseconds(50));
                            finished->store(true);
                            return he::exec::execution_status::completed;
                        } },
                    .on_complete{ [on_complete_ran] (he::exec::execution_status) { on_complete_ran->store(true); } }
                });

            // wait until the worker is actually inside the definition (past invoke_definition's
            // own pre-dispatch cancellation check) before destroying — otherwise ~scheduler()'s
            // request_stop() sweep can race ahead of thread startup and cancel it before it ever runs
            while (!started->load()) {}

            // instance destroyed here, mid-sleep — this task never checks its own stop_token
            // (non-cooperative), so it keeps running regardless, genuinely outliving the scheduler
        }

        while (!finished->load()) {}

        // give the worker a moment to reach its (now-skipped) delivery attempt
        std::this_thread::sleep_for(std::chrono::milliseconds(20));

        REQUIRE_FALSE(on_complete_ran->load());
    }
}
