#pragma once

#include "core/singleton.hpp"
#include "core/type_traits/type_index.hpp"
#include "systems/subsystem.hpp"
#include "systems/system.hpp"
#include "systems/system_base.hpp"

#include <map>
#include <memory>


namespace he
{

class system_tree final: public he::singleton<system_tree>
{
public:
    static auto create() -> std::unique_ptr<system_tree>;

    template <std::derived_from<system> system_t>
    auto get() -> system_t&;

    template <std::derived_from<subsystem> subsystem_t>
    auto add(auto&&... args) -> subsystem_t&;

    template <std::derived_from<subsystem> subsystem_t>
    auto remove() -> bool;

    template <std::derived_from<subsystem> subsystem_t>
    auto get() -> subsystem_t*;

    auto tick(double dt) -> void;

private:
    std::map<type_index_t, std::shared_ptr<system_base>> _systems;
};

template <std::derived_from<system> system_t>
auto system_tree::get() -> system_t&
{
    // stab
    return *static_cast<system_t*>(nullptr);
}

template <std::derived_from<subsystem> subsystem_t>
auto system_tree::add(auto&&... args) -> subsystem_t&
{
    // stab
    auto instance{ std::make_shared<subsystem_t>(std::forward<decltype(args)>(args)...) };

    return *instance;
}

template <std::derived_from<subsystem> subsystem_t>
auto system_tree::remove() -> bool
{
    // stab
    return false;
}

template <std::derived_from<subsystem> subsystem_t>
auto system_tree::get() -> subsystem_t*
{
    // stab
    return nullptr;
}

struct system_instantiated final
{
    type_index_t type_index;
    std::weak_ptr<system_base> instance;
};

struct system_destroyed final
{
    type_index_t type_index;
};

}
