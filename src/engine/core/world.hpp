#pragma once

#include <cassert>
#include <map>

#include "engine/core/system.hpp"
#include "engine/utils/type_traits/type_id.hpp"
#include "engine/utils/type_traits/type_index.hpp"
#include "game/systems/log_system.hpp"


namespace he
{

class world
{
public:
    world();

public:
    static auto instance() -> world&;

    template <typename T> requires std::derived_from<T, system_base>
    auto get_system() -> T&;

private:
    std::map<type_index_t, system_base> _systems;
};


template <typename T> requires std::derived_from<T, system_base>
auto world::get_system() -> T&
{
    return {};
}


}
