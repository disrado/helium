#pragma once

#include "core/delegate/delegate.hpp"
#include "core/delegate/multicast_delegate.hpp"
#include "core/execution/defs.hpp"
#include "core/execution/task_graph.hpp"

#include <any>
#include <map>
#include <memory>
#include <stop_token>
#include <string>


namespace he::exec
{

struct graph_segment final
{
public:
    task_graph::node& begin;
    task_graph::node& end;
};


class basic_action: public std::enable_shared_from_this<basic_action>
{
public:
    using context = std::map<std::string, std::any>;

    enum class state
    {
        dormant,
        running,
        succeeded,
        failed,
        aborted
    };

public:
    basic_action() = default;

    explicit basic_action(delegate<bool(const context&)> definition, std::optional<context> initial_context = std::nullopt);
    explicit basic_action(delegate<bool(const context&, std::stop_token)> definition, std::optional<context> initial_context = std::nullopt);

    template <typename callable_t>
        requires std::is_invocable_r_v<bool, callable_t, const context&>
              || std::is_invocable_r_v<bool, callable_t, const context&, std::stop_token>
    explicit basic_action(callable_t definition, std::optional<context> initial_context = std::nullopt);

    basic_action(basic_action&&) noexcept = default;
    auto operator=(basic_action&&) noexcept -> basic_action& = default;

    virtual ~basic_action() noexcept = 0;

    virtual auto execute(std::stop_token token = {}) -> void;
    virtual auto abort() -> void;

    auto translate_into_graph(task_graph::node& parent) -> graph_segment;

    auto get_state() const -> state;
    auto get_context() const -> const std::optional<context>&;
    auto set_context(std::optional<context> new_context) -> void;

    template <typename t>
    auto on(state target_state, t&& delegate) -> basic_action&;

protected:
    auto succeed() -> void;
    auto fail() -> void;

    auto set_state(state new_state) -> void;

    auto store_and_then(std::unique_ptr<basic_action> next_action) -> void;
    auto store_or_else(std::unique_ptr<basic_action> next_action) -> void;

    virtual auto expand_on_graph(task_graph::node& parent) -> graph_segment = 0;

protected:
    state _state{ state::dormant };

    std::optional<context> _context;

    std::unique_ptr<basic_action> _then_action;
    std::unique_ptr<basic_action> _else_action;

    std::map<state, multicast_delegate<>> _ons;

private:
    delegate<bool(const context&, std::stop_token)> _definition;

    std::weak_ptr<task_graph> _self_graph;
    task_graph::node* _self_node{ nullptr };
};


template <typename callable_t>
    requires std::is_invocable_r_v<bool, callable_t, const basic_action::context&>
          || std::is_invocable_r_v<bool, callable_t, const basic_action::context&, std::stop_token>
basic_action::basic_action(callable_t definition, std::optional<context> initial_context)
    : _context{ std::move(initial_context) }
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
auto basic_action::on(state target_state, t&& delegate) -> basic_action&
{
    const auto [item, inserted]{ _ons.try_emplace(target_state, multicast_delegate<>{}) };

    item->second.bind(std::forward<t>(delegate));

    return *this;
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
