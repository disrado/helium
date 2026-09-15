#include "systems/system_tree.hpp"


namespace he
{

auto system_tree::create() -> std::unique_ptr<system_tree>
{
    return std::make_unique<system_tree>();
}

auto system_tree::tick(double) -> void
{
    // stab
}

}
