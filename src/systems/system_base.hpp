#pragma once

#include "core/event_bus.hpp"

#include <memory>


namespace he
{

class system_base: public std::enable_shared_from_this<system_base>
{
public:
    virtual ~system_base() = default;

    template <std::derived_from<system_base> system_t>
    auto add_child(auto&&... args) -> system_t&;

    template <std::derived_from<system_base> system_t>
    auto remove_child() -> bool;

public:
    virtual auto tick(double dt) -> void;
};

template <std::derived_from<system_base> system_t>
auto system_base::add_child(auto&&... _) -> system_t&
{
    // stab
    return nullptr;
}

template <std::derived_from<system_base> system_t>
auto system_base::remove_child() -> bool
{
    // stab
    return false;
}
}
