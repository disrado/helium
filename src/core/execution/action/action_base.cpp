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


auto basic_action::translate_into_graph(task_node& parent) -> graph_segment
{
    auto& self_node{ parent.add_child() };

    auto& end_node{ setup_node(self_node) };

    translate_links(self_node, end_node);

    return graph_segment{ .start{ self_node }, .end{ end_node } };
}


auto basic_action::add_link(delegate<bool(state)> condition, std::shared_ptr<basic_action> next_action) -> void
{
    _links.push_back({ .condition{ std::move(condition) }, .next_action{ std::move(next_action) } });
}


auto basic_action::translate_links(task_node& self_node, task_node& end_node) -> void
{
    for (auto& entry : _links)
    {
        end_node.add_link({ .condition{ entry.condition }, .target{ &entry.next_action->translate_into_graph(self_node).start } });
    }
}

}
