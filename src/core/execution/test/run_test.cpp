#include "../action/action.h"
#include "core/execution/run.hpp"
#include "../composite/single.hpp"

#include <catch2/catch_test_macros.hpp>

#include <print>


TEST_CASE("single execution")
{
    auto action = he::action{
        he::delegate<he::action::exec_token&, const he::action_base::context&>{
            [] (auto& token, const auto& _)
            {
                token.fail();
            }
        }
    };

    auto then_action = he::action{
        he::delegate<he::action::exec_token&, const he::action_base::context&>{
            [] ([[maybe_unused]] auto& token, const auto& _)
            {
                std::println("then action");
            }
        }
    };

    auto else_action = he::action{
        he::delegate<he::action::exec_token&, const he::action_base::context&>{
            [] ([[maybe_unused]] auto& token, const auto& _)
            {
                std::println("else action");
            }
        }
    };

    auto runner{ he::run(he::single{ action })
        .and_then(he::single{ then_action })
        .or_else(he::single{ else_action })
    };

    runner.execute();
}
