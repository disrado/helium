#pragma once

#include <cstdint>


namespace he
{
using index_type = uint32_t;

namespace internal
{
struct sequential_index final
{
	[[nodiscard]] static constexpr auto value() noexcept -> index_type
	{
		static auto index{ index_type{ 0 } };
		return index++;
	}
};
}

template <typename T>
struct type_index final
{
	[[nodiscard]] static constexpr auto value() noexcept -> index_type
	{
		static auto id{ internal::sequential_index::value() };
		return id;
	}
};
}
