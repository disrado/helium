#include "core/execution/action/action.hpp"
#include "core/execution/action/async_action.hpp"
#include "core/execution/action/sequential_composite.hpp"
#include "core/execution/run.hpp"
#include "core/execution/scheduler.hpp"

#include <catch2/catch_test_macros.hpp>

#include <atomic>
#include <chrono>
#include <optional>
#include <stop_token>
#include <string>
#include <thread>


TEST_CASE("sequential_composite")
{
    SECTION("runs steps in order")
    {
        auto order{ std::string{} };

        auto token{
            he::run(
                he::sequential_composite{
                    he::action{ [&order] (const he::action::context&)
                    {
                        order += "a";
                        return true;
                    } },
                    he::action{ [&order] (const he::action::context&)
                    {
                        order += "b";
                        return true;
                    } },
                    he::action{ [&order] (const he::action::context&)
                    {
                        order += "c";
                        return true;
                    } }
                })
        };

        REQUIRE(order == "abc");
    }

    SECTION("stops after a mid-chain step fails")
    {
        auto third_ran{ false };

        auto token{
            he::run(
                he::sequential_composite{
                    he::action{ [] (const he::action::context&) { return true; } },
                    he::action{ [] (const he::action::context&) { return false; } },
                    he::action{ [&third_ran] (const he::action::context&)
                    {
                        third_ran = true;
                        return true;
                    } }
                })
        };

        REQUIRE_FALSE(third_ran);
    }

    SECTION("fails on mid-chain failure")
    {
        auto failed{ false };

        auto token{
            he::run(
                he::sequential_composite{
                    he::action{ [] (const he::action::context&) { return true; } },
                    he::action{ [] (const he::action::context&) { return false; } },
                    he::action{ [] (const he::action::context&) { return true; } }
                }
                .or_else(he::action{ [&failed] (const he::action::context&)
                {
                    failed = true;
                    return true;
                } }))
        };

        REQUIRE(failed);
    }

    SECTION("context propagates to next sibling")
    {
        auto received{ std::optional<std::string>{} };

        auto initial_context{ he::action::context{ { "label", std::string{ "run" } } } };

        auto token{
            he::run(
                he::sequential_composite{
                    he::action{ [] (const he::action::context&) { return true; } },
                    he::action{ [&received] (const he::action::context& ctx)
                    {
                        if (ctx.contains("label"))
                        {
                            received = std::any_cast<std::string>(ctx.at("label"));
                        }

                        return true;
                    } }
                },
                std::move(initial_context))
        };

        REQUIRE(received.has_value());
        REQUIRE(received.value() == "run");
    }

    SECTION("context propagates into nested composite")
    {
        auto received{ std::optional<std::string>{} };

        auto initial_context{ he::action::context{ { "label", std::string{ "run" } } } };

        auto token{
            he::run(
                he::sequential_composite{
                    he::action{ [] (const he::action::context&) { return true; } },
                    he::sequential_composite{
                        he::action{ [&received] (const he::action::context& ctx)
                        {
                            if (ctx.contains("label"))
                            {
                                received = std::any_cast<std::string>(ctx.at("label"));
                            }

                            return true;
                        } }
                    }
                },
                std::move(initial_context))
        };

        REQUIRE(received.has_value());
        REQUIRE(received.value() == "run");
    }

    SECTION("succeeds when last step succeeds")
    {
        auto succeeded{ false };

        auto token{
            he::run(
                he::sequential_composite{
                    he::action{ [] (const he::action::context&) { return true; } },
                    he::action{ [] (const he::action::context&) { return true; } }
                }
                .and_then(he::action{ [&succeeded] (const he::action::context&)
                {
                    succeeded = true;
                    return true;
                } }))
        };

        REQUIRE(succeeded);
    }

    SECTION("fails when last step fails")
    {
        auto failed{ false };

        auto token{
            he::run(
                he::sequential_composite{
                    he::action{ [] (const he::action::context&) { return true; } },
                    he::action{ [] (const he::action::context&) { return false; } }
                }
                .or_else(he::action{ [&failed] (const he::action::context&)
                {
                    failed = true;
                    return true;
                } }))
        };

        REQUIRE(failed);
    }
}


TEST_CASE("sequential_composite chaining")
{
    SECTION("and_then runs when composite succeeds")
    {
        auto then_ran{ false };

        auto token{
            he::run(
                he::sequential_composite{
                    he::action{ [] (const he::action::context&) { return true; } }
                }
                .and_then(
                    he::action{ [&then_ran] (const he::action::context&)
                    {
                        then_ran = true;
                        return true;
                    } }))
        };

        REQUIRE(then_ran);
    }

    SECTION("or_else runs when composite fails")
    {
        auto otherwise_ran{ false };

        auto token{
            he::run(
                he::sequential_composite{
                    he::action{ [] (const he::action::context&) { return false; } }
                }
                .or_else(
                    he::action{ [&otherwise_ran] (const he::action::context&)
                    {
                        otherwise_ran = true;
                        return true;
                    } }))
        };

        REQUIRE(otherwise_ran);
    }

    SECTION("or_else runs on mid-chain failure")
    {
        auto otherwise_ran{ false };

        auto token{
            he::run(
                he::sequential_composite{
                    he::action{ [] (const he::action::context&) { return true; } },
                    he::action{ [] (const he::action::context&) { return false; } },
                    he::action{ [] (const he::action::context&) { return true; } }
                }
                .or_else(
                    he::action{ [&otherwise_ran] (const he::action::context&)
                    {
                        otherwise_ran = true;
                        return true;
                    } }))
        };

        REQUIRE(otherwise_ran);
    }

    SECTION("context propagates to and_then branch")
    {
        auto received{ std::optional<std::string>{} };

        auto initial_context{ he::action::context{ { "label", std::string{ "run" } } } };

        auto token{
            he::run(
                he::sequential_composite{
                    he::action{ [] (const he::action::context&) { return true; } }
                }
                .and_then(
                    he::action{ [&received] (const he::action::context& ctx)
                    {
                        if (ctx.contains("label"))
                        {
                            received = std::any_cast<std::string>(ctx.at("label"));
                        }

                        return true;
                    } }),
                std::move(initial_context))
        };

        REQUIRE(received.has_value());
        REQUIRE(received.value() == "run");
    }
}


TEST_CASE("sequential_composite with async step")
{
    SECTION("continues past an async step")
    {
        auto order{ std::string{} };
        auto done{ std::atomic<bool>{ false } };

        auto token{
            he::run(
                he::sequential_composite{
                    he::async_action{ [&order] (const he::async_action::context&)
                    {
                        order += "a";
                        return true;
                    } },
                    he::action{ [&order, &done] (const he::action::context&)
                    {
                        order += "b";
                        done = true;
                        return true;
                    } }
                })
        };

        while (!done)
        {
            he::exec::scheduler::instance().process();
        }

        REQUIRE(order == "ab");
    }
}


TEST_CASE("sequential_composite thread marshaling")
{
    SECTION("continues past a buried async step")
    {
        auto worker_thread_id{ std::optional<std::thread::id>{} };
        auto continuation_thread_id{ std::optional<std::thread::id>{} };
        auto continuation_ran{ false };

        auto token{
            he::run(
                he::sequential_composite{
                    he::sequential_composite{
                        he::action{ [] (const he::action::context&) { return true; } },
                        he::async_action{ [&worker_thread_id] (const he::async_action::context&)
                        {
                            worker_thread_id = std::this_thread::get_id();
                            return true;
                        } }
                    },
                    he::action{ [&continuation_thread_id, &continuation_ran] (const he::action::context&)
                    {
                        continuation_thread_id = std::this_thread::get_id();
                        continuation_ran = true;
                        return true;
                    } }
                })
        };

        // proves the continuation is marshaled, not fired inline on the worker thread
        REQUIRE_FALSE(continuation_ran);

        while (!continuation_ran)
        {
            he::exec::scheduler::instance().process();
        }

        REQUIRE(worker_thread_id.has_value());
        REQUIRE(continuation_thread_id.has_value());
        REQUIRE(continuation_thread_id.value() == std::this_thread::get_id());
        REQUIRE(continuation_thread_id.value() != worker_thread_id.value());
    }
}


TEST_CASE("sequential_composite cancel")
{
    SECTION("cascades to an in-flight step")
    {
        auto started{ std::atomic<bool>{ false } };
        auto observed_cancel{ std::atomic<bool>{ false } };

        auto token{
            he::run(
                he::sequential_composite{
                    he::async_action{ [&started, &observed_cancel] (const he::async_action::context&, std::stop_token stop)
                    {
                        started = true;

                        while (!stop.stop_requested())
                        {
                        }

                        observed_cancel = true;

                        return false;
                    } }
                })
        };

        while (!started)
        {
        }

        token.cancel();

        while (!observed_cancel)
        {
        }

        REQUIRE(observed_cancel);

        he::exec::scheduler::instance().process();
    }

    SECTION("mid-flight cancel with and_then/or_else does not crash or clobber state")
    {
        auto started{ std::atomic<bool>{ false } };
        auto worker_done{ std::atomic<bool>{ false } };
        auto then_ran{ false };
        auto otherwise_ran{ false };

        auto token{
            he::run(
                he::sequential_composite{
                    he::async_action{ [&started, &worker_done] (const he::async_action::context&, std::stop_token stop)
                    {
                        started = true;

                        while (!stop.stop_requested())
                        {
                        }

                        worker_done = true;

                        return false;
                    } }
                }
                .and_then(
                    he::action{ [&then_ran] (const he::action::context&)
                    {
                        then_ran = true;
                        return true;
                    } })
                .or_else(
                    he::action{ [&otherwise_ran] (const he::action::context&)
                    {
                        otherwise_ran = true;
                        return true;
                    } }))
        };

        while (!started)
        {
        }

        token.cancel();

        while (!worker_done)
        {
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(20));

        he::exec::scheduler::instance().process();

        REQUIRE_FALSE(then_ran);
        REQUIRE_FALSE(otherwise_ran);
    }
}
