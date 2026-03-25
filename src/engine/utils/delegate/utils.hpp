#pragma once

#include <type_traits>


namespace he::delegates
{
template <typename callable_t, typename... arg_ts>
concept bindable = std::is_invocable_v<callable_t, arg_ts...>;

template <typename callable_t, typename... arg_ts>
concept lifetime_bound_bindable = std::is_member_function_pointer_v<callable_t> || bindable<callable_t, arg_ts...>;
}
