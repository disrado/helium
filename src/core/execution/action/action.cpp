#include "action.h"


namespace he
{

auto action::exec_token::succeed() -> void
{
    _succeeded = true;
}

auto action::exec_token::fail() -> void
{
    _succeeded = false;
}

auto action::exec_token::is_succeeded() const -> bool
{
    return _succeeded;
}

action::action(delegate<exec_token&, const context&> definition, std::optional<context> initial_context)
    : action_base{ std::move(initial_context) }
    , _definition{ std::move(definition) }
{
}

auto action::execute() -> void
{
    if (_definition.try_execute(_exec_token, _context.value_or({})))
    {
        if (_exec_token.is_succeeded())
        {
            succeed();

            return;
        }
    }

    fail();
}

auto action::abort() -> void
{
    action_base::abort();

    _definition = {};
    _exec_token = {};

    _on_success = {};
    _on_failure = {};
}

}
