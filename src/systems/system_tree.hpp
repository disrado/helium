#pragma once

#include "core/containers/ordered_tree.hpp"
#include "core/singleton.hpp"
#include "core/type_traits/type_index.hpp"
#include "systems/subsystem.hpp"
#include "systems/system.hpp"
#include "systems/system_base.hpp"

#include <cassert>
#include <memory>


namespace he
{

class system_tree final: public he::singleton<system_tree>
{
public:
    using root = system_tree;

public:
    static auto create() -> std::unique_ptr<system_tree>;

    template <std::derived_from<system> system_t>
    static auto get() -> system_t&;

    template <std::derived_from<subsystem> subsystem_t>
    static auto try_get() -> subsystem_t*;

    template <std::derived_from<subsystem> subsystem_t>
    static auto get() -> subsystem_t&;

    template <typename parent_t, std::derived_from<subsystem> subsystem_t>
    static auto add(auto&&... args) -> subsystem_t&;

    template <std::derived_from<subsystem> subsystem_t>
    static auto remove() -> bool;

    template <std::derived_from<subsystem> subsystem_t>
    static auto contains() -> bool;

    auto tick(double dt) -> void;

private:
    ordered_tree<type_index_t, std::shared_ptr<system_base>> _systems{ type_index<system_tree>(), std::make_shared<system_base>() };
};


template <std::derived_from<system> system_t>
auto system_tree::get() -> system_t&
{
    auto* found{ instance()._systems.find(type_index<system_t>()) };
    assert(found && "system not registered");

    return *static_cast<system_t*>(found->get());
}


template <std::derived_from<subsystem> subsystem_t>
auto system_tree::try_get() -> subsystem_t*
{
    auto* found{ instance()._systems.find(type_index<subsystem_t>()) };

    return found ? static_cast<subsystem_t*>(found->get()) : nullptr;
}


template <std::derived_from<subsystem> subsystem_t>
auto system_tree::get() -> subsystem_t&
{
    auto* found{ instance()._systems.find(type_index<subsystem_t>()) };
    assert(found && "subsystem not registered");

    return *static_cast<subsystem_t*>(found->get());
}


template <typename parent_t, std::derived_from<subsystem> subsystem_t>
auto system_tree::add(auto&&... args) -> subsystem_t&
{
    assert(instance()._systems.contains(type_index<parent_t>()) && "parent not registered");

    auto new_instance{ std::make_shared<subsystem_t>(std::forward<decltype(args)>(args)...) };

    instance()._systems.emplace(type_index<parent_t>(), type_index<subsystem_t>(), std::shared_ptr<system_base>{ new_instance });

    return *new_instance;
}


template <std::derived_from<subsystem> subsystem_t>
auto system_tree::remove() -> bool
{
    return instance()._systems.erase(type_index<subsystem_t>());
}


template <std::derived_from<subsystem> subsystem_t>
auto system_tree::contains() -> bool
{
    return instance()._systems.contains(type_index<subsystem_t>());
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
