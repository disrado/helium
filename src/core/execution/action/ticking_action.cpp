#include "ticking_action.hpp"

#include "core/execution/task/task_node.hpp"

#include <memory>


namespace he
{

auto ticking_action::tick(exec::task_node& self_node, std::stop_token token) -> result
{
    return _definition.try_execute(self_node.get_context(), std::move(token)).value_or(result::failed);
}


auto ticking_action::setup_node(exec::task_node& self_node) -> exec::task_node&
{
    self_node.request = exec::ticking_task_request{
        .definition{ exec::ticking_definition{
            [self{ std::static_pointer_cast<ticking_action>(shared_from_this()) }, &self_node] (std::stop_token token) -> exec::tick_result
            {
                return self->tick(self_node, std::move(token));
            }
        } },
        .on_complete{}
    };

    self_node.post_execution.bind(
        [&self_node] (exec::task_result result)
        {
            switch (result)
            {
                case exec::task_result::succeeded: self_node.state = exec::action_state::succeeded; break;
                case exec::task_result::failed:    self_node.state = exec::action_state::failed;    break;
                case exec::task_result::cancelled: self_node.state = exec::action_state::cancelled; break;
            }

            self_node.resolve_links();
        });

    return self_node;
}

}
