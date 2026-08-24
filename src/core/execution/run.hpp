#pragma once

#include <memory>

#include "utils.hpp"
#include "action/action.hpp"


namespace he
{

class run final
{
public:
    run(exec::action_like auto target);

    auto execute() const -> void;
    auto abort() const -> void;

private:
    std::unique_ptr<exec::basic_action> _target;
};

run::run(exec::action_like auto target)
    : _target{ std::make_unique<decltype(target)>(std::move(target)) }
{
}

}
