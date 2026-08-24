#pragma once

#include "core/delegate/delegate.hpp"

#include <any>
#include <map>
#include <string>

#include "core/delegate/multicast_delegate.hpp"
#include "core/execution/utils.hpp"


namespace he
{

class action_base
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

    struct exec_token final
    {
    public:
        auto succeed() -> void;
        auto fail() -> void;

        auto is_succeeded() const -> bool;

    private:
        bool _succeeded{ false };
    };

public:
    action_base() = default;
    explicit action_base(delegate<exec_token&, const context&> definition, std::optional<context> initial_context = std::nullopt);

    action_base(action_base&&) = default;
    auto operator=(action_base&&) -> action_base& = default;

    virtual ~action_base() noexcept;

    virtual auto execute() -> void;
    virtual auto abort() -> void;

    auto get_state() const -> state;

    virtual auto on(state target_state, delegate<> delegate) -> action_base&;

    auto inherit_context(context&& target_context) -> void;

protected:
    virtual auto on_success() -> void;
    virtual auto on_failure() -> void;

    auto succeed() -> void;
    auto fail() -> void;

    friend class run;

    virtual auto setup() -> void;

    auto store_and_then(std::unique_ptr<action_base> next_action) -> void;
    auto store_or_else(std::unique_ptr<action_base> action) -> void;

    static auto allocate(action_like auto target) -> std::unique_ptr<action_base>;

protected:
    state _state{ state::dormant };

    std::optional<context> _context;

    multicast_delegate<> _on_success;
    multicast_delegate<> _on_failure;
    multicast_delegate<> _on_abort;

    std::unique_ptr<action_base> _then_action;
    std::unique_ptr<action_base> _else_action;

private:
    delegate<exec_token&, const context&> _definition;

    exec_token _exec_token;
};


// then/otherwise has to return rvalue in order to allow composites chaining based on values (basically keep it clean)
// naive implementation of this produces an issue: in order to to chain then/otherwise we have to call setup(), but when we do
// then on raw object and later move it - this in lambda-captured contexts (during setup) invalidates, which means we have to
// allocate instead. Options are:
// - external management - makes semantics ugly
// - allocate state as a separate object - requires every composite to create a separate state class
// - make a crtp wrapper which would be the mad in the middle which wraps us into allocation - this is what we're going with
template <typename t>
class action: public action_base
{
public:
    action() = default;
    explicit action(delegate<exec_token&, const context&> definition, std::optional<context> initial_context = std::nullopt);

    auto then(action_like auto next) -> t&&;
    auto otherwise(action_like auto next) -> t&&;

private:
    auto allocate(action_like auto next) -> std::unique_ptr<action_base>;
};

template <typename t>
action<t>::action(delegate<exec_token&, const context&> definition, std::optional<context> initial_context)
    : action_base(std::move(definition), std::move(initial_context))
{
}


template <typename t>
auto action<t>::then(action_like auto next) -> t&&
{
    store_and_then(allocate(std::move(next)));

    return std::move(static_cast<t&>(*this));
}


template <typename t>
auto action<t>::otherwise(action_like auto next) -> t&&
{
    store_or_else(allocate(std::move(next)));
    return std::move(static_cast<t&>(*this));
}


template <typename t>
auto action<t>::allocate(action_like auto next) -> std::unique_ptr<action_base>
{
    auto allocation{ std::make_unique<decltype(next)>(std::move(next)) };

    allocation->setup();

    return allocation;
}


}
