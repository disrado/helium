#include "task_node.hpp"

#include "core/execution/task_graph.hpp"


namespace he::exec
{

task_node::task_node(task_graph& graph, task_node* parent)
    : _graph{ graph }
    , _parent{ parent }
{
}


auto task_node::add_child() -> task_node&
{
    return *_children.emplace_back(std::make_unique<task_node>(_graph, this));
}


auto task_node::activate() -> void
{
    _graph.activate(*this);
}


auto task_node::parent() const -> task_node*
{
    return _parent;
}


auto task_node::children() const -> const std::vector<std::unique_ptr<task_node>>&
{
    return _children;
}


auto task_node::get_context() const -> const std::optional<action_context>&
{
    return _context;
}


auto task_node::set_context(std::optional<action_context> new_context) -> void
{
    _context = std::move(new_context);
}


auto task_node::merge_context(std::optional<action_context> new_entries) -> void
{
    if (!new_entries.has_value())
    {
        return;
    }

    if (!_context.has_value())
    {
        _context = std::move(new_entries);

        return;
    }

    for (auto& [key, value]: new_entries.value())
    {
        _context.value()[key] = std::move(value);
    }
}

}
