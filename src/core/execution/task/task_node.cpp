#include "task_node.hpp"

#include "core/execution/task/task_graph.hpp"


namespace he::exec
{

task_node::task_node(task_graph& graph, task_node* parent)
    : _graph{ graph }
    , _parent{ parent }
{
}


auto task_node::resolve_links() -> void
{
    const auto current_state{ state.load() };

    for (auto& entry : _links)
    {
        if (entry.condition.try_execute(current_state).value_or(false))
        {
            entry.target->set_context(get_context());
            entry.target->activate();

            return;
        }
    }
}


auto task_node::add_child() -> task_node&
{
    return *_children.emplace_back(std::make_unique<task_node>(_graph, this));
}


auto task_node::activate() -> void
{
    _graph.activate(*this);
}


auto task_node::get_parent() const -> task_node*
{
    return _parent;
}


auto task_node::get_children() const -> const std::vector<std::unique_ptr<task_node>>&
{
    return _children;
}


auto task_node::get_context() const -> const action_context&
{
    return _context;
}


auto task_node::set_context(action_context new_context) -> void
{
    _context = std::move(new_context);
}


auto task_node::merge_context(action_context source) -> void
{
    for (auto& [key, value]: source)
    {
        _context[key] = std::move(value);
    }
}


auto task_node::add_link(link entry) -> void
{
    _links.push_back(std::move(entry));
}


auto task_node::get_links() const -> const std::vector<link>&
{
    return _links;
}

}
