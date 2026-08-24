#pragma once

#include <type_traits>

namespace he::exec
{
class basic_action;
}


namespace he::exec
{

template <typename t>
concept action_like = std::is_base_of_v<basic_action, t>;

}
