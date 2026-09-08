#pragma once

#include "core/execution/action/action_base.hpp"

#include <type_traits>


namespace he
{

class ticking_action final: public exec::action_base<ticking_action>
{
public:
    enum class result : uint8_t
    {
        running,
        succeeded,
        failed
    };

public:
    using state = exec::basic_action::state;
    using context = exec::basic_action::context;

    template <typename callable_t>
        requires std::is_invocable_r_v<result, callable_t, const context&, std::stop_token>
    explicit ticking_action(callable_t definition);

    auto execute(exec::task_node& self_node, std::stop_token token) -> void override;

protected:
    auto setup_node(exec::task_node& self_node) -> exec::task_node& override;

private:
    delegate<result(const context&, std::stop_token)> _definition;
};


template <typename callable_t>
    requires std::is_invocable_r_v<ticking_action::result, callable_t, const ticking_action::context&, std::stop_token>
ticking_action::ticking_action(callable_t definition)
    : _definition{ std::move(definition) }
{
}

}
