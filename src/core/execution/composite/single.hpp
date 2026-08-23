#pragma once

#include "composite_base.hpp"
#include "../utils.hpp"


namespace he
{

class single : public composite_base
{
public:
    explicit single(action_like auto target_action);

    auto setup() -> void override;

    auto execute() -> void override;
    auto abort() -> void override;

    auto and_then(std::unique_ptr<action_base> next_action) -> void override;
    auto or_else(std::unique_ptr<action_base> action) -> void override;

private:
    std::unique_ptr<action_base> _action;

    std::unique_ptr<action_base> _then_action;
    std::unique_ptr<action_base> _else_action;

    action_base* _running_action{ nullptr };
};

single::single(action_like auto target_action)
    : _action{ std::make_unique<decltype(target_action)>(std::move(target_action)) }
{
}

}
