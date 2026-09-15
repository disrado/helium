#pragma once

#include "systems/system_tree.hpp"

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/wrapped.hpp>


namespace he
{

class module_node: public godot::Node
{
    GDCLASS(module_node, godot::Node)

public:
    static auto _bind_methods() -> void;

    auto _ready() -> void override;
};

}
