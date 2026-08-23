#pragma once

#include "../action/action_base.hpp"


namespace he
{

class composite_base : public action_base
{
public:
    virtual auto setup() -> void = 0;

    virtual auto and_then(std::unique_ptr<action_base> next_action) -> void = 0;
    virtual auto or_else(std::unique_ptr<action_base> action) -> void = 0;
};

}
