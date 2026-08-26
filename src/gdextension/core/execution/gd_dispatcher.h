#pragma once

#include "core/execution/dispatcher.hpp"


namespace he
{

class gd_dispatcher final: public exec::dispatcher
{
public:
    auto dispatch(std::function<void()> work) -> void override;
};

}
