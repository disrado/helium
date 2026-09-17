#pragma once

#include "core/event_bus.hpp"

#include <memory>


namespace he
{

class system_base: public std::enable_shared_from_this<system_base>
{
public:
    virtual ~system_base() = default;

    virtual auto tick(double dt) -> void;
};

}
