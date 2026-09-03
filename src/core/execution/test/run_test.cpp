#include "core/execution/action/action.hpp"
#include "core/execution/action/async_action.hpp"
#include "core/execution/action/parallel_composite.hpp"
#include "core/execution/action/sequential_composite.hpp"
#include "core/execution/scheduler.hpp"

#include <catch2/catch_test_macros.hpp>

#include <any>
#include <atomic>
#include <stop_token>
#include <string>
#include <tuple>
#include <utility>


TEST_CASE("run")
{
    SECTION("execute forwards")
    {
        auto ran{ false };

        auto token{
            he::action{ [&ran] (const he::action::context&)
            {
                ran = true;
                return true;
            } }.run()
        };

        REQUIRE(ran);
    }

    SECTION("cancel cancels an in-flight async task")
    {
        auto started{ std::atomic<bool>{ false } };
        auto observed_cancel{ std::atomic<bool>{ false } };

        auto token{
            he::async_action{ [&started, &observed_cancel] (const he::async_action::context&, std::stop_token stop)
            {
                started = true;

                while (!stop.stop_requested())
                {
                }

                observed_cancel = true;

                return false;
            } }.run()
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

    SECTION("works with a custom action_base subclass, not just he::action")
    {
        static auto custom_execute_ran{ false };
        custom_execute_ran = false;

        class custom_action final: public he::action
        {
        public:
            auto execute(he::exec::task_graph::node& self, std::stop_token) -> void override
            {
                custom_execute_ran = true;

                self.state = state::succeeded;
            }
        };

        auto token{ custom_action{}.run() };

        REQUIRE(custom_execute_ran);
    }

    SECTION("survives async completion after execute() returns")
    {
        auto done{ std::atomic<bool>{ false } };

        auto token{
            he::async_action{ [] (const he::async_action::context&) { return true; } }
            .then(
                he::action{ [&done] (const he::action::context&)
                {
                    done = true;
                    return true;
                } })
            .run()
        };

        while (!done)
        {
            he::exec::scheduler::instance().process();
        }

        REQUIRE(done);
    }
}


TEST_CASE("run example")
{
    SECTION("nested composites, branching, propagated context")
    {
        class plain_action final: public he::action
        {
        public:
            auto execute(he::exec::task_graph::node& self, std::stop_token) -> void override
            {
                self.state = state::succeeded;
            }
        };

        class label_reader_action final: public he::action
        {
        public:
            auto execute(he::exec::task_graph::node& self, std::stop_token) -> void override
            {
                std::ignore = std::any_cast<std::string>(self.context.value().at("label"));

                self.state = state::failed;
            }
        };

        auto done{ std::atomic<bool>{ false } };

        auto initial_context{ he::action::context{ { "label", std::string{ "run" } } } };

        auto token{
            he::sequential_composite{
                he::action{ [] (const auto&) { return true; } }
                    .then(he::action{ [] (const auto&) { return true; } }),
                he::async_action{ [] (const he::async_action::context&) { return true; } }
                    .then(he::sequential_composite{
                        he::async_action{ [] (const he::async_action::context&) { return true; } },
                        he::async_action{ [] (const he::async_action::context&) { return true; } }
                            .then(plain_action{}
                                .then(he::parallel_composite{
                                    he::async_action{ [] (const he::async_action::context&) { return true; } },
                                    he::async_action{ [] (const he::async_action::context&) { return true; } },
                                    plain_action{},
                                    he::async_action{ [] (const he::async_action::context&) { return true; } }
                                        .then(plain_action{}),
                                    plain_action{},
                                    plain_action{}
                                }))
                            .otherwise(label_reader_action{}),
                        plain_action{},
                        he::async_action{ [] (const he::async_action::context&) { return true; } },
                        plain_action{}
                    }),
                label_reader_action{}
            }
            .then(
                he::action{ [&] (const auto&)
                {
                    done = true;
                    return true;
                } })
            .otherwise(
                he::action{ [&] (const auto&)
                {
                    done = true;
                    return true;
                } })
            .run(std::move(initial_context))
        };

        while (!done)
        {
            he::exec::scheduler::instance().process();
        }
    }
}
