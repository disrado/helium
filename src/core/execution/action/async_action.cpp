#include "async_action.hpp"


namespace he
{

auto async_action::setup_node(exec::task_node& self_node) -> exec::task_node&
{
    self_node.mode = exec::launch_policy::async;
    self_node.definition = exec::task_definition{
        [self{ shared_from_this() }, &self_node] (std::stop_token token) -> exec::execution_status
        {
            self->execute(self_node, std::move(token));

            switch (self_node.state)
            {
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
