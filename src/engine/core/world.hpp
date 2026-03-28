#pragma once

#include "engine/utils/event_bus.hpp"
#include "engine/utils/type_traits/type_id.hpp"
#include "engine/utils/type_traits/type_index.hpp"


namespace he
{

class system_base;

class world
{
private:
    struct systems_cache_entry final
    {
        type_index_t type_index;
        std::weak_ptr<system_base> system;
    };

public:
    world();

public:
    static auto instance() -> world&;

    template <std::derived_from<system_base> t>
    static auto get_system() -> t&;

    static auto events() -> event_bus&;

    static auto get_frame_number() -> uint16_t;

    auto tick() -> void;

private:
    auto rebuild_systems_cache() -> void;

    auto traverse_systems_tree(std::weak_ptr<system_base> node) -> void;

public:
    std::shared_ptr<system_base> _root_system;

    std::vector<systems_cache_entry> _systems_cache;

    he::event_bus _events;
};


template <std::derived_from<system_base> t>
auto world::get_system() -> t&
{
    return *(static_cast<t*>(std::ranges::find_if(
        instance()._systems_cache,
        [look_for = type_index<t>::value()] (const auto& entry)
        {
            return entry.type_index == look_for;
        })->system.lock().get()));
}

}
