#pragma once

#include "core/execution/action/action_base.hpp"


namespace he
{

class async_action: public exec::action_base<async_action>
{
public:
    using state = exec::basic_action::state;
    using context = exec::basic_action::context;
    using action_base::action_base;

    auto translate_into_graph(exec::task_node& parent) -> exec::graph_segment override;
};

}
