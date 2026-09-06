#pragma once

#include "core/execution/action/action_base.hpp"


namespace he
{

class action: public exec::action_base<action>
{
public:
    using state = exec::basic_action::state;
    using context = exec::basic_action::context;
    using action_base::action_base;

protected:
    auto setup_node(exec::task_node& self_node) -> exec::task_node& override;
};

}
