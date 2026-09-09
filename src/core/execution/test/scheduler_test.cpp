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
            he::exec::sync_task_request{
                .definition{
                    [&definition_ran] (std::stop_token)
                    {
                        definition_ran = true;
                        return he::exec::task_result::succeeded;
                    } },
                .on_complete{ [&on_complete_ran] (he::exec::task_result) { on_complete_ran = true; } }
            });

        REQUIRE(definition_ran);
        REQUIRE(on_complete_ran);
    }

    SECTION("definition before on_complete")
    {
        auto order{ std::string{} };

        auto instance{ he::exec::scheduler::create() };

        instance->post(
            he::exec::sync_task_request{
                .definition{
                    [&order] (std::stop_token)
                    {
                        order += "a";
                        return he::exec::task_result::succeeded;
                    } },
                .on_complete{ [&order] (he::exec::task_result) { order += "b"; } }
            });

        REQUIRE(order == "ab");
    }

    SECTION("cannot be cancelled once posted")
    {
        auto instance{ he::exec::scheduler::create() };

        const auto id{
            instance->post(
                he::exec::sync_task_request{
                    .definition{ [] (std::stop_token) { return he::exec::task_result::succeeded; } },
                    .on_complete{ [] (he::exec::task_result) {} }
                })
        };

        REQUIRE_FALSE(instance->cancel(id));
    }

    SECTION("failed for unbound definition")
    {
        auto status{ std::optional<he::exec::task_result>{} };

        auto instance{ he::exec::scheduler::create() };

        instance->post(
            he::exec::sync_task_request{
                .definition{},
                .on_complete{ [&status] (he::exec::task_result s) { status = s; } }
            });

        REQUIRE(status == he::exec::task_result::failed);
    }
}


TEST_CASE("scheduler ticking task, single repetition")
{
    SECTION("waits")
    {
        auto ran{ false };

        auto instance{ he::exec::scheduler::create() };

        instance->post(
            he::exec::ticking_task_request{
                .definition{
                    [&ran] (std::stop_token) -> he::exec::tick_result
                    {
                        ran = true;
                        return he::exec::tick_result::succeeded;
                    } },
                .on_complete{ [] (he::exec::task_result) {} }
            });

        REQUIRE_FALSE(ran);
    }

    SECTION("runs both")
    {
        auto definition_ran{ false };
        auto on_complete_ran{ false };

        auto instance{ he::exec::scheduler::create() };

        instance->post(
            he::exec::ticking_task_request{
                .definition{
                    [&definition_ran] (std::stop_token) -> he::exec::tick_result
                    {
                        definition_ran = true;
                        return he::exec::tick_result::succeeded;
                    } },
                .on_complete{ [&on_complete_ran] (he::exec::task_result) { on_complete_ran = true; } }
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
            he::exec::ticking_task_request{
                .definition{
                    [&order] (std::stop_token) -> he::exec::tick_result
                    {
                        order += "a";
                        return he::exec::tick_result::succeeded;
                    } },
                .on_complete{ [&order] (he::exec::task_result) { order += "b"; } }
            });

        instance->process();

        REQUIRE(order == "ab");
    }

    SECTION("submission order")
    {
        auto order{ std::string{} };

        auto instance{ he::exec::scheduler::create() };

        instance->post(
            he::exec::ticking_task_request{
                .definition{
                    [&order] (std::stop_token) -> he::exec::tick_result
                    {
                        order += "1";
                        return he::exec::tick_result::succeeded;
                    } },
                .on_complete{ [] (he::exec::task_result) {} }
            });
        instance->post(
            he::exec::ticking_task_request{
                .definition{
                    [&order] (std::stop_token) -> he::exec::tick_result
                    {
                        order += "2";
                        return he::exec::tick_result::succeeded;
                    } },
                .on_complete{ [] (he::exec::task_result) {} }
            });

        instance->process();

        REQUIRE(order == "12");
    }

    SECTION("re-queue resolves next call")
    {
        auto order{ std::string{} };

        auto instance{ he::exec::scheduler::create() };

        instance->post(
            he::exec::ticking_task_request{
                .definition{
                    [&order, &instance] (std::stop_token) -> he::exec::tick_result
                    {
                        order += "a";
                        instance->post(
                            he::exec::ticking_task_request{
                                .definition{
                                    [&order] (std::stop_token) -> he::exec::tick_result
                                    {
                                        order += "b";
                                        return he::exec::tick_result::succeeded;
                                    } },
                                .on_complete{ [] (he::exec::task_result) {} }
                            });
                        return he::exec::tick_result::succeeded;
                    } },
                .on_complete{ [] (he::exec::task_result) {} }
            });

        instance->process();

        REQUIRE(order == "a");

        instance->process();

        REQUIRE(order == "ab");
    }
}


TEST_CASE("scheduler ticking task repetitions")
{
    SECTION("repetitions{N} fires on_complete exactly N times, then the id is gone")
    {
        auto tick_count{ 0 };
        auto complete_count{ 0 };

        auto instance{ he::exec::scheduler::create() };

        const auto id{
            instance->post(
                he::exec::ticking_task_request{
                    .definition{
                        [&tick_count] (std::stop_token) -> he::exec::tick_result
                        {
                            ++tick_count;
                            return he::exec::tick_result::succeeded;
                        } },
                    .on_complete{ [&complete_count] (he::exec::task_result) { ++complete_count; } },
                    .repetitions{ 3 }
                })
        };

        instance->process();
        instance->process();
        instance->process();
        instance->process();   // one extra call — must be a no-op, entry should already be gone

        REQUIRE(tick_count == 3);
        REQUIRE(complete_count == 3);
        REQUIRE_FALSE(instance->cancel(id));
    }

    SECTION("repetitions{nullopt} keeps firing until explicitly cancelled")
    {
        auto complete_count{ 0 };

        auto instance{ he::exec::scheduler::create() };

        const auto id{
            instance->post(
                he::exec::ticking_task_request{
                    .definition{ [] (std::stop_token) -> he::exec::tick_result { return he::exec::tick_result::succeeded; } },
                    .on_complete{ [&complete_count] (he::exec::task_result) { ++complete_count; } },
                    .repetitions{ std::nullopt }
                })
        };

        for (auto i{ 0 }; i < 10; ++i)
        {
            instance->process();
        }

        REQUIRE(complete_count == 10);

        REQUIRE(instance->cancel(id));

        // cancel() lands while idle (dormant, between cycles) — that delivers `cancelled` synchronously,
        // one more on_complete beyond the 10 successful cycles, per the documented idle-cancel contract
        REQUIRE(complete_count == 11);

        instance->process();

        REQUIRE(complete_count == 11);
    }
}


TEST_CASE("scheduler async task")
{
    SECTION("starts before the first process() call")
    {
        auto work_done{ std::atomic<bool>{ false } };

        auto instance{ he::exec::scheduler::create() };

        instance->post(
            he::exec::async_task_request{
                .definition{
                    [&work_done] (std::stop_token)
                    {
                        work_done = true;
                        return he::exec::task_result::succeeded;
                    } },
                .on_complete{ [] (he::exec::task_result) {} }
            });

        while (!work_done) {}

        REQUIRE(work_done);
    }

    SECTION("waits")
    {
        auto work_done{ std::atomic<bool>{ false } };
        auto on_complete_ran{ false };

        auto instance{ he::exec::scheduler::create() };

        instance->post(
            he::exec::async_task_request{
                .definition{
                    [&work_done] (std::stop_token)
                    {
                        work_done = true;
                        return he::exec::task_result::succeeded;
                    } },
                .on_complete{ [&on_complete_ran] (he::exec::task_result) { on_complete_ran = true; } }
            });

        while (!work_done) {}

        REQUIRE_FALSE(on_complete_ran);
    }

    SECTION("on_complete after process")
    {
        auto on_complete_ran{ false };

        auto instance{ he::exec::scheduler::create() };

        instance->post(
            he::exec::async_task_request{
                .definition{ [] (std::stop_token) { return he::exec::task_result::succeeded; } },
                .on_complete{ [&on_complete_ran] (he::exec::task_result) { on_complete_ran = true; } }
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
            he::exec::async_task_request{
                .definition{
                    [&worker_thread_id, &work_done] (std::stop_token)
                    {
                        worker_thread_id = std::this_thread::get_id();
                        work_done = true;
                        return he::exec::task_result::succeeded;
                    } },
                .on_complete{ [] (he::exec::task_result) {} }
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
            he::exec::async_task_request{
                .definition{ [] (std::stop_token) { return he::exec::task_result::succeeded; } },
                .on_complete{
                    [&on_complete_ran, &on_complete_thread_id] (he::exec::task_result)
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
            he::exec::async_task_request{
                .definition{ [] (std::stop_token) { return he::exec::task_result::succeeded; } },
                .on_complete{
                    [&instance, &second_ran] (he::exec::task_result)
                    {
                        instance->post(
                            he::exec::sync_task_request{
                                .definition{
                                    [&second_ran] (std::stop_token)
                                    {
                                        second_ran = true;
                                        return he::exec::task_result::succeeded;
                                    } },
                                .on_complete{ [] (he::exec::task_result) {} }
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
            he::exec::async_task_request{ .definition{ [] (std::stop_token) { return he::exec::task_result::succeeded; } },
                                          .on_complete{ [&completed_count] (he::exec::task_result) { completed_count++; } } });
        instance->post(
            he::exec::async_task_request{ .definition{ [] (std::stop_token) { return he::exec::task_result::succeeded; } },
                                          .on_complete{ [&completed_count] (he::exec::task_result) { completed_count++; } } });
        instance->post(
            he::exec::async_task_request{ .definition{ [] (std::stop_token) { return he::exec::task_result::succeeded; } },
                                          .on_complete{ [&completed_count] (he::exec::task_result) { completed_count++; } } });

        while (completed_count != 3)
        {
            instance->process();
        }

        REQUIRE(completed_count == 3);
    }
}


TEST_CASE("scheduler async task timing")
{
    SECTION("initial_delay defers the first dispatch")
    {
        auto work_done{ std::atomic<bool>{ false } };

        auto instance{ he::exec::scheduler::create() };

        instance->post(
            he::exec::async_task_request{
                .definition{
                    [&work_done] (std::stop_token)
                    {
                        work_done = true;
                        return he::exec::task_result::succeeded;
                    } },
                .on_complete{ [] (he::exec::task_result) {} },
                .initial_delay{ std::chrono::milliseconds(200) }
            });

        instance->process();
        instance->process();

        REQUIRE_FALSE(work_done);

        while (!work_done)
        {
            instance->process();
        }
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
            he::exec::async_task_request{
                .definition{ [] (std::stop_token) { return he::exec::task_result::succeeded; } },
                .on_complete{ [&on_complete_ran] (he::exec::task_result) { on_complete_ran = true; } }
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
        auto ticking_ran{ false };
        auto async_completed{ false };

        auto instance{ he::exec::scheduler::create() };

        instance->post(
            he::exec::async_task_request{
                .definition{ [] (std::stop_token) { return he::exec::task_result::succeeded; } },
                .on_complete{ [&async_completed] (he::exec::task_result) { async_completed = true; } }
            });
        instance->post(
            he::exec::ticking_task_request{
                .definition{
                    [&ticking_ran] (std::stop_token) -> he::exec::tick_result
                    {
                        ticking_ran = true;
                        return he::exec::tick_result::succeeded;
                    } },
                .on_complete{ [] (he::exec::task_result) {} }
            });

        while (!async_completed)
        {
            instance->process();
        }

        REQUIRE(ticking_ran);
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
                he::exec::ticking_task_request{
                    .definition{ [] (std::stop_token) { return he::exec::tick_result::succeeded; } },
                    .on_complete{ [] (he::exec::task_result) {} }
                })
        };

        REQUIRE(instance->cancel(id));
    }

    SECTION("false after processed")
    {
        auto instance{ he::exec::scheduler::create() };

        const auto id{
            instance->post(
                he::exec::ticking_task_request{
                    .definition{ [] (std::stop_token) { return he::exec::tick_result::succeeded; } },
                    .on_complete{ [] (he::exec::task_result) {} }
                })
        };

        instance->process();

        REQUIRE_FALSE(instance->cancel(id));
    }

    SECTION("skips ticking task, delivered synchronously")
    {
        auto ran{ false };
        auto on_complete_status{ std::optional<he::exec::task_result>{} };

        auto instance{ he::exec::scheduler::create() };

        const auto id{
            instance->post(
                he::exec::ticking_task_request{
                    .definition{
                        [&ran] (std::stop_token) -> he::exec::tick_result
                        {
                            ran = true;
                            return he::exec::tick_result::succeeded;
                        } },
                    .on_complete{ [&on_complete_status] (he::exec::task_result status) { on_complete_status = status; } }
                })
        };

        instance->cancel(id);

        // idle (never dispatched) — request_cancel() delivers cancelled synchronously
        REQUIRE(on_complete_status == he::exec::task_result::cancelled);

        instance->process();

        REQUIRE_FALSE(ran);
    }

    SECTION("cancel after real completion has no effect")
    {
        auto work_done{ std::atomic<bool>{ false } };
        auto on_complete_status{ std::optional<he::exec::task_result>{} };

        auto instance{ he::exec::scheduler::create() };

        const auto id{
            instance->post(
                he::exec::async_task_request{
                    .definition{
                        [&work_done] (std::stop_token)
                        {
                            work_done = true;
                            return he::exec::task_result::succeeded;
                        } },
                    .on_complete{ [&on_complete_status] (he::exec::task_result status) { on_complete_status = status; } }
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
        REQUIRE(on_complete_status == he::exec::task_result::succeeded);
    }

    SECTION("false once delivered")
    {
        auto on_complete_ran{ false };

        auto instance{ he::exec::scheduler::create() };

        const auto id{
            instance->post(
                he::exec::async_task_request{
                    .definition{ [] (std::stop_token) { return he::exec::task_result::succeeded; } },
                    .on_complete{ [&on_complete_ran] (he::exec::task_result) { on_complete_ran = true; } }
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
                he::exec::ticking_task_request{
                    .definition{
                        [&first_ran] (std::stop_token) -> he::exec::tick_result
                        {
                            first_ran = true;
                            return he::exec::tick_result::succeeded;
                        } },
                    .on_complete{ [] (he::exec::task_result) {} }
                })
        };
        instance->post(
            he::exec::ticking_task_request{
                .definition{
                    [&second_ran] (std::stop_token) -> he::exec::tick_result
                    {
                        second_ran = true;
                        return he::exec::tick_result::succeeded;
                    } },
                .on_complete{ [] (he::exec::task_result) {} }
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
        auto on_complete_status{ std::optional<he::exec::task_result>{} };

        auto instance{ he::exec::scheduler::create() };

        const auto id{
            instance->post(
                he::exec::async_task_request{
                    .definition{
                        [&started, &observed_cancel] (std::stop_token token)
                        {
                            started = true;

                            while (!token.stop_requested()) {}

                            observed_cancel = true;

                            return he::exec::task_result::cancelled;
                        } },
                    .on_complete{ [&on_complete_status] (he::exec::task_result status) { on_complete_status = status; } }
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
        REQUIRE(on_complete_status == he::exec::task_result::cancelled);
    }

    SECTION("cancelling an in-flight repeating async task stops it, exactly one delivery")
    {
        auto started{ std::atomic<bool>{ false } };
        auto complete_count{ std::atomic<int>{ 0 } };

        auto instance{ he::exec::scheduler::create() };

        const auto id{
            instance->post(
                he::exec::async_task_request{
                    .definition{
                        [&started] (std::stop_token token)
                        {
                            started = true;

                            while (!token.stop_requested()) {}

                            return he::exec::task_result::cancelled;
                        } },
                    .on_complete{ [&complete_count] (he::exec::task_result) { ++complete_count; } },
                    .repetitions{ std::nullopt }
                })
        };

        while (!started) {}
        instance->cancel(id);

        while (complete_count.load() == 0)
        {
            instance->process();
        }

        // give a would-be extra cycle a chance to (wrongly) start
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        instance->process();

        REQUIRE(complete_count.load() == 1);
        REQUIRE_FALSE(instance->cancel(id));
    }

    SECTION("self-cancellation from on_complete delivers exactly once")
    {
        auto complete_count{ 0 };

        auto instance{ he::exec::scheduler::create() };
        auto id{ he::exec::invalid_task_id };

        id = instance->post(
            he::exec::ticking_task_request{
                .definition{ [] (std::stop_token) -> he::exec::tick_result { return he::exec::tick_result::succeeded; } },
                .on_complete{
                    [&instance, &id, &complete_count] (he::exec::task_result)
                    {
                        ++complete_count;
                        instance->cancel(id);
                    } },
                .repetitions{ 5 }
            });

        instance->process();
        instance->process();

        REQUIRE(complete_count == 1);
        REQUIRE_FALSE(instance->cancel(id));
    }

    SECTION("self-cancellation from inside the ticking definition delivers exactly once")
    {
        auto complete_count{ 0 };

        auto instance{ he::exec::scheduler::create() };
        auto id{ he::exec::invalid_task_id };

        id = instance->post(
            he::exec::ticking_task_request{
                .definition{
                    [&instance, &id] (std::stop_token token) -> he::exec::tick_result
                    {
                        instance->cancel(id);

                        // cooperative: the definition notices the self-requested stop and reports it —
                        // a cancel() alone (without a cancelled *result*) doesn't force termination,
                        // it only sets the flag; see the in-flight-repeat-cancel fix this exercises
                        return token.stop_requested() ? he::exec::tick_result::cancelled : he::exec::tick_result::succeeded;
                    } },
                .on_complete{ [&complete_count] (he::exec::task_result) { ++complete_count; } },
                .repetitions{ 5 }
            });

        instance->process();

        REQUIRE(complete_count == 1);
        REQUIRE_FALSE(instance->cancel(id));
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
                he::exec::async_task_request{
                    .definition{ [] (std::stop_token) { return he::exec::task_result::succeeded; } },
                    .on_complete{ [&on_complete_ran] (he::exec::task_result) { on_complete_ran = true; } }
                });

            // deliberately no instance->process() call — relying purely on ~scheduler()
        }

        REQUIRE(on_complete_ran);
    }

    SECTION("notifies a mid-cycle ticking task too, not just async")
    {
        auto tick_count{ 0 };
        auto on_complete_status{ std::optional<he::exec::task_result>{} };

        {
            auto instance{ he::exec::scheduler::create() };

            instance->post(
                he::exec::ticking_task_request{
                    .definition{
                        [&tick_count] (std::stop_token) -> he::exec::tick_result
                        {
                            ++tick_count;
                            return he::exec::tick_result::running;
                        } },
                    .on_complete{ [&on_complete_status] (he::exec::task_result status) { on_complete_status = status; } }
                });

            instance->process();

            REQUIRE(tick_count == 1);

            // instance destroyed here, mid-cycle (phase::running, already returned `running` once) —
            // this is the second genuinely-dropped case alongside in-flight async: nothing calls
            // tick() on it again, so on_complete is never delivered
        }

        REQUIRE_FALSE(on_complete_status.has_value());
    }

    SECTION("rejects post from on_complete during destruction")
    {
        auto observed{ false };
        auto id_during_shutdown{ he::exec::invalid_task_id };

        {
            auto instance{ he::exec::scheduler::create() };
            instance->set_dispatcher(std::make_unique<recording_dispatcher>());

            instance->post(
                he::exec::async_task_request{
                    .definition{ [] (std::stop_token) { return he::exec::task_result::succeeded; } },
                    .on_complete{
                        [&instance, &observed, &id_during_shutdown] (he::exec::task_result)
                        {
                            observed = true;
                            id_during_shutdown = instance->post(
                                he::exec::sync_task_request{
                                    .definition{ [] (std::stop_token) { return he::exec::task_result::succeeded; } },
                                    .on_complete{ [] (he::exec::task_result) {} }
                                });
                        } }
                });

            // deliberately no instance->process() call — ~scheduler()'s notification loop delivers
            // on_complete, which reenters post() while the object is already shutting down
        }

        REQUIRE(observed);
        REQUIRE(id_during_shutdown == he::exec::invalid_task_id);
    }

    SECTION("rejects a zero-delay async post during shutdown before it can dispatch")
    {
        auto first_observed{ false };
        auto second_definition_ran{ false };

        {
            auto instance{ he::exec::scheduler::create() };
            instance->set_dispatcher(std::make_unique<recording_dispatcher>());

            instance->post(
                he::exec::async_task_request{
                    .definition{ [] (std::stop_token) { return he::exec::task_result::succeeded; } },
                    .on_complete{
                        [&instance, &first_observed, &second_definition_ran] (he::exec::task_result)
                        {
                            first_observed = true;
                            instance->post(
                                he::exec::async_task_request{
                                    .definition{
                                        [&second_definition_ran] (std::stop_token)
                                        {
                                            second_definition_ran = true;
                                            return he::exec::task_result::succeeded;
                                        } },
                                    .on_complete{ [] (he::exec::task_result) {} }
                                });
                        } }
                });

            // no instance->process() call — reentrant post() above happens during ~scheduler()'s
            // notification loop; the shutdown check must run before the eager dispatch, so the
            // second task's definition must never actually execute
        }

        REQUIRE(first_observed);
        REQUIRE_FALSE(second_definition_ran);
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
                he::exec::async_task_request{
                    .definition{
                        [started, finished] (std::stop_token) -> he::exec::task_result
                        {
                            started->store(true);
                            std::this_thread::sleep_for(std::chrono::milliseconds(50));
                            finished->store(true);
                            return he::exec::task_result::succeeded;
                        } },
                    .on_complete{ [on_complete_ran] (he::exec::task_result) { on_complete_ran->store(true); } }
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
