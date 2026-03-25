#pragma once

#include <cstdint>


namespace he
{
using type_index_t = uint32_t;

namespace internal
{
struct sequential_index final
{
    [[nodiscard]] static constexpr auto value() noexcept -> type_index_t
    {
        static auto index{ type_index_t{ 0 } };
        return index++;
    }
};
}

template <typename T>
struct type_index final
{
    [[nodiscard]] static constexpr auto value() noexcept -> type_index_t
    {
        static auto id{ internal::sequential_index::value() };
        return id;
    }
};
}
