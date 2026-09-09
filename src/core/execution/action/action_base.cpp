#include "action_base.hpp"


namespace he::exec
{

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
