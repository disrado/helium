#include "action.h"


namespace he
{

action::action(delegate<exec_token, const context&> definition, context initial_context)
    : _definition{ definition }
    , _context{ initial_context }
{

}

auto action::execute() -> void
{
    if (_definition.try_execute(_exec_token, _context))
    {
        if (_exec_token.is_succeeded())
        {
            succeed();

            return;
        }
    }

    fail();
}

auto action::succeed() -> void
{
    _state = state::succeeded;
}

auto action::fail() -> void
{
    _state = state::failed;
}

auto action::on(state target_state, delegate<> delegate) -> void
{
    switch (target_state)
    {
        case state::succeeded:
        {
            _on_success = std::move(delegate);
            break;
        }
        case state::failed:
        {
            _on_failure = std::move(delegate);
            break;
        }
        default:
        {
            std::unreachable();
            break;
        }
    }
}
}
