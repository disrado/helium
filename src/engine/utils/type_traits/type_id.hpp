#pragma once

#include "engine/utils/type_traits/type_index.hpp"
#include "engine/utils/type_traits/type_name.hpp"

#include <utility>


namespace he
{
struct type_info final
{
    template <typename T>
    constexpr type_info(std::in_place_type_t<T>) noexcept
        : _type_index{ type_index<T>::value() }
        , _type_name{ type_name<T>::value() }
    {
    }

    [[nodiscard]] constexpr auto index() const noexcept -> type_index_t
    {
        return _type_index;
    }

    [[nodiscard]] constexpr auto name() const noexcept -> std::string_view
    {
        return _type_name;
    }

    auto operator<=>(const type_info& other) const
    {
        return _type_index <=> other._type_index;
    }

    bool operator==(const type_info& other) const
    {
        return (*this <=> other) == 0;
    }

private:
    type_index_t _type_index;
    std::string_view _type_name;
};

template <typename t>
[[nodiscard]] constexpr auto type_id() noexcept -> const type_info&
{
    static const auto info{ type_info{ std::in_place_type<t> } };
    return info;
}

template <typename t>
[[nodiscard]] constexpr auto type_of(t&&) noexcept -> const type_info&
{
    return type_id<t>();
}
}
