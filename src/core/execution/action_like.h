#pragma once

#include <any>
#include <map>

#include "core/delegate/delegate.hpp"
#include "core/type_traits/type_index.hpp"


namespace he
{

struct exec_token final
{
public:
    auto succeed() -> void;
    auto fail() -> void;

    auto is_succeeded() -> bool;

private:
    bool _succeeded{ false };
};

class action_like
{
public:
    using context = std::map<he::type_index_t, std::any>;

    enum class state
    {
        dormant,
        running,
        succeeded,
        failed,
        aborted
    };

public:
    virtual ~action_like() noexcept = 0;

    virtual auto execute() -> void = 0;
    virtual auto abort() -> void;

    auto get_state() const -> state;

protected:
    virtual auto succeed() -> void;
    virtual auto fail() -> void;

    virtual auto on(state target_state, delegate<> delegate) -> void;

    auto inherit_context() -> void;

protected:
    state _state{ state::dormant };
};

}
