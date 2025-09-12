#pragma once

#include <concepts>
#include <cstdint>
#include <memory>
#include <functional>


namespace he
{
template <typename... Ts>
class delegate final
{
public:
	struct handle
	{

	};

private:
	struct callback
	{
		std::uint32_t index;;
		std::function<Ts...> _callback;
		// TOptional<std::weak_ptr<T>> _lifetime_owner;
	};

public:
	auto execute(Ts&&...) -> void
	{

	}

	auto assign(std::invocable<Ts...> auto callback) -> void
	{

	}

	template <typename T>
	auto assign(std::invocable<Ts...> auto callback, std::weak_ptr<T> lifetime_owner) -> void
	{
		
	}

private:
	std::vector<callback> _callbacks;

	static std::uint32_t index;
};
}