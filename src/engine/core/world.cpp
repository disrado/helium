#include "engine/core/world.hpp"

#include <memory>
#include <catch2/internal/catch_unique_ptr.hpp>


namespace he
{
world::world()
{
    
}


auto world::instance() -> world&
{
    static world instance;

    return instance;
}

}
