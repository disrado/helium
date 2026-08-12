#pragma once

#include "systems/root_system.hpp"

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/wrapped.hpp>

#include <memory>


namespace he
{

class system_node: public godot::Node
{
    GDCLASS(system_node, godot::Node)

public:
    static auto _bind_methods() -> void;

    auto _ready() -> void override;

private:
    std::shared_ptr<root_system> _root;
};

}
