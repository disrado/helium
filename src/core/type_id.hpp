#pragma once

#include "type_index.hpp"
#include "type_name.hpp"

#include <utility>


namespace he
{
struct type_info final
{
	template <typename T>
	constexpr type_info(std::in_place_type_t<T>) noexcept
		: _type_index{ type_index<T>::value() },
		  _type_name{ type_name<T>::value() }
	{
	}

	[[nodiscard]] constexpr auto index() const noexcept -> index_type
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
	index_type _type_index;
	std::string_view _type_name;
};

template <typename T>
[[nodiscard]] constexpr auto type_id() noexcept -> type_info
{
	static const auto info{ type_info{ std::in_place_type<T> } };
	return info;
}
}
