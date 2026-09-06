#pragma once

#include "core/delegate/delegate.hpp"
#include "core/execution/defs.hpp"
#include "core/execution/task_graph.hpp"

#include <memory>
#include <stop_token>
#include <type_traits>
#include <vector>


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

    struct link final
    {
        delegate<bool(state)> condition;
        std::shared_ptr<basic_action> next_action;
    };

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

    auto translate_into_graph(task_node& parent) -> graph_segment;

protected:
    virtual auto setup_node(task_node& self_node) -> task_node& = 0;
    auto add_link(delegate<bool(state)> condition, std::shared_ptr<basic_action> next_action) -> void;

private:
    auto translate_links(task_node& self_node) -> void;

protected:
    std::vector<link> _links;

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

    auto and_then(action_like auto next) -> t&&;
    auto or_else(action_like auto next) -> t&&;
};


template <typename t>
auto action_base<t>::and_then(action_like auto next) -> t&&
{
    add_link(
        delegate<bool(state)>{ [] (state s) { return s == state::succeeded; } },
        std::make_shared<decltype(next)>(std::move(next)));

    return std::move(static_cast<t&>(*this));
}


template <typename t>
auto action_base<t>::or_else(action_like auto next) -> t&&
{
    add_link(
        delegate<bool(state)>{ [] (state s) { return s == state::failed; } },
        std::make_shared<decltype(next)>(std::move(next)));

    return std::move(static_cast<t&>(*this));
}

}
