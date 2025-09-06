#pragma once

#include "core_macro.hpp"

#include <string_view>
#include <source_location>
#include <type_traits>


namespace details
{
constexpr auto parse_type_name(std::string_view signature, std::string_view begin_pattern, std::string_view end_pattern) -> std::string_view
{
    signature.remove_prefix(signature.find(begin_pattern) + begin_pattern.length());
    signature.remove_suffix(signature.length() - signature.find(end_pattern));

    return signature;
}
}

namespace he
{
///
/// Returns name of the given type
/// The way compilers define function signatures are vastly different and you should never rely on their equality
///
template<typename T>
constexpr auto type_name(std::type_identity<T> = {}) -> std::string_view;

#if GCC_COMPILER

// gcc (15.2) function signature format:
// std::string_view he::type_name(std::type_identity<_Tp>) [with T = int; std::string_view = std::basic_string_view<char>]

template<typename T>
constexpr auto type_name(std::type_identity<T>) -> std::string_view
{
    return details::parse_type_name(std::source_location::current().function_name(), "[with T = ", ";");
}

#elif CLANG_COMPILER

// clang (21.1.0) function signature format:
// std::string_view he::type_name(std::type_identity<T>) [T = int]

template<typename T>
constexpr auto type_name(std::type_identity<T>) -> std::string_view
{
    return details::parse_type_name(std::source_location::current().function_name(), "[T = ", "]");
}

#elif MSVC_COMPILER

// msvc (17.13) function signature format:
// class std::basic_string_view<char,struct std::char_traits<char> > __cdecl he::type_name<int>(struct std::type_identity<int>)

template<typename T>
constexpr auto type_name(std::type_identity<T>) -> std::string_view
{
    return details::parse_type_name(std::source_location::current().function_name(), "he::type_name<", ">(struct std::type_identity<");
}

#elif

template<typename T>
constexpr auto type_name(std::type_identity<T>) -> std::string_view
{
    static_assert(false, "compiler is not supported")

    return {};
}

#endif
}
