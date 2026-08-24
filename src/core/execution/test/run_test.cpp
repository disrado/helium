#include "core/execution/action/action.hpp"
#include "core/execution/run.hpp"

#include <catch2/catch_test_macros.hpp>

#include <print>


TEST_CASE("single execution")
{
    class custom_action: public he::action<custom_action>
    {
    public:
        auto execute() -> void override
        {
            std::println("custom_action");

            succeed();
        }
    };

    const auto chain{
        he::run(
            custom_action{}
            .then(
                custom_action{}
                .then(
                    custom_action{}
                    .then(custom_action{}))
                .otherwise(custom_action{}))
            .otherwise(custom_action{})
        )
    };

    chain.execute();
}
