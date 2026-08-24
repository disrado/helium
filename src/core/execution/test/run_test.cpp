#include "core/execution/action/action.hpp"
#include "core/execution/run.hpp"

#include <catch2/catch_test_macros.hpp>


TEST_CASE("run")
{
    SECTION("execute forwards")
    {
        auto ran{ false };

        const auto chain{ he::run(
            he::action{ [&ran] (const he::action::context&)
            {
                ran = true;
                return true;
            } }) };

        chain.execute();

        REQUIRE(ran);
    }

    SECTION("abort forwards")
    {
        auto fired{ false };

        auto instance{ he::action{ [] (const he::action::context&) { return true; } } };

        instance.on(he::action::state::aborted, [&fired] { fired = true; });

        const auto chain{ he::run(std::move(instance)) };

        chain.abort();

        REQUIRE(fired);
    }

    SECTION("works with a custom action_base subclass, not just he::action")
    {
        static auto custom_execute_ran{ false };
        custom_execute_ran = false;

        class custom_action final: public he::exec::action_base<custom_action>
        {
        public:
            auto execute() -> void override
            {
                custom_execute_ran = true;

                succeed();
            }
        };

        const auto chain{ he::run(custom_action{}) };

        chain.execute();

        REQUIRE(custom_execute_ran);
    }
}


TEST_CASE("run executes a deeply nested chain")
{
    SECTION("only taken path runs")
    {
        auto root_then_ran{ false };
        auto root_otherwise_ran{ false };
        auto inner_then_ran{ false };
        auto inner_otherwise_ran{ false };
        auto leaf_ran{ false };

        const auto chain{
            he::run(
                he::action{ [] (const he::action::context&) { return true; } }
                .then(
                    he::action{ [&root_then_ran] (const he::action::context&)
                    {
                        root_then_ran = true;
                        return false;
                    } }
                    .then(
                        he::action{ [&inner_then_ran] (const he::action::context&)
                        {
                            inner_then_ran = true;
                            return true;
                        } })
                    .otherwise(
                        he::action{ [&inner_otherwise_ran] (const he::action::context&)
                        {
                            inner_otherwise_ran = true;
                            return true;
                        } }
                        .then(
                            he::action{ [&leaf_ran] (const he::action::context&)
                            {
                                leaf_ran = true;
                                return true;
                            } })))
                .otherwise(
                    he::action{ [&root_otherwise_ran] (const he::action::context&)
                    {
                        root_otherwise_ran = true;
                        return true;
                    } })
            )
        };

        chain.execute();

        REQUIRE(root_then_ran);
        REQUIRE_FALSE(root_otherwise_ran);
        REQUIRE_FALSE(inner_then_ran);
        REQUIRE(inner_otherwise_ran);
        REQUIRE(leaf_ran);
    }
}
