#include "core/execution/action/action.hpp"
#include "core/execution/action/async_action.hpp"
#include "core/execution/action/parallel_composite.hpp"
#include "core/execution/action/sequential_composite.hpp"
#include "core/execution/scheduler.hpp"

#include <catch2/catch_test_macros.hpp>

#include <any>
#include <atomic>
#include <chrono>
#include <stop_token>
#include <string>
#include <thread>
#include <utility>


namespace
{

// leaves output context directly on its own node, standing in for "whatever this step
// produced" since actions no longer carry per-instance seed state to pre-fill it with
class context_action final: public he::action
{
public:
    explicit context_action(he::action::context ctx)
        : _ctx{ std::move(ctx) }
    {
    }

    auto execute(he::exec::task_graph::node& self, std::stop_token) -> void override
    {
        self.context = _ctx;
        self.state = state::succeeded;
    }

private:
    he::action::context _ctx;
};

}


TEST_CASE("parallel_composite")
{
    SECTION("succeeds when all steps succeed")
    {
        auto succeeded{ false };

        auto token{
            he::parallel_composite{
                he::action{ [] (const he::action::context&) { return true; } },
                he::action{ [] (const he::action::context&) { return true; } },
                he::action{ [] (const he::action::context&) { return true; } }
            }
            .then(he::action{ [&succeeded] (const he::action::context&)
            {
                succeeded = true;
                return true;
            } })
            .run()
        };

        REQUIRE(succeeded);
    }

    SECTION("fails when any step fails, but every step still runs to completion")
    {
        auto failed{ false };
        auto second_ran{ false };
        auto third_ran{ false };

        auto token{
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
            .otherwise(he::action{ [&failed] (const he::action::context&)
            {
                failed = true;
                return true;
            } })
            .run()
        };

        REQUIRE(failed);
        REQUIRE(second_ran);
        REQUIRE(third_ran);
    }
}


TEST_CASE("parallel_composite nesting")
{
    SECTION("resolves via its own join")
    {
        auto inner_second_ran{ false };
        auto outer_done{ false };

        auto token{
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
            }.run()
        };

        REQUIRE(inner_second_ran);
        REQUIRE(outer_done);
    }
}


TEST_CASE("parallel_composite chaining")
{
    SECTION("then runs when all steps succeed")
    {
        auto then_ran{ false };

        auto token{
            he::parallel_composite{
                he::action{ [] (const he::action::context&) { return true; } },
                he::action{ [] (const he::action::context&) { return true; } }
            }
            .then(
                he::action{ [&then_ran] (const he::action::context&)
                {
                    then_ran = true;
                    return true;
                } })
            .run()
        };

        REQUIRE(then_ran);
    }

    SECTION("otherwise runs when any step fails")
    {
        auto otherwise_ran{ false };

        auto token{
            he::parallel_composite{
                he::action{ [] (const he::action::context&) { return true; } },
                he::action{ [] (const he::action::context&) { return false; } }
            }
            .otherwise(
                he::action{ [&otherwise_ran] (const he::action::context&)
                {
                    otherwise_ran = true;
                    return true;
                } })
            .run()
        };

        REQUIRE(otherwise_ran);
    }

    SECTION("merges context from every step")
    {
        auto received{ he::action::context{} };

        auto first_context{ he::action::context{ { "first", std::string{ "a" } } } };
        auto second_context{ he::action::context{ { "second", std::string{ "b" } } } };

        auto token{
            he::parallel_composite{
                context_action{ first_context },
                context_action{ second_context }
            }
            .then(
                he::action{ [&received] (const he::action::context& ctx)
                {
                    received = ctx;
                    return true;
                } })
            .run()
        };

        REQUIRE(std::any_cast<std::string>(received.at("first")) == "a");
        REQUIRE(std::any_cast<std::string>(received.at("second")) == "b");
    }

    SECTION("later step wins on context key collision")
    {
        auto received{ he::action::context{} };

        auto first_context{ he::action::context{ { "key", std::string{ "first" } } } };
        auto second_context{ he::action::context{ { "key", std::string{ "second" } } } };

        auto token{
            he::parallel_composite{
                context_action{ first_context },
                context_action{ second_context }
            }
            .then(
                he::action{ [&received] (const he::action::context& ctx)
                {
                    received = ctx;
                    return true;
                } })
            .run()
        };

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

        auto token{
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
                } })
            .run()
        };

        while (!succeeded)
        {
            he::exec::scheduler::instance().process();
        }

        REQUIRE(sync_ran);
        REQUIRE(async_ran);
    }
}


TEST_CASE("parallel_composite cancel")
{
    SECTION("cascades to an in-flight step")
    {
        auto started{ std::atomic<bool>{ false } };
        auto observed_cancel{ std::atomic<bool>{ false } };

        auto token{
            he::parallel_composite{
                he::action{ [] (const he::action::context&) { return true; } },
                he::async_action{ [&started, &observed_cancel] (const he::async_action::context&, std::stop_token stop)
                {
                    started = true;

                    while (!stop.stop_requested())
                    {
                    }

                    observed_cancel = true;

                    return false;
                } }
            }.run()
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

    SECTION("mid-flight cancel with then/otherwise does not crash or clobber state")
    {
        auto started{ std::atomic<bool>{ false } };
        auto worker_done{ std::atomic<bool>{ false } };
        auto then_ran{ false };
        auto otherwise_ran{ false };

        auto token{
            he::parallel_composite{
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
            .then(
                he::action{ [&then_ran] (const he::action::context&)
                {
                    then_ran = true;
                    return true;
                } })
            .otherwise(
                he::action{ [&otherwise_ran] (const he::action::context&)
                {
                    otherwise_ran = true;
                    return true;
                } })
            .run()
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
