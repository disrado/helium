#include "single.hpp"


namespace he
{

auto single::setup() -> void
{
    _action->on(
        state::succeeded,
        delegate{
            [this]
            {
                succeed();

                if (_context.has_value())
                {
                    _then_action->inherit_context(std::move(_context.value()));
                }

                _then_action->execute();

                _running_action = _then_action.get();
            }
        });

    _action->on(
        state::failed,
        delegate{
            [this]
            {
                fail();

                if (_context.has_value())
                {
                    _else_action->inherit_context(std::move(_context.value()));
                }

                _else_action->execute();

                _running_action = _else_action.get();
            } });
}

auto single::execute() -> void
{
    _action->execute();

    _running_action = _action.get();
}

auto single::abort() -> void
{
    if (_running_action)
    {
        _running_action->abort();
    }

    _action = nullptr;
    _then_action = nullptr;
    _else_action = nullptr;
    _running_action = nullptr;

    action_base::abort();
}

auto single::and_then(std::unique_ptr<action_base> next_action) -> void
{
    _then_action = std::move(next_action);
}

auto single::or_else(std::unique_ptr<action_base> next_action) -> void
{
    _else_action = std::move(next_action);
}

}
