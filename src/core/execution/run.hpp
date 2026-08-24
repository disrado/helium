#pragma once

#include <memory>

#include "utils.hpp"
#include "action/action.hpp"


namespace he
{

// small helper to kick-off action chain by calling setup() on root
class run final
{
public:
    run(action_like auto target);

    auto execute() const -> void;
    auto abort() const -> void;

private:
    std::unique_ptr<action_base> _target;
};

run::run(action_like auto target)
    : _target{ std::make_unique<decltype(target)>(std::move(target)) }
{
    _target->setup();
}

}
