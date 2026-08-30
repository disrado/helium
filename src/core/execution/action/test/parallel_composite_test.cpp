#include "core/execution/action/action.hpp"
#include "core/execution/action/async_action.hpp"
#include "core/execution/action/parallel_composite.hpp"
#include "core/execution/action/sequential_composite.hpp"
#include "core/execution/run.hpp"
#include "core/execution/scheduler.hpp"

#include <catch2/catch_test_macros.hpp>

#include <atomic>
#include <stop_token>
#include <string>


TEST_CASE("parallel_composite")
{
    SECTION("succeeds when all steps succeed")
    {
        auto succeeded{ false };

        auto composite{
            he::parallel_composite{
                he::action{ [] (const he::action::context&) { return true; } },
                he::action{ [] (const he::action::context&) { return true; } },
                he::action{ [] (const he::action::context&) { return true; } }
            }
        };

        composite.on(he::action::state::succeeded, [&succeeded] { succeeded = true; });

        const auto chain{ he::run(std::move(composite)) };

        chain.execute();

        REQUIRE(succeeded);
    }

    SECTION("fails when any step fails, but every step still runs to completion")
    {
        auto failed{ false };
        auto second_ran{ false };
        auto third_ran{ false };

        auto composite{
            he::parallel_composite{
                he::action{ [] (const he::action::context&) { return false; } },
                he::action{ [&second_ran] (const he::action::context&)
                {
                    second_ran = true;
                    return true;
                } },
                he::action{ [&third_ran] (const he::action::context&)
                {
                    third_ran = true;
                    return true;
                } }
            }
        };

        composite.on(he::action::state::failed, [&failed] { failed = true; });

        const auto chain{ he::run(std::move(composite)) };

        chain.execute();

        REQUIRE(failed);
        REQUIRE(second_ran);
        REQUIRE(third_ran);
    }

    SECTION("execute() is repeatable")
    {
        auto run_count{ 0 };

        const auto chain{
            he::run(
                he::parallel_composite{
                    he::action{ [&run_count] (const he::action::context&)
                    {
                        ++run_count;
                        return true;
                    } },
                    he::action{ [&run_count] (const he::action::context&)
                    {
                        ++run_count;
                        return true;
                    } }
                })
        };

        chain.execute();
        chain.execute();

        REQUIRE(run_count == 4);
    }
}


TEST_CASE("parallel_composite nesting")
{
    SECTION("resolves via its own join")
    {
        auto inner_second_ran{ false };
        auto outer_done{ false };

        const auto chain{
            he::run(
                he::sequential_composite{
                    he::parallel_composite{
                        he::action{ [] (const he::action::context&) { return true; } },
                        he::action{ [&inner_second_ran] (const he::action::context&)
                        {
                            inner_second_ran = true;
                            return true;
                        } }
                    },
                    he::action{ [&outer_done] (const he::action::context&)
                    {
                        outer_done = true;
                        return true;
                    } }
                })
        };

        chain.execute();

        REQUIRE(inner_second_ran);
        REQUIRE(outer_done);
    }
}


TEST_CASE("parallel_composite chaining")
{
    SECTION("then runs when all steps succeed")
    {
        auto then_ran{ false };

        const auto chain{
            he::run(
                he::parallel_composite{
                    he::action{ [] (const he::action::context&) { return true; } },
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

    SECTION("otherwise runs when any step fails")
    {
        auto otherwise_ran{ false };

        const auto chain{
            he::run(
                he::parallel_composite{
                    he::action{ [] (const he::action::context&) { return true; } },
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

    SECTION("merges context from every step")
    {
        auto received{ he::action::context{} };

        auto first_context{ he::action::context{ { "first", std::string{ "a" } } } };
        auto second_context{ he::action::context{ { "second", std::string{ "b" } } } };

        const auto chain{
            he::run(
                he::parallel_composite{
                    he::action{ [] (const he::action::context&) { return true; }, first_context },
                    he::action{ [] (const he::action::context&) { return true; }, second_context }
                }
                .then(
                    he::action{ [&received] (const he::action::context& ctx)
                    {
                        received = ctx;
                        return true;
                    } }))
        };

        chain.execute();

        REQUIRE(std::any_cast<std::string>(received.at("first")) == "a");
        REQUIRE(std::any_cast<std::string>(received.at("second")) == "b");
    }

    SECTION("later step wins on context key collision")
    {
        auto received{ he::action::context{} };

        auto first_context{ he::action::context{ { "key", std::string{ "first" } } } };
        auto second_context{ he::action::context{ { "key", std::string{ "second" } } } };

        const auto chain{
            he::run(
                he::parallel_composite{
                    he::action{ [] (const he::action::context&) { return true; }, first_context },
                    he::action{ [] (const he::action::context&) { return true; }, second_context }
                }
                .then(
                    he::action{ [&received] (const he::action::context& ctx)
                    {
                        received = ctx;
                        return true;
                    } }))
        };

        chain.execute();

        REQUIRE(std::any_cast<std::string>(received.at("key")) == "second");
    }
}


TEST_CASE("parallel_composite with async step")
{
    SECTION("waits for both a sync and an async step before resolving")
    {
        auto sync_ran{ std::atomic<bool>{ false } };
        auto async_ran{ std::atomic<bool>{ false } };
        auto succeeded{ std::atomic<bool>{ false } };

        const auto chain{
            he::run(
                he::parallel_composite{
                    he::action{ [&sync_ran] (const he::action::context&)
                    {
                        sync_ran = true;
                        return true;
                    } },
                    he::async_action{ [&async_ran] (const he::async_action::context&)
                    {
                        async_ran = true;
                        return true;
                    } }
                }
                .then(
                    he::action{ [&succeeded] (const he::action::context&)
                    {
                        succeeded = true;
                        return true;
                    } }))
        };

        chain.execute();

        while (!succeeded)
        {
            he::exec::scheduler::instance().process();
        }

        REQUIRE(sync_ran);
        REQUIRE(async_ran);
    }
}


TEST_CASE("parallel_composite abort")
{
    SECTION("cascades to every step")
    {
        auto first{ he::action{ [] (const he::action::context&) { return true; } } };
        auto second{ he::action{ [] (const he::action::context&) { return true; } } };

        auto first_aborted{ false };
        auto second_aborted{ false };

        first.on(he::action::state::aborted, [&first_aborted] { first_aborted = true; });
        second.on(he::action::state::aborted, [&second_aborted] { second_aborted = true; });

        auto composite{ he::parallel_composite{ std::move(first), std::move(second) } };

        const auto chain{ he::run(std::move(composite)) };

        chain.abort();

        REQUIRE(first_aborted);
        REQUIRE(second_aborted);
    }

    SECTION("cascades to untranslated steps")
    {
        auto first_aborted{ false };

        auto first{ he::action{ [] (const he::action::context&) { return true; } } };

        first.on(he::action::state::aborted, [&first_aborted] { first_aborted = true; });

        auto composite{ he::parallel_composite{ std::move(first) } };

        const auto chain{ he::run(std::move(composite)) };

        chain.abort();

        REQUIRE(first_aborted);
    }

    SECTION("cascades to an in-flight step")
    {
        auto started{ std::atomic<bool>{ false } };
        auto observed_cancel{ std::atomic<bool>{ false } };

        auto composite{
            he::parallel_composite{
                he::action{ [] (const he::action::context&) { return true; } },
                he::async_action{ [&started, &observed_cancel] (const he::async_action::context&, std::stop_token token)
                {
                    started = true;

                    while (!token.stop_requested())
                    {
                    }

                    observed_cancel = true;

                    return false;
                } }
            }
        };

        const auto chain{ he::run(std::move(composite)) };

        chain.execute();

        while (!started)
        {
        }

        chain.abort();

        while (!observed_cancel)
        {
        }

        REQUIRE(observed_cancel);

        he::exec::scheduler::instance().process();
    }
}
