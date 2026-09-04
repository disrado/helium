#include "action_base.hpp"


namespace he::exec
{

basic_action::basic_action(delegate<bool(const context&)> definition)
    : _definition{
        [fn{ std::move(definition) }] (const context& ctx, std::stop_token) { return fn.try_execute(ctx).value_or(false); }
    }
{
}


basic_action::basic_action(delegate<bool(const context&, std::stop_token)> definition)
    : _definition{ std::move(definition) }
{
}


auto basic_action::execute(task_node& self_node, std::stop_token token) -> void
{
    if (auto result{ _definition.try_execute(self_node.get_context().value_or({}), std::move(token)) }; result.has_value() && result.value())
    {
        self_node.state = action_state::succeeded;

        return;
    }

    self_node.state = action_state::failed;
}


auto basic_action::store_and_then(std::shared_ptr<basic_action> next_action) -> void
{
    _then_action = std::move(next_action);
}


auto basic_action::store_or_else(std::shared_ptr<basic_action> next_action) -> void
{
    _else_action = std::move(next_action);
}

}
