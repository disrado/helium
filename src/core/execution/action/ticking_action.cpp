#include "ticking_action.hpp"

#include "core/execution/task_node.hpp"


namespace he
{

auto ticking_action::execute(exec::task_node& self_node, std::stop_token token) -> void
{
    const auto result{ _definition.try_execute(self_node.get_context(), std::move(token)).value_or(result::failed) };

    switch (result)
    {
        case result::running:
        {
            self_node.state = exec::action_state::running;
            break;
        }
        case result::succeeded:
        {
            self_node.state = exec::action_state::succeeded;
            break;
        }
        case result::failed:
        {
            self_node.state = exec::action_state::failed;
            break;
        }
    }
}


auto ticking_action::setup_node(exec::task_node& self_node) -> exec::task_node&
{
    self_node.mode = exec::launch_policy::tick;
    self_node.definition = exec::task_definition{
        [self{ shared_from_this() }, &self_node] (std::stop_token token) -> exec::execution_status
        {
            self->execute(self_node, std::move(token));

            switch (self_node.state)
            {
                case exec::action_state::running: return exec::execution_status::running;
                case exec::action_state::succeeded: return exec::execution_status::completed;
                case exec::action_state::cancelled: return exec::execution_status::cancelled;
                default: return exec::execution_status::faulted;
            }
        }
    };

    self_node.post_execution.bind(
        [&self_node] (exec::execution_status)
        {
            if (self_node.cancel_requested)
            {
                self_node.state = exec::action_state::cancelled;

                return;
            }

            self_node.resolve_links();
        });

    return self_node;
}

}
