#include "action.hpp"


namespace he
{

auto action::setup_node(exec::task_node& self_node) -> exec::task_node&
{
    self_node.mode = exec::launch_policy::sync;
    self_node.definition = exec::task_definition{
        [self{ shared_from_this() }, &self_node] (std::stop_token token) { self->execute(self_node, std::move(token)); }
    };

    self_node.post_execution.bind(
        [&self_node] (exec::execution_status)
        {
            if (self_node.cancel_requested)
            {
                self_node.state = exec::action_state::cancelled;

                return;
            }

            resolve_link(self_node);
        });

    return self_node;
}


auto action::resolve_link(exec::task_node& self_node) -> void
{
    for (auto& entry : self_node.links)
    {
        if (entry.condition.try_execute(self_node.state).value_or(false))
        {
            entry.target->set_context(self_node.get_context());
            entry.target->activate();

            break;
        }
    }
}

}
