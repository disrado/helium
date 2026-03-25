#pragma once

#include <functional>
#include <memory>


namespace he
{
namespace details
{
template <typename callable_t, typename... arg_ts>
concept bindable = std::is_invocable_v<callable_t, arg_ts...>;

template <typename callable_t, typename... arg_ts>
concept lifetime_bound_bindable = std::is_member_function_pointer_v<callable_t> || bindable<callable_t, arg_ts...>;
}

template <typename... arg_ts>
class delegate final
{
public:
    delegate() = default;

    delegate(const delegate&) = default;
    delegate& operator =(const delegate&) = default;

    delegate(const delegate&&) = default;
    delegate& operator =(const delegate&&) = default;

    template <typename callable_t>
        requires details::bindable<callable_t, arg_ts...>
    explicit delegate(callable_t&& callback);

    template <typename caller_t, typename callable_t>
        requires details::lifetime_bound_bindable<callable_t, arg_ts...>
    delegate(std::weak_ptr<caller_t> caller, callable_t&& callback);

    template <typename caller_t, typename callable_t>
        requires details::lifetime_bound_bindable<callable_t, arg_ts...>
    delegate(std::shared_ptr<caller_t> caller, callable_t&& callback);

public:
    template <typename callable_t>
        requires details::bindable<callable_t, arg_ts...>
    auto bind(callable_t&& callback) -> void;

    template <typename caller_t, typename callable_t>
        requires details::lifetime_bound_bindable<callable_t, arg_ts...>
    auto bind(std::weak_ptr<caller_t> caller, callable_t&& callback) -> void;

    template <typename caller_t, typename callable_t>
        requires details::lifetime_bound_bindable<callable_t, arg_ts...>
    auto bind(std::shared_ptr<caller_t> caller, callable_t&& callback) -> void;

    [[nodiscard]] auto try_execute(arg_ts... args) -> bool;

    auto execute(arg_ts... args) -> void;

    auto is_bound() -> bool;

private:
    std::function<bool(arg_ts...)> _callback;
};

template <typename... arg_ts>
template <typename callable_t>
    requires details::bindable<callable_t, arg_ts...>
delegate<arg_ts...>::delegate(callable_t&& callback)
{
    bind(std::forward<decltype(callback)>(callback));
}

template <typename... arg_ts>
template <typename caller_t, typename callable_t>
    requires details::lifetime_bound_bindable<callable_t, arg_ts...>
delegate<arg_ts...>::delegate(std::weak_ptr<caller_t> caller, callable_t&& callback)
{
    bind(caller, std::forward<decltype(callback)>(callback));
}

template <typename... arg_ts>
template <typename caller_t, typename callable_t>
    requires details::lifetime_bound_bindable<callable_t, arg_ts...>
delegate<arg_ts...>::delegate(std::shared_ptr<caller_t> caller, callable_t&& callback)
{
    bind(caller, std::forward<decltype(callback)>(callback));
}

template <typename... arg_ts>
template <typename callable_t>
    requires details::bindable<callable_t, arg_ts...>
auto delegate<arg_ts...>::bind(callable_t&& callback) -> void
{
    _callback = [callback = std::forward<decltype(callback)>(callback)] (arg_ts... args)
    {
        std::invoke(callback, std::forward<arg_ts>(args)...);

        return true;
    };
}

template <typename... arg_ts>
template <typename caller_t, typename callable_t>
    requires details::lifetime_bound_bindable<callable_t, arg_ts...>
auto delegate<arg_ts...>::bind(std::weak_ptr<caller_t> caller, callable_t&& callback) -> void
{
    _callback = [caller, callback = std::forward<decltype(callback)>(callback)] (arg_ts&&... args)
    {
        if (auto locked_caller{ caller.lock() })
        {
            if constexpr (std::is_member_function_pointer_v<decltype(callback)>)
            {
                std::invoke(callback, *locked_caller, std::forward<arg_ts>(args)...);
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
template <typename caller_t, typename callable_t>
    requires details::lifetime_bound_bindable<callable_t, arg_ts...>
auto delegate<arg_ts...>::bind(std::shared_ptr<caller_t> caller, callable_t&& callback) -> void
{
    bind(std::weak_ptr{ caller }, std::forward<decltype(callback)>(callback));
}

template <typename... arg_ts>
auto delegate<arg_ts...>::try_execute(arg_ts... args) -> bool
{
    if (!_callback)
    {
        return false;
    }

    return std::invoke(_callback, std::forward<arg_ts>(args)...);
}

template <typename... arg_ts>
auto delegate<arg_ts...>::execute(arg_ts... args) -> void
{
    if (_callback)
    {
        std::invoke(_callback, std::forward<arg_ts>(args)...);
    }
}

template <typename... arg_ts>
auto delegate<arg_ts...>::is_bound() -> bool
{
    return static_cast<bool>(_callback);
}
}
