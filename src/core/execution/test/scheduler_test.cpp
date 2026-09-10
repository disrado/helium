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

        instance->tick();

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

        instance->tick();

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

        instance->tick();

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

        instance->tick();

        REQUIRE(order == "a");

        instance->tick();

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

        instance->tick();
        instance->tick();
        instance->tick();
        instance->tick();   // one extra call — must be a no-op, entry should already be gone

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
            instance->tick();
        }

        REQUIRE(complete_count == 10);

        REQUIRE(instance->cancel(id));
        REQUIRE(complete_count == 10);   // cancel() only flags it now; resolves on the next tick()

        instance->tick();

        REQUIRE(complete_count == 11);
    }
}


TEST_CASE("scheduler async task")
{
    SECTION("starts before the first tick() call")
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

    SECTION("on_complete after tick")
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
            instance->tick();
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
        instance->tick();

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
            instance->tick();
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
            instance->tick();
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
            instance->tick();
        }

        REQUIRE(completed_count == 3);
    }
}


TEST_CASE("scheduler async task timing")
{
    SECTION("delay defers the first dispatch")
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
                .delay{ std::chrono::milliseconds(200) }
            });

        instance->tick();
        instance->tick();

        REQUIRE_FALSE(work_done);

        while (!work_done)
        {
            instance->tick();
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
            instance->tick();
        }

        REQUIRE(dispatcher->dispatched);
    }
}


TEST_CASE("scheduler tick")
{
    SECTION("empty is a no-op")
    {
        auto instance{ he::exec::scheduler::create() };

        instance->tick();
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
            instance->tick();
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

        instance->tick();

        REQUIRE_FALSE(instance->cancel(id));
    }

    SECTION("cancel before first tick skips the definition, resolves cancelled")
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

        REQUIRE_FALSE(on_complete_status.has_value());   // cancel() only flags it; resolves on tick()

        instance->tick();

        REQUIRE_FALSE(ran);
        REQUIRE(on_complete_status == he::exec::task_result::cancelled);
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
            instance->tick();
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
            instance->tick();
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
        instance->tick();

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
            instance->tick();
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
            instance->tick();
        }

        // no sleep-and-hope needed: finalize_task_instance() delivers on_complete and, for a cancelled
        // result, erases the entry within that same synchronous tick() call — by the time the
        // loop above observes complete_count == 1, a would-be extra cycle is already impossible
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

        instance->tick();
        instance->tick();

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

        instance->tick();

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

            // deliberately no instance->tick() call — relying purely on ~scheduler()
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
                            return he::exec::tick_result::keep_going;
                        } },
                    .on_complete{ [&on_complete_status] (he::exec::task_result status) { on_complete_status = status; } }
                });

            instance->tick();

            REQUIRE(tick_count == 1);
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

        }

        REQUIRE(first_observed);
        REQUIRE_FALSE(second_definition_ran);
    }

    SECTION("task outliving a non-joining dispatcher's scheduler is dropped, not delivered")
    {
        class detaching_dispatcher final: public he::exec::dispatcher
        {
        public:
            explicit detaching_dispatcher(std::shared_ptr<std::atomic<bool>> delivery_attempted)
                : delivery_attempted{ std::move(delivery_attempted) }
            {
            }

            auto dispatch(std::function<void()> work) -> void override
            {
                std::thread{
                    [work{ std::move(work) }, delivery_attempted{ delivery_attempted }] () mutable
                    {
                        work();   // includes async_task's promise->set_value() delivery attempt
                        delivery_attempted->store(true);
                    } }.detach();
            }

        private:
            std::shared_ptr<std::atomic<bool>> delivery_attempted;
        };

        const auto started{ std::make_shared<std::atomic<bool>>(false) };
        const auto on_complete_ran{ std::make_shared<std::atomic<bool>>(false) };
        const auto delivery_attempted{ std::make_shared<std::atomic<bool>>(false) };

        {
            auto instance{ he::exec::scheduler::create() };
            instance->set_dispatcher(std::make_unique<detaching_dispatcher>(delivery_attempted));

            instance->post(
                he::exec::async_task_request{
                    .definition{
                        [started] (std::stop_token) -> he::exec::task_result
                        {
                            started->store(true);
                            std::this_thread::sleep_for(std::chrono::milliseconds(50));
                            return he::exec::task_result::succeeded;
                        } },
                    .on_complete{ [on_complete_ran] (he::exec::task_result) { on_complete_ran->store(true); } }
                });

            while (!started->load()) {}
        }

        // deterministic: waits for the wrapped work item (definition + promise->set_value attempt)
        // to actually finish, instead of sleeping a fixed duration and hoping it was long enough
        while (!delivery_attempted->load()) {}

        REQUIRE_FALSE(on_complete_ran->load());
    }
}
