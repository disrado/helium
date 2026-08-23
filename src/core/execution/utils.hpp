#pragma once

#include <type_traits>

namespace he
{
class action_base;
class composite_base;
}


namespace he
{

template <typename t>
concept action_like = std::is_base_of_v<action_base, t>;

template <typename t>
concept run_like = std::is_base_of_v<composite_base, t>;
}
