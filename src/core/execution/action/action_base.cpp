#include "action_base.hpp"


namespace he::exec
{

basic_action::basic_action(delegate<bool(const context&)> definition)
    : _definition{
        [fn{ std::move(definition) }] (const context& ctx, std::stop_token) { return fn.try_execute(ctx).value_or(false); } }
{
}


basic_action::basic_action(delegate<bool(const context&, std::stop_token)> definition)
    : _definition{ std::move(definition) }
{
}


basic_action::~basic_action() noexcept
{
    // empty
}


auto basic_action::execute(task_graph::node& self, std::stop_token token) -> void
{
    if (auto result{ _definition.try_execute(self.context.value_or({}), std::move(token)) }; result.has_value() && result.value())
    {
        self.state = action_state::succeeded;

        return;
    }

    self.state = action_state::failed;
}


auto basic_action::store_and_then(std::unique_ptr<basic_action> next_action) -> void
{
    _then_action = std::move(next_action);
}


auto basic_action::store_or_else(std::unique_ptr<basic_action> next_action) -> void
{
    _else_action = std::move(next_action);
}


execution_token::execution_token(std::shared_ptr<basic_action> root, std::shared_ptr<task_graph> graph)
    : _root{ std::move(root) }
    , _graph{ std::move(graph) }
{
}


auto execution_token::cancel() const -> void
{
    _graph->cancel();
}

}
