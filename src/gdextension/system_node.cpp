#include "system_node.h"

#include <godot_cpp/variant/utility_functions.hpp>


namespace he
{

auto system_node::_bind_methods() -> void
{
}

auto system_node::_ready() -> void
{
    _root = std::make_shared<root_system>();
}

}
