#include "core/execution/action/action.hpp"
#include "core/execution/action/async_action.hpp"
#include "core/execution/action/sequential_composite.hpp"
#include "core/execution/run.hpp"
#include "core/execution/scheduler.hpp"

#include <catch2/catch_test_macros.hpp>

#include <atomic>
#include <optional>
#include <string>
#include <thread>


TEST_CASE("sequential_composite")
{
    SECTION("runs steps in order")
    {
        auto order{ std::string{} };

        const auto chain{
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

        chain.execute();

        REQUIRE(order == "abc");
    }

    SECTION("stops after a mid-chain step fails")
    {
        auto third_ran{ false };

        const auto chain{
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

        chain.execute();

        REQUIRE_FALSE(third_ran);
    }

    SECTION("fails on mid-chain failure")
    {
        auto failed{ false };

        auto composite{
            he::sequential_composite{
                he::action{ [] (const he::action::context&) { return true; } },
                he::action{ [] (const he::action::context&) { return false; } },
                he::action{ [] (const he::action::context&) { return true; } }
            }
        };

        composite.on(he::action::state::failed, [&failed] { failed = true; });

        const auto chain{ he::run(std::move(composite)) };

        chain.execute();

        REQUIRE(failed);
    }

    SECTION("context propagates to next sibling")
    {
        auto received{ std::optional<std::string>{} };

        auto initial_context{ he::action::context{ { "label", std::string{ "run" } } } };

        const auto chain{
            he::run(
                he::sequential_composite{
                    he::action{ [] (const he::action::context&) { return true; }, initial_context },
                    he::action{ [&received] (const he::action::context& ctx)
                    {
                        if (ctx.contains("label"))
                        {
                            received = std::any_cast<std::string>(ctx.at("label"));
                        }

                        return true;
                    } }
                })
        };

        chain.execute();

        REQUIRE(received.has_value());
        REQUIRE(received.value() == "run");
    }

    SECTION("context propagates into nested composite")
    {
        auto received{ std::optional<std::string>{} };

        auto initial_context{ he::action::context{ { "label", std::string{ "run" } } } };

        const auto chain{
            he::run(
                he::sequential_composite{
                    he::action{ [] (const he::action::context&) { return true; }, initial_context },
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
                })
        };

        chain.execute();

        REQUIRE(received.has_value());
        REQUIRE(received.value() == "run");
    }

    SECTION("succeeds when last step succeeds")
    {
        auto succeeded{ false };

        auto composite{
            he::sequential_composite{
                he::action{ [] (const he::action::context&) { return true; } },
                he::action{ [] (const he::action::context&) { return true; } }
            }
        };

        composite.on(he::action::state::succeeded, [&succeeded] { succeeded = true; });

        const auto chain{ he::run(std::move(composite)) };

        chain.execute();

        REQUIRE(succeeded);
    }

    SECTION("fails when last step fails")
    {
        auto failed{ false };

        auto composite{
            he::sequential_composite{
                he::action{ [] (const he::action::context&) { return true; } },
                he::action{ [] (const he::action::context&) { return false; } }
            }
        };

        composite.on(he::action::state::failed, [&failed] { failed = true; });

        const auto chain{ he::run(std::move(composite)) };

        chain.execute();

        REQUIRE(failed);
    }
}


TEST_CASE("sequential_composite chaining")
{
    SECTION("then runs when composite succeeds")
    {
        auto then_ran{ false };

        const auto chain{
            he::run(
                he::sequential_composite{
                    he::action{ [] (const he::action::context&) { return true; } }
                }
                .then(
                    he::action{ [&then_ran] (const he::action::context&)
                    {
                        then_ran = true;
                        return true;
                    } }))
        };

        chain.execute();

        REQUIRE(then_ran);
    }

    SECTION("otherwise runs when composite fails")
    {
        auto otherwise_ran{ false };

        const auto chain{
            he::run(
                he::sequential_composite{
                    he::action{ [] (const he::action::context&) { return false; } }
                }
                .otherwise(
                    he::action{ [&otherwise_ran] (const he::action::context&)
                    {
                        otherwise_ran = true;
                        return true;
                    } }))
        };

        chain.execute();

        REQUIRE(otherwise_ran);
    }

    SECTION("otherwise runs on mid-chain failure")
    {
        auto otherwise_ran{ false };

        const auto chain{
            he::run(
                he::sequential_composite{
                    he::action{ [] (const he::action::context&) { return true; } },
                    he::action{ [] (const he::action::context&) { return false; } },
                    he::action{ [] (const he::action::context&) { return true; } }
                }
                .otherwise(
                    he::action{ [&otherwise_ran] (const he::action::context&)
                    {
                        otherwise_ran = true;
                        return true;
                    } }))
        };

        chain.execute();

        REQUIRE(otherwise_ran);
    }

    SECTION("context propagates to then branch")
    {
        auto received{ std::optional<std::string>{} };

        auto initial_context{ he::action::context{ { "label", std::string{ "run" } } } };

        const auto chain{
            he::run(
                he::sequential_composite{
                    he::action{ [] (const he::action::context&) { return true; }, initial_context }
                }
                .then(
                    he::action{ [&received] (const he::action::context& ctx)
                    {
                        if (ctx.contains("label"))
                        {
                            received = std::any_cast<std::string>(ctx.at("label"));
                        }

                        return true;
                    } }))
        };

        chain.execute();

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

        const auto chain{
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

        chain.execute();

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

        const auto chain{
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

        chain.execute();

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


TEST_CASE("sequential_composite abort")
{
    SECTION("does not cascade to steps")
    {
        auto step_aborted{ false };

        auto step{ he::action{ [] (const he::action::context&) { return true; } } };

        step.on(he::action::state::aborted, [&step_aborted] { step_aborted = true; });

        auto composite{ he::sequential_composite{ std::move(step) } };

        const auto chain{ he::run(std::move(composite)) };

        chain.abort();

        REQUIRE_FALSE(step_aborted);
    }
}
