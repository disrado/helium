#pragma once

#include "action_like.h"


namespace he
{

class action : public action_like
{
public:
    action(delegate<exec_token, const context&> definition, context initial_context);

    auto execute() -> void override;

protected:
    auto succeed() -> void override;
    auto fail() -> void override;

    auto on(state target_state, delegate<> delegate) -> void override;

private:
    delegate<exec_token, const context&> _definition;

    exec_token _exec_token;

    context _context;

    delegate<> _on_success;
    delegate<> _on_failure;
};

}
