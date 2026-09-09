#pragma once

#include "core/delegate/delegate.hpp"
#include "core/execution/defs.hpp"
#include "core/execution/task/task_graph.hpp"

#include <memory>
#include <vector>


namespace he::exec
{

struct graph_segment final
{
public:
    task_node& start;
    task_node& end;
};


class basic_action: public std::enable_shared_from_this<basic_action>
{
public:
    using state = action_state;
    using context = action_context;
    using result = task_result;

    struct link final
    {
        delegate<bool(state)> condition;
        std::shared_ptr<basic_action> next_action;
    };

public:
    basic_action() = default;

    basic_action(basic_action&&) noexcept = default;
    auto operator=(basic_action&&) noexcept -> basic_action& = default;

    virtual ~basic_action() noexcept = default;

    auto translate_into_graph(task_node& parent) -> graph_segment;

protected:
    virtual auto setup_node(task_node& self_node) -> task_node& = 0;
    auto add_link(delegate<bool(state)> condition, std::shared_ptr<basic_action> next_action) -> void;

private:
    auto translate_links(task_node& self_node, task_node& end_node) -> void;

protected:
    std::vector<link> _links;
};


template <typename t>
class action_base: public basic_action
{
public:
    auto and_then(action_like auto next) -> t&&;
    auto or_else(action_like auto next) -> t&&;
};


template <typename t>
auto action_base<t>::and_then(action_like auto next) -> t&&
{
    add_link(delegate{ [] (state s) { return s == state::succeeded; } }, std::make_shared<decltype(next)>(std::move(next)));

    return std::move(static_cast<t&>(*this));
}


template <typename t>
auto action_base<t>::or_else(action_like auto next) -> t&&
{
    add_link(delegate{ [] (state s) { return s == state::failed; } }, std::make_shared<decltype(next)>(std::move(next)));

    return std::move(static_cast<t&>(*this));
}

}
