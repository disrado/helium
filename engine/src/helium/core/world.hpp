#pragma once

#include "utils/event_bus.hpp"
#include "utils/type_traits/type_index.hpp"

#include <memory>
#include <vector>

#include "utils/singleton.hpp"


namespace he
{
class root_system;
class system_base;

class world: public singleton<world>
{
private:
    struct systems_cache_entry final
    {
        type_index_t type_index;
        std::weak_ptr<system_base> system;
    };

    struct system_tree_entry final
    {
        // std::map<type_index_t, std::un>
    };

public:
    world();

public:
    template <std::derived_from<system_base> system_t>
    auto get() -> system_t&;

    template <std::derived_from<system_base> system_t>
    auto has() const -> bool;

    auto get_frame_number() -> uint16_t;

    auto tick() -> void;

private:
    auto rebuild_systems_cache() -> void;

    auto traverse_systems_tree(std::weak_ptr<system_base> node) -> void;

public:
    inline static he::event_bus events;

private:
    std::shared_ptr<system_base> _root_system;

    std::vector<systems_cache_entry> _systems_cache;
};


template <std::derived_from<system_base> system_t>
auto world::get() -> system_t&
{
    return *(static_cast<system_t*>(std::ranges::find_if(
        _systems_cache,
        [look_for = type_index<system_t>()] (const auto& entry)
        {
            return entry.type_index == look_for;
        })->system.lock().get()));
}

template <std::derived_from<system_base> system_t>
auto world::has() const -> bool
{
    return std::ranges::any_of(
        _systems_cache,
        [look_for = type_index<system_t>()] (const auto& entry)
        {
            return entry.type_index == look_for;
        });
}

}
