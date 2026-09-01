#include "action_base.hpp"


namespace he::exec
{

basic_action::basic_action(delegate<bool(const context&)> definition, std::optional<context> initial_context)
    : _context{ std::move(initial_context) }
    , _definition{
        [fn{ std::move(definition) }] (const context& ctx, std::stop_token) { return fn.try_execute(ctx).value_or(false); } }
{
}


basic_action::basic_action(delegate<bool(const context&, std::stop_token)> definition, std::optional<context> initial_context)
    : _context{ std::move(initial_context) }
    , _definition{ std::move(definition) }
{
}


basic_action::~basic_action() noexcept
{
    // empty
}


auto basic_action::execute(std::stop_token token) -> void
{
    if (auto result{ _definition.try_execute(_context.value_or({}), std::move(token)) }; result.has_value() && result.value())
    {
        succeed();

        return;
    }

    fail();
}


auto basic_action::translate_into_graph(task_graph::node& parent) -> graph_segment
{
    auto segment{ expand_on_graph(parent) };

    if (auto self{ weak_from_this().lock() })
    {
        segment.begin.anchor = std::move(self);
    }

    return segment;
}


auto basic_action::set_context(std::optional<context> new_context) -> void
{
    _context = std::move(new_context);
}


auto basic_action::get_context() const -> const std::optional<context>&
{
    return _context;
}


auto basic_action::get_state() const -> state
{
    return _state;
}


auto basic_action::succeed() -> void
{
    set_state(state::succeeded);
}


auto basic_action::fail() -> void
{
    set_state(state::failed);
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
