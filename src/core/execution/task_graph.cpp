#include "task_graph.hpp"

#include "core/execution/scheduler.hpp"


namespace he::exec
{

task_graph::node::node(task_graph& graph, node* parent)
    : _graph{ graph }
    , _parent{ parent }
{
}


auto task_graph::node::add_child() -> node&
{
    return *_children.emplace_back(std::make_unique<node>(_graph, this));
}


auto task_graph::node::activate() -> void
{
    _graph.activate(*this);
}


auto task_graph::node::parent() const -> node*
{
    return _parent;
}


auto task_graph::node::children() const -> const std::vector<std::unique_ptr<node>>&
{
    return _children;
}


task_graph::task_graph()
    : _root{ *this }
{
}


auto task_graph::root() -> node&
{
    return _root;
}


auto task_graph::activate(node& target) -> void
{
    {
        const auto _{ std::lock_guard{ _mutex } };

        _stack.push_back(&target);
    }

    advance();
}


auto task_graph::advance() -> void
{
    auto lock{ std::unique_lock{ _mutex } };

    if (_running)
    {
        return;
    }

    _running = true;

    while (!_stack.empty())
    {
        auto* current{ _stack.back() };
        _stack.pop_back();

        // unlocked while running: may re-enter activate()/advance(), same thread or another
        lock.unlock();

        if (!current->pre_condition.try_execute().value_or(true))
        {
            lock.lock();

            continue;
        }

        if (!current->definition.is_bound())
        {
            std::ignore = current->post_condition.execute();

            lock.lock();

            continue;
        }

        scheduler::instance().post(
            task_request{
                .mode{ current->mode },
                .definition{ current->definition },
                .on_complete{
                    [self{ shared_from_this() }, current] (execution_status)
                    {
                        std::ignore = current->post_condition.execute();

                        self->advance();
                    } }
            });

        lock.lock();
    }

    _running = false;
}

}
