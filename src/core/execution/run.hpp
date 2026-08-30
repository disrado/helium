#pragma once

#include <memory>

#include "defs.hpp"
#include "action/action.hpp"


namespace he
{

class run final
{
public:
    explicit run(exec::action_like auto target);

    auto execute() const -> void;
    auto abort() const -> void;

private:
    std::shared_ptr<exec::basic_action> _target;
};

run::run(exec::action_like auto target)
    : _target{ std::make_shared<decltype(target)>(std::move(target)) }
{
}

}
