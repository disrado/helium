#pragma once

#include "core/delegate/utils.hpp"

#include <functional>
#include <memory>
#include <optional>


namespace he
{

template <typename return_t, typename... arg_ts>
class rdelegate final
{
public:
    rdelegate() = default;

    rdelegate(const rdelegate&) = default;
    rdelegate& operator =(const rdelegate&) = default;

    rdelegate(rdelegate&&) = default;
    rdelegate& operator =(rdelegate&&) = default;

    template <typename callable_t>
        requires delegates::bindable<callable_t, arg_ts...>
    explicit rdelegate(callable_t&& callback);

    template <typename lifetime_owner_t, typename callable_t>
        requires delegates::lifetime_bound_bindable<callable_t, arg_ts...>
    rdelegate(std::weak_ptr<lifetime_owner_t> owner, callable_t&& callback);

public:
    template <typename callable_t>
        requires delegates::bindable<callable_t, arg_ts...>
    auto bind(callable_t&& callback) -> void;

    template <typename lifetime_owner_t, typename callable_t>
        requires delegates::lifetime_bound_bindable<callable_t, arg_ts...>
    auto bind(std::weak_ptr<lifetime_owner_t> owner, callable_t&& callback) -> void;

    [[nodiscard]] auto try_execute(arg_ts... args) const -> std::optional<return_t>;

    template <typename... ts>
        requires (sizeof...(ts) == sizeof...(arg_ts)) && (std::convertible_to<ts, arg_ts> && ...)
    auto execute(ts&&... args) const -> std::optional<return_t>;

    [[nodiscard]] auto is_bound() const -> bool;

private:
    std::function<std::optional<return_t>(arg_ts...)> _callback;
};

template <typename return_t, typename... arg_ts>
template <typename callable_t>
    requires delegates::bindable<callable_t, arg_ts...>
rdelegate<return_t, arg_ts...>::rdelegate(callable_t&& callback)
{
    bind(std::forward<decltype(callback)>(callback));
}

template <typename return_t, typename... arg_ts>
template <typename lifetime_owner_t, typename callable_t>
    requires delegates::lifetime_bound_bindable<callable_t, arg_ts...>
rdelegate<return_t, arg_ts...>::rdelegate(std::weak_ptr<lifetime_owner_t> owner, callable_t&& callback)
{
    bind(owner, std::forward<decltype(callback)>(callback));
}

template <typename return_t, typename... arg_ts>
template <typename callable_t>
    requires delegates::bindable<callable_t, arg_ts...>
auto rdelegate<return_t, arg_ts...>::bind(callable_t&& callback) -> void
{
    _callback = [callback = std::forward<decltype(callback)>(callback)] (arg_ts... args)
    {
        return std::invoke(callback, std::forward<arg_ts>(args)...);
    };
}

template <typename return_t, typename... arg_ts>
template <typename lifetime_owner_t, typename callable_t>
    requires delegates::lifetime_bound_bindable<callable_t, arg_ts...>
auto rdelegate<return_t, arg_ts...>::bind(std::weak_ptr<lifetime_owner_t> owner, callable_t&& callback) -> void
{
    _callback = [owner, callback = std::forward<decltype(callback)>(callback)] (arg_ts&&... args) -> std::optional<return_t>
    {
        if (auto locked_owner{ owner.lock() })
        {
            if constexpr (std::is_member_function_pointer_v<decltype(callback)>)
            {
                return std::invoke(callback, *locked_owner, std::forward<arg_ts>(args)...);
            }
            else
            {
                return std::invoke(callback, std::forward<arg_ts>(args)...);
            }
        }

        return std::nullopt;
    };
}

template <typename return_t, typename... arg_ts>
auto rdelegate<return_t, arg_ts...>::try_execute(arg_ts... args) const -> std::optional<return_t>
{
    if (!_callback)
    {
        return std::nullopt;
    }

    return std::invoke(_callback, std::forward<arg_ts>(args)...);
}

template <typename return_t, typename... arg_ts>
template <typename... ts>
    requires (sizeof...(ts) == sizeof...(arg_ts)) && (std::convertible_to<ts, arg_ts> && ...)
auto rdelegate<return_t, arg_ts...>::execute(ts&&... args) const -> std::optional<return_t>
{
    if (_callback)
    {
        return std::invoke(_callback, std::forward<ts>(args)...);
    }

    return std::nullopt;
}

template <typename return_t, typename... arg_ts>
auto rdelegate<return_t, arg_ts...>::is_bound() const -> bool
{
    return static_cast<bool>(_callback);
}

}
