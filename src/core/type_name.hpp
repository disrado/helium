#pragma once

#include <source_location>
#include <string_view>

#include "core_macro.hpp"
#include "type_id.hpp"


namespace he
{
namespace internal
{
#if GCC_COMPILER

    // gcc (15.2) function signature format:
    // constexpr std::string_view internal::parse_name_from_signature() [with T = int; std::string_view = std::basic_string_view<char>]

    constexpr const auto prefix{ std::string_view{ "= " } };
    constexpr const auto suffix{ std::string_view{ ";" } };

#elif CLANG_COMPILER

    // clang (21.1.0) function signature format:
    // std::string_view internal::parse_name_from_signature() [T = int]

    constexpr const auto prefix{ std::string_view { "= " } };
    constexpr const auto suffix{ std::string_view { "]" } };

#elif MSVC_COMPILER

    // msvc (v19.43 VS17.13) function signature format:
    // class std::basic_string_view<char,struct std::char_traits<char> > __cdecl internal::parse_name_from_signature<int>(void) noexcept

    constexpr const auto prefix{ std::string_view{ "e<" } };
    constexpr const auto suffix{ std::string_view{ ">" } };

#endif

template <typename T>
[[nodiscard]] constexpr auto parse_name_from_signature() noexcept -> std::string_view
{
    auto signature{ std::string_view{ std::source_location::current().function_name() } };

    signature.remove_prefix(signature.find(prefix) + prefix.length());
    signature.remove_suffix(signature.length() - signature.find(suffix));

    return signature;
}

template<typename T>
[[nodiscard]] constexpr auto get_name_of() noexcept -> std::string_view
{
    static const auto name{ internal::parse_name_from_signature<T>() };
    return name;
}
}

///
/// Provides a name of the given type
/// The way how compilers define function signatures are vastly different and you should not rely on it
///
template<typename T>
struct type_name final
{
    [[nodiscard]] static constexpr auto value() noexcept -> std::string_view
    {
        return internal::get_name_of<T>();
    }
};
}
