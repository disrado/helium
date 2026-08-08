#pragma once

#include "core/type_traits/type_index.hpp"
#include "core/event_bus.hpp"

#include <map>
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

    auto get_subsystems() -> const std::map<type_index_t, std::shared_ptr<system_base>>&;

public:
    virtual auto tick(double dt) -> void;

private:
    std::map<type_index_t, std::shared_ptr<system_base>> _subsystems;
};

struct system_instantiated final
{
    type_index_t type_index;
    std::weak_ptr<system_base> instance;
};

struct system_destroyed final
{
    type_index_t type_index;
};

template <std::derived_from<system_base> system_t>
auto system_base::add_child(auto&&... args) -> system_t&
{
    auto system{ std::make_shared<system_t>(std::forward<decltype(args)>(args)...) };

    return *system;
}

template <std::derived_from<system_base> system_t>
auto system_base::remove_child() -> bool
{
    return false;
}
}
