#pragma once

#include "utils/delegate/utils.hpp"

#include <functional>
#include <memory>


namespace he
{

template <typename... arg_ts>
class delegate final
{
public:
    delegate() = default;

    delegate(const delegate&) = default;
    delegate& operator =(const delegate&) = default;

    delegate(delegate&&) = default;
    delegate& operator =(delegate&&) = default;

    template <typename callable_t>
        requires delegates::bindable<callable_t, arg_ts...>
    explicit delegate(callable_t&& callback);

    template <typename lifetime_owner_t, typename callable_t>
        requires delegates::lifetime_bound_bindable<callable_t, arg_ts...>
    delegate(std::weak_ptr<lifetime_owner_t> owner, callable_t&& callback);

public:
    template <typename callable_t>
        requires delegates::bindable<callable_t, arg_ts...>
    auto bind(callable_t&& callback) -> void;

    template <typename lifetime_owner_t, typename callable_t>
        requires delegates::lifetime_bound_bindable<callable_t, arg_ts...>
    auto bind(std::weak_ptr<lifetime_owner_t> owner, callable_t&& callback) -> void;

    [[nodiscard]] auto try_execute(arg_ts... args) const -> bool;

    auto execute(arg_ts... args) const -> void;

    [[nodiscard]] auto is_bound() const -> bool;

private:
    std::function<bool(arg_ts...)> _callback;
};

template <typename... arg_ts>
template <typename callable_t>
    requires delegates::bindable<callable_t, arg_ts...>
delegate<arg_ts...>::delegate(callable_t&& callback)
{
    bind(std::forward<decltype(callback)>(callback));
}

template <typename... arg_ts>
template <typename lifetime_owner_t, typename callable_t>
    requires delegates::lifetime_bound_bindable<callable_t, arg_ts...>
delegate<arg_ts...>::delegate(std::weak_ptr<lifetime_owner_t> owner, callable_t&& callback)
{
    bind(owner, std::forward<decltype(callback)>(callback));
}

template <typename... arg_ts>
template <typename callable_t>
    requires delegates::bindable<callable_t, arg_ts...>
auto delegate<arg_ts...>::bind(callable_t&& callback) -> void
{
    _callback = [callback = std::forward<decltype(callback)>(callback)] (arg_ts... args)
    {
        std::invoke(callback, std::forward<arg_ts>(args)...);

        return true;
    };
}

template <typename... arg_ts>
template <typename lifetime_owner_t, typename callable_t>
    requires delegates::lifetime_bound_bindable<callable_t, arg_ts...>
auto delegate<arg_ts...>::bind(std::weak_ptr<lifetime_owner_t> owner, callable_t&& callback) -> void
{
    _callback = [owner, callback = std::forward<decltype(callback)>(callback)] (arg_ts&&... args)
    {
        if (auto locked_owner{ owner.lock() })
        {
            if constexpr (std::is_member_function_pointer_v<decltype(callback)>)
            {
                std::invoke(callback, *locked_owner, std::forward<arg_ts>(args)...);
            }
            else
            {
                std::invoke(callback, std::forward<arg_ts>(args)...);
            }

            return true;
        }

        return false;
    };
}

template <typename... arg_ts>
auto delegate<arg_ts...>::try_execute(arg_ts... args) const -> bool
{
    if (!_callback)
    {
        return false;
    }

    return std::invoke(_callback, std::forward<arg_ts>(args)...);
}

template <typename... arg_ts>
auto delegate<arg_ts...>::execute(arg_ts... args) const -> void
{
    if (_callback)
    {
        std::invoke(_callback, std::forward<arg_ts>(args)...);
    }
}

template <typename... arg_ts>
auto delegate<arg_ts...>::is_bound() const -> bool
{
    return static_cast<bool>(_callback);
}

}
