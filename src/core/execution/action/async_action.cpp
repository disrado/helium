#include "async_action.hpp"

#include <memory>


namespace he
{

async_action::async_action(delegate<bool(const context&)> definition)
    : _definition{
        [fn{ std::move(definition) }] (const context& ctx, std::stop_token) { return fn.try_execute(ctx).value_or(false); }
    }
{
}


async_action::async_action(delegate<bool(const context&, std::stop_token)> definition)
    : _definition{ std::move(definition) }
{
}


auto async_action::execute(exec::task_node& self_node, std::stop_token token) -> result
{
    const auto outcome{ _definition.try_execute(self_node.get_context(), std::move(token)) };

    return (outcome.has_value() && outcome.value()) ? result::succeeded : result::failed;
}


auto async_action::setup_node(exec::task_node& self_node) -> exec::task_node&
{
    self_node.request = exec::async_task_request{
        .definition{ exec::task_definition{
            [self{ std::static_pointer_cast<async_action>(shared_from_this()) }, &self_node] (std::stop_token token) -> exec::task_result
            {
                return self->execute(self_node, std::move(token));
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
