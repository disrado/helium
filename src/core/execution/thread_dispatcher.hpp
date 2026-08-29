#pragma once

#include "core/execution/dispatcher.hpp"


namespace he::exec
{

class thread_dispatcher final: public dispatcher
{
public:
    auto dispatch(std::function<void()> work) -> void override;
};

}
