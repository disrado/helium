#include "action.hpp"


namespace he
{

auto action_base::exec_token::succeed() -> void
{
    _succeeded = true;
}


auto action_base::exec_token::fail() -> void
{
    _succeeded = false;
}


auto action_base::exec_token::is_succeeded() const -> bool
{
    return _succeeded;
}


action_base::action_base(delegate<exec_token&, const context&> definition, std::optional<context> initial_context)
    : _context{ std::move(initial_context) }
    , _definition{ std::move(definition) }
{
}


action_base::~action_base() noexcept
{
    // empty
}


auto action_base::execute() -> void
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


auto action_base::abort() -> void
{
    _state = state::aborted;

    _definition = {};
    _exec_token = {};

    _on_success = {};
    _on_failure = {};

    _then_action = nullptr;
    _else_action = nullptr;

    std::ignore = _on_abort.execute();
}


auto action_base::get_state() const -> state
{
    return _state;
}


auto action_base::on([[maybe_unused]] state target_state, [[maybe_unused]] delegate<> delegate) -> action_base&
{
    switch (target_state)
    {
        case state::succeeded:
        {
            _on_success.bind(std::move(delegate));
            break;
        }
        case state::failed:
        {
            _on_failure.bind(std::move(delegate));
            break;
        }
        case state::aborted:
        {
            _on_abort.bind(std::move(delegate));
            break;
        }
        default:
        {
            std::unreachable();
            break;
        }
    }

    return *this;
}


auto action_base::inherit_context(context&& target_context) -> void
{
    if (!_context.has_value())
    {
        _context = std::move(target_context);
    }
}


auto action_base::on_success() -> void
{
    if (!_then_action)
    {
        return;
    }

    if (_context.has_value())
    {
        _then_action->inherit_context(std::move(_context.value()));
    }

    _then_action->execute();
}


auto action_base::on_failure() -> void
{
    if (!_else_action)
    {
        return;
    }

    if (_context.has_value())
    {
        _else_action->inherit_context(std::move(_context.value()));
    }

    _else_action->execute();
}



auto action_base::succeed() -> void
{
    _state = state::succeeded;

    on_success();

    std::ignore = _on_success.execute();
}


auto action_base::fail() -> void
{
    _state = state::failed;

    on_failure();

    std::ignore = _on_failure.execute();
}


auto action_base::setup() -> void
{
    // empty
}


auto action_base::store_and_then(std::unique_ptr<action_base> next_action) -> void
{
    _then_action = std::move(next_action);
}


auto action_base::store_or_else(std::unique_ptr<action_base> next_action) -> void
{
    _else_action = std::move(next_action);
}

}
