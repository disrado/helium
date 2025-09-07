#pragma once

#include <cstdint>


namespace he
{
using id_type = uint32_t;

namespace internal
{
struct sequential_index final
{
	[[nodiscard]] static auto value() noexcept -> id_type
	{
		static auto index{ id_type{} };
		return index++;
	}
};

template <typename T>
[[nodiscard]] constexpr auto get_index_of() noexcept -> id_type
{
	static auto id{ internal::sequential_index::value() };
	return id;
}
}


template <typename T>
struct type_index final
{
	[[nodiscard]] static constexpr auto value() noexcept -> id_type
	{
		return internal::get_index_of<T>();
	}
};
}
