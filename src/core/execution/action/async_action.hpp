#pragma once

#include "core/execution/action/action_base.hpp"

#include <optional>


namespace he
{

class async_action: public exec::action_base<async_action>
{
public:
    using state = exec::basic_action::state;
    using context = exec::basic_action::context;
    using action_base::action_base;

    auto cancel() -> void override;

protected:
    auto expand_on_graph(exec::task_graph::node& parent) -> exec::graph_segment override;

private:
    std::optional<exec::graph_segment> _graph_segment;
};

}
