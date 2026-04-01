#include "core/world.hpp"

#include "systems/root_system.hpp"
#include "systems/logging.hpp"
#include "utils/type_traits/type_index.hpp"


namespace he
{
world::world()
    : _root_system{ std::make_shared<root_system>() }
{
    _events.on<system_instantiated>([this] (const auto& event) { rebuild_systems_cache(); });
    _events.on<system_destroyed>([this] (const auto&) { rebuild_systems_cache(); });

    _systems_cache.emplace_back(
        systems_cache_entry{
            .type_index = type_index<root_system>::value(),
            .system = std::weak_ptr{ _root_system }
        });
}

auto world::instance() -> world&
{
    static world instance;

    return instance;
}

auto world::events() -> event_bus&
{
    return instance()._events;
}

auto world::rebuild_systems_cache() -> void
{
    _systems_cache.clear();

    _systems_cache.emplace_back(
        systems_cache_entry{
            .type_index = type_index<root_system>::value(),
            .system = std::weak_ptr{ _root_system }
        });

    traverse_systems_tree(_root_system);
}

auto world::traverse_systems_tree(std::weak_ptr<system_base> node) -> void
{
    const auto locked{ node.lock() };

    if (!locked)
    {
        return;
    }

    for (const auto& [type_index, subsystem] : locked->get_subsystems())
    {
        traverse_systems_tree(subsystem);

        _systems_cache.emplace_back(systems_cache_entry{
            .type_index = type_index,
            .system = std::weak_ptr{ subsystem }
        });
    }
}

auto world::get_frame_number() -> uint16_t
{
    return 954;
}

auto world::tick() -> void
{
    for (const auto& [type_index, system] : _systems_cache)
    {
        system.lock()->tick(.0f);
    }
}

}
