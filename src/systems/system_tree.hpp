#pragma once

#include "core/singleton.hpp"
#include "core/type_traits/type_index.hpp"

#include <map>
#include <memory>

namespace he
{
class system_base;
}


class system_tree final: he::singleton<system_tree>
{

private:
    std::map<he::type_index_t, std::shared_ptr<he::system_base>> _systems;
};


struct system_instantiated final
{
    he::type_index_t type_index;
    std::weak_ptr<he::system_base> instance;
};

struct system_destroyed final
{
    he::type_index_t type_index;
};