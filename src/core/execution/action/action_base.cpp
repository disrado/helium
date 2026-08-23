#include "action_base.hpp"


namespace he
{
action_base::action_base(std::optional<context> initial_context)
    : _context{ std::move(initial_context) }
{
}

action_base::~action_base() noexcept
{
    // empty
}

auto action_base::get_state() const -> state
{
    return _state;
}

auto action_base::abort() -> void
{
    _state = state::aborted;

    std::ignore = _on_abort.execute();
}

auto action_base::succeed() -> void
{
    _state = state::succeeded;

    std::ignore = _on_success.execute();
}

auto action_base::fail() -> void
{
    _state = state::failed;

    std::ignore = _on_failure.execute();
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

}
