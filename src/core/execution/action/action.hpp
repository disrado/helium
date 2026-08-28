#pragma once

#include "core/delegate/delegate.hpp"
#include "core/delegate/multicast_delegate.hpp"
#include "core/execution/utils.hpp"

#include <any>
#include <map>
#include <string>


namespace he::exec
{

class basic_action
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

    template <typename callable_t>
        requires std::is_invocable_r_v<bool, callable_t, const context&>
    explicit basic_action(callable_t definition, std::optional<context> initial_context = std::nullopt);

    basic_action(basic_action&&) noexcept = default;
    auto operator=(basic_action&&) noexcept -> basic_action& = default;

    virtual ~basic_action() noexcept = 0;

    virtual auto execute() -> void;
    virtual auto abort() -> void;

    auto get_state() const -> state;

    template <typename t>
    auto on(state target_state, t&& delegate) -> basic_action&;

protected:
    virtual auto on_success() -> void;
    virtual auto on_failure() -> void;

    auto succeed() -> void;
    auto fail() -> void;

    auto set_state(state new_state) -> void;

    auto store_and_then(std::unique_ptr<basic_action> next_action) -> void;
    auto store_or_else(std::unique_ptr<basic_action> next_action) -> void;

protected:
    state _state{ state::dormant };

    std::optional<context> _context;

    std::unique_ptr<basic_action> _then_action;
    std::unique_ptr<basic_action> _else_action;

    std::map<state, multicast_delegate<>> _ons;

private:
    delegate<bool(const context&)> _definition;
};


template <typename callable_t>
    requires std::is_invocable_r_v<bool, callable_t, const basic_action::context&>
basic_action::basic_action(callable_t definition, std::optional<context> initial_context)
    : _context{ std::move(initial_context) }
    , _definition{ std::move(definition) }
{
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


namespace he
{

class action: public exec::action_base<action>
{
public:
    using state = exec::basic_action::state;
    using context = exec::basic_action::context;
    using action_base::action_base;
};

}
