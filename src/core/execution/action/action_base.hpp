#pragma once

#include "core/delegate/delegate.hpp"
#include "core/execution/defs.hpp"
#include "core/execution/task_graph.hpp"

#include <memory>
#include <stop_token>
#include <type_traits>


namespace he::exec
{

struct graph_segment final
{
public:
    task_node& start;
    task_node& end;
};


class basic_action: public std::enable_shared_from_this<basic_action>
{
public:
    using state = action_state;
    using context = action_context;

public:
    basic_action() = default;

    explicit basic_action(delegate<bool(const context&)> definition);
    explicit basic_action(delegate<bool(const context&, std::stop_token)> definition);

    template <typename callable_t>
        requires std::is_invocable_r_v<bool, callable_t, const context&>
                 || std::is_invocable_r_v<bool, callable_t, const context&, std::stop_token>
    explicit basic_action(callable_t definition);

    basic_action(basic_action&&) noexcept = default;
    auto operator=(basic_action&&) noexcept -> basic_action& = default;

    virtual ~basic_action() noexcept = default;

    virtual auto execute(task_node& self_node, std::stop_token token = {}) -> void;

    virtual auto translate_into_graph(task_node& parent) -> graph_segment = 0;

protected:
    auto store_and_then(std::shared_ptr<basic_action> next_action) -> void;
    auto store_or_else(std::shared_ptr<basic_action> next_action) -> void;

protected:
    std::shared_ptr<basic_action> _then_action;
    std::shared_ptr<basic_action> _else_action;

private:
    delegate<bool(const context&, std::stop_token)> _definition;
};


template <typename callable_t>
    requires std::is_invocable_r_v<bool, callable_t, const basic_action::context&>
             || std::is_invocable_r_v<bool, callable_t, const basic_action::context&, std::stop_token>
basic_action::basic_action(callable_t definition)
{
    if constexpr (std::is_invocable_r_v<bool, callable_t, const basic_action::context&, std::stop_token>)
    {
        _definition = delegate<bool(const context&, std::stop_token)>{ std::move(definition) };
    }
    else
    {
        _definition = delegate<bool(const context&, std::stop_token)>{
            [fn{ std::move(definition) }] (const context& ctx, std::stop_token) mutable { return fn(ctx); } };
    }
}


template <typename t>
class action_base: public basic_action
{
public:
    using basic_action::basic_action;

    auto then(action_like auto next) -> t&&;
    auto otherwise(action_like auto next) -> t&&;
};


template <typename t>
auto action_base<t>::then(action_like auto next) -> t&&
{
    store_and_then(std::make_shared<decltype(next)>(std::move(next)));

    return std::move(static_cast<t&>(*this));
}


template <typename t>
auto action_base<t>::otherwise(action_like auto next) -> t&&
{
    store_or_else(std::make_shared<decltype(next)>(std::move(next)));

    return std::move(static_cast<t&>(*this));
}

}
