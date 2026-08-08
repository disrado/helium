#include "systems/system_base.hpp"

#include "core/type_traits/type_index.hpp"


namespace he
{

auto system_base::get_subsystems() -> const std::map<type_index_t, std::shared_ptr<system_base>>&
{
    return _subsystems;
}

auto system_base::tick(double) -> void
{
    //
}

}
