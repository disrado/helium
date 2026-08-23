#pragma once

#include "core/delegate/delegate.hpp"

#include <any>
#include <map>
#include <string>

#include "core/delegate/multicast_delegate.hpp"


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

public:
    action_base(std::optional<context> initial_context = std::nullopt);
    virtual ~action_base() noexcept = 0;

    virtual auto execute() -> void = 0;
    virtual auto abort() -> void = 0;

    auto get_state() const -> state;

    virtual auto on(state target_state, delegate<> delegate) -> action_base&;

    auto inherit_context(context&& target_context) -> void;

protected:
    virtual auto succeed() -> void;
    virtual auto fail() -> void;

protected:
    state _state{ state::dormant };

    std::optional<context> _context;

    multicast_delegate<> _on_success;
    multicast_delegate<> _on_failure;
    multicast_delegate<> _on_abort;
};

}
