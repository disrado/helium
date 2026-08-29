#pragma once

#include <functional>


namespace he::exec
{

class dispatcher
{
public:
    virtual ~dispatcher() noexcept = default;

    virtual auto dispatch(std::function<void()> work) -> void = 0;
};

}
