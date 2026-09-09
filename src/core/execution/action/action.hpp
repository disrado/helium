#pragma once

#include "core/execution/action/action_base.hpp"

#include <type_traits>


namespace he
{

class action: public exec::action_base<action>
{
public:
    using state = exec::basic_action::state;
    using context = exec::basic_action::context;
    using result = exec::task_result;

    action() = default;

    explicit action(delegate<bool(const context&)> definition);
    explicit action(delegate<bool(const context&, std::stop_token)> definition);

    template <typename callable_t>
        requires std::is_invocable_r_v<bool, callable_t, const context&>
                 || std::is_invocable_r_v<bool, callable_t, const context&, std::stop_token>
    explicit action(callable_t definition);

    virtual auto execute(exec::task_node& self_node, std::stop_token token = {}) -> result;

protected:
    auto setup_node(exec::task_node& self_node) -> exec::task_node& override;

private:
    delegate<bool(const context&, std::stop_token)> _definition;
};


template <typename callable_t>
    requires std::is_invocable_r_v<bool, callable_t, const action::context&>
             || std::is_invocable_r_v<bool, callable_t, const action::context&, std::stop_token>
action::action(callable_t definition)
{
    if constexpr (std::is_invocable_r_v<bool, callable_t, const context&, std::stop_token>)
    {
        _definition = delegate{ std::move(definition) };
    }
    else
    {
        _definition = delegate{[fn{ std::move(definition) }] (const context& ctx, std::stop_token) mutable { return fn(ctx); } };
    }
}

}
