#pragma once

#include "core/delegate/delegate.hpp"
#include "core/execution/defs.hpp"
#include "core/execution/task_graph.hpp"

#include <memory>
#include <optional>
#include <stop_token>
#include <type_traits>


namespace he::exec
{

struct graph_segment final
{
public:
    task_graph::node& begin;
    task_graph::node& end;
};


class execution_token;


class basic_action
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

    virtual ~basic_action() noexcept = 0;

    virtual auto execute(task_graph::node& self, std::stop_token token = {}) -> void;

    virtual auto translate_into_graph(task_graph::node& parent) -> graph_segment = 0;

    // explicit-object parameter deduces the *actual* most-derived type at each call site,
    // unlike action_base<t>'s CRTP t (fixed at the first CRTP level, which would slice anything
    // subclassed a level further, e.g. a custom_action : action)
    template <typename self_t>
    auto run(this self_t&& self, std::optional<action_context> initial_context = std::nullopt) -> execution_token;

protected:
    auto store_and_then(std::unique_ptr<basic_action> next_action) -> void;
    auto store_or_else(std::unique_ptr<basic_action> next_action) -> void;

protected:
    std::unique_ptr<basic_action> _then_action;
    std::unique_ptr<basic_action> _else_action;

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


class execution_token final
{
public:
    auto cancel() const -> void;

private:
    friend class basic_action;

    execution_token(std::shared_ptr<basic_action> root, std::shared_ptr<task_graph> graph);

    std::shared_ptr<basic_action> _root;
    std::shared_ptr<task_graph> _graph;
};


template <typename self_t>
auto basic_action::run(this self_t&& self, std::optional<action_context> initial_context) -> execution_token
{
    auto root{ std::make_shared<std::decay_t<self_t>>(std::forward<self_t>(self)) };
    auto graph{ std::make_shared<task_graph>() };
    auto segment{ root->translate_into_graph(graph->root()) };

    segment.begin.anchor = root;
    segment.begin.context = std::move(initial_context);

    graph->activate(segment.begin);

    return execution_token{ std::move(root), std::move(graph) };
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
    store_and_then(std::make_unique<decltype(next)>(std::move(next)));

    return std::move(static_cast<t&>(*this));
}


template <typename t>
auto action_base<t>::otherwise(action_like auto next) -> t&&
{
    store_or_else(std::make_unique<decltype(next)>(std::move(next)));

    return std::move(static_cast<t&>(*this));
}

}
