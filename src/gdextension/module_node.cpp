#include "module_node.h"

#include <godot_cpp/variant/utility_functions.hpp>


namespace he
{

auto module_node::_bind_methods() -> void
{
}

auto module_node::_ready() -> void
{
    _root = std::make_shared<root_system>();
}

}
