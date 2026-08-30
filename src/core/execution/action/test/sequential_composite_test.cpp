#include "core/execution/action/action.hpp"
#include "core/execution/action/async_action.hpp"
#include "core/execution/action/sequential_composite.hpp"
#include "core/execution/run.hpp"

#include <catch2/catch_test_macros.hpp>

#include <atomic>
#include <optional>
#include <string>


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

    SECTION("stops after a step fails")
    {
        auto second_ran{ false };
        auto third_ran{ false };

        const auto chain{
            he::run(
                he::sequential_composite{
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
                })
        };

        chain.execute();

        REQUIRE_FALSE(second_ran);
        REQUIRE_FALSE(third_ran);
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

    SECTION("context propagates into nested composite's first step")
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

    SECTION("context propagates from last step to then branch")
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
        }

        REQUIRE(order == "ab");
    }
}
