#include "systems/system_tree.hpp"

#include <ranges>


namespace he
{

auto system_tree::create() -> std::unique_ptr<system_tree>
{
    return std::make_unique<system_tree>();
}


auto system_tree::tick(double dt) -> void
{
    for (const auto& child : std::views::values(_systems))
    {
        child->tick(dt);
    }
}

}
