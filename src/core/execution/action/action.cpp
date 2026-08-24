#include "action.hpp"


namespace he::exec
{

basic_action::basic_action(rdelegate<bool, const context&> definition, std::optional<context> initial_context)
    : _context{ std::move(initial_context) }
    , _definition{ std::move(definition) }
{
}


basic_action::~basic_action() noexcept
{
    // empty
}


auto basic_action::execute() -> void
{
    if (auto result{ _definition.try_execute(_context.value_or({})) }; result.has_value() && result.value())
    {
        succeed();

        return;
    }

    fail();
}


auto basic_action::abort() -> void
{
    _definition = {};

    _then_action = nullptr;
    _else_action = nullptr;

    set_state(state::aborted);
}


auto basic_action::get_state() const -> state
{
    return _state;
}


auto basic_action::on_success() -> void
{
    if (!_then_action)
    {
        return;
    }

    _then_action->_context = std::move(_context);

    _then_action->execute();
}


auto basic_action::on_failure() -> void
{
    if (!_else_action)
    {
        return;
    }

    _else_action->_context = std::move(_context);

    _else_action->execute();
}


auto basic_action::succeed() -> void
{
    set_state(state::succeeded);

    on_success();
}


auto basic_action::fail() -> void
{
    set_state(state::failed);

    on_failure();
}


auto basic_action::set_state(state new_state) -> void
{
    _state = new_state;

    if (const auto found{ _ons.find(_state) }; found != std::ranges::end(_ons))
    {
        std::ignore = found->second.execute();
    }
}


auto basic_action::store_and_then(std::unique_ptr<basic_action> next_action) -> void
{
    _then_action = std::move(next_action);
}


auto basic_action::store_or_else(std::unique_ptr<basic_action> next_action) -> void
{
    _else_action = std::move(next_action);
}

}
