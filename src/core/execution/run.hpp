#pragma once

#include <memory>

#include "composite/composite_base.hpp"
#include "utils.hpp"


namespace he
{

class run final
{
public:
    run(run_like auto target);

    auto and_then(run_like auto next_action) -> run&&;

    auto or_else(run_like auto next_action) -> run&&;

    auto execute() const -> void;
    auto abort() const -> void;

private:
    std::unique_ptr<composite_base> _target;
};


run::run(run_like auto target)
    : _target{ std::make_unique<decltype(target)>(std::move(target)) }
{
    _target->setup();
}

auto run::and_then(run_like auto next_action) -> run&&
{
    _target->and_then(std::make_unique<decltype(next_action)>(std::move(next_action)));

    return std::move(*this);
}

auto run::or_else(run_like auto next_action) -> run&&
{
    _target->or_else(std::make_unique<decltype(next_action)>(std::move(next_action)));

    return std::move(*this);
}

}
