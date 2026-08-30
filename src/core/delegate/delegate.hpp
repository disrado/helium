#pragma once

#include "core/delegate/utils.hpp"

#include <cassert>
#include <functional>
#include <memory>
#include <optional>


namespace he
{

template <typename signature_t = void()>
class delegate;


template <typename... arg_ts>
class delegate<void(arg_ts...)> final
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

    template <typename... ts>
        requires (sizeof...(ts) == sizeof...(arg_ts)) && (std::convertible_to<ts, arg_ts> && ...)
    [[nodiscard]] auto try_execute(ts&&... args) const -> bool;

    template <typename... ts>
        requires (sizeof...(ts) == sizeof...(arg_ts)) && (std::convertible_to<ts, arg_ts> && ...)
    auto execute(ts&&... args) const -> void;

    [[nodiscard]] auto is_bound() const -> bool;

private:
    std::function<bool(arg_ts...)> _callback;
};


template <typename... arg_ts>
template <typename callable_t>
    requires delegates::bindable<callable_t, arg_ts...>
delegate<void(arg_ts...)>::delegate(callable_t&& callback)
{
    bind(std::forward<decltype(callback)>(callback));
}


template <typename... arg_ts>
template <typename lifetime_owner_t, typename callable_t>
    requires delegates::lifetime_bound_bindable<callable_t, arg_ts...>
delegate<void(arg_ts...)>::delegate(std::weak_ptr<lifetime_owner_t> owner, callable_t&& callback)
{
    bind(owner, std::forward<decltype(callback)>(callback));
}


template <typename... arg_ts>
template <typename callable_t>
    requires delegates::bindable<callable_t, arg_ts...>
auto delegate<void(arg_ts...)>::bind(callable_t&& callback) -> void
{
    _callback = [callback{ std::forward<decltype(callback)>(callback) }] (arg_ts... args) mutable
    {
        std::invoke(callback, std::forward<arg_ts>(args)...);

        return true;
    };
}


template <typename... arg_ts>
template <typename lifetime_owner_t, typename callable_t>
    requires delegates::lifetime_bound_bindable<callable_t, arg_ts...>
auto delegate<void(arg_ts...)>::bind(std::weak_ptr<lifetime_owner_t> owner, callable_t&& callback) -> void
{
    _callback = [owner, callback{ std::forward<decltype(callback)>(callback) }] (arg_ts&&... args) mutable
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
template <typename... ts>
    requires (sizeof...(ts) == sizeof...(arg_ts)) && (std::convertible_to<ts, arg_ts> && ...)
auto delegate<void(arg_ts...)>::try_execute(ts&&... args) const -> bool
{
    return _callback ? std::invoke(_callback, std::forward<ts>(args)...) : false;
}


template <typename... arg_ts>
template <typename... ts>
    requires (sizeof...(ts) == sizeof...(arg_ts)) && (std::convertible_to<ts, arg_ts> && ...)
auto delegate<void(arg_ts...)>::execute(ts&&... args) const -> void
{
    assert(_callback && "unbound delegate");

    [[maybe_unused]] const auto succeeded{ std::invoke(_callback, std::forward<ts>(args)...) };
    assert(succeeded && "owner expired");
}


template <typename... arg_ts>
auto delegate<void(arg_ts...)>::is_bound() const -> bool
{
    return static_cast<bool>(_callback);
}


namespace delegates
{

template <typename callable_t, typename call_op_t>
struct delegate_signature_from_call_op;


template <typename callable_t, typename class_t, typename return_t, typename... arg_ts>
struct delegate_signature_from_call_op<callable_t, return_t(class_t::*)(arg_ts...)>
{
    using type = return_t(arg_ts...);
};


template <typename callable_t, typename class_t, typename return_t, typename... arg_ts>
struct delegate_signature_from_call_op<callable_t, return_t(class_t::*)(arg_ts...) const>
{
    using type = return_t(arg_ts...);
};

}


template <typename callable_t>
    requires delegates::callable<callable_t>
delegate(callable_t) -> delegate<typename delegates::delegate_signature_from_call_op<callable_t, decltype(&callable_t::operator())>::type>;


template <typename lifetime_owner_t, typename class_t, typename return_t, typename... arg_ts>
delegate(std::weak_ptr<lifetime_owner_t>, return_t (class_t::*)(arg_ts...)) -> delegate<return_t(arg_ts...)>;


template <typename lifetime_owner_t, typename class_t, typename return_t, typename... arg_ts>
delegate(std::weak_ptr<lifetime_owner_t>, return_t (class_t::*)(arg_ts...) const) -> delegate<return_t(arg_ts...)>;


template <typename lifetime_owner_t, typename return_t, typename... arg_ts>
delegate(std::weak_ptr<lifetime_owner_t>, return_t (*)(arg_ts...)) -> delegate<return_t(arg_ts...)>;


template <typename lifetime_owner_t, typename callable_t>
    requires delegates::callable<callable_t>
delegate(std::weak_ptr<lifetime_owner_t>,
         callable_t) -> delegate<typename delegates::delegate_signature_from_call_op<callable_t, decltype(&callable_t::operator())>::type>;


template <typename return_t, typename... arg_ts>
class delegate<return_t(arg_ts...)> final
{
public:
    delegate() = default;

    delegate(const delegate&) = default;
    delegate& operator =(const delegate&) = default;

    delegate(delegate&&) = default;
    delegate& operator =(delegate&&) = default;

    template <typename callable_t>
        requires delegates::rbindable<return_t, callable_t, arg_ts...>
    explicit delegate(callable_t&& callback);

    template <typename lifetime_owner_t, typename callable_t>
        requires delegates::lifetime_bound_rbindable<return_t, callable_t, arg_ts...>
    delegate(std::weak_ptr<lifetime_owner_t> owner, callable_t&& callback);

public:
    template <typename callable_t>
        requires delegates::rbindable<return_t, callable_t, arg_ts...>
    auto bind(callable_t&& callback) -> void;

    template <typename lifetime_owner_t, typename callable_t>
        requires delegates::lifetime_bound_rbindable<return_t, callable_t, arg_ts...>
    auto bind(std::weak_ptr<lifetime_owner_t> owner, callable_t&& callback) -> void;

    template <typename... ts>
        requires (sizeof...(ts) == sizeof...(arg_ts)) && (std::convertible_to<ts, arg_ts> && ...)
    [[nodiscard]] auto try_execute(ts&&... args) const -> std::optional<return_t>;

    template <typename... ts>
        requires (sizeof...(ts) == sizeof...(arg_ts)) && (std::convertible_to<ts, arg_ts> && ...)
    [[nodiscard]] auto execute(ts&&... args) const -> return_t;

    [[nodiscard]] auto is_bound() const -> bool;

private:
    std::function<std::optional<return_t>(arg_ts...)> _callback;
};


template <typename return_t, typename... arg_ts>
template <typename callable_t>
    requires delegates::rbindable<return_t, callable_t, arg_ts...>
delegate<return_t(arg_ts...)>::delegate(callable_t&& callback)
{
    bind(std::forward<decltype(callback)>(callback));
}


template <typename return_t, typename... arg_ts>
template <typename lifetime_owner_t, typename callable_t>
    requires delegates::lifetime_bound_rbindable<return_t, callable_t, arg_ts...>
delegate<return_t(arg_ts...)>::delegate(std::weak_ptr<lifetime_owner_t> owner, callable_t&& callback)
{
    bind(owner, std::forward<decltype(callback)>(callback));
}


template <typename return_t, typename... arg_ts>
template <typename callable_t>
    requires delegates::rbindable<return_t, callable_t, arg_ts...>
auto delegate<return_t(arg_ts...)>::bind(callable_t&& callback) -> void
{
    _callback = [callback{ std::forward<decltype(callback)>(callback) }] (arg_ts... args) mutable
    {
        return std::invoke_r<return_t>(callback, std::forward<arg_ts>(args)...);
    };
}


template <typename return_t, typename... arg_ts>
template <typename lifetime_owner_t, typename callable_t>
    requires delegates::lifetime_bound_rbindable<return_t, callable_t, arg_ts...>
auto delegate<return_t(arg_ts...)>::bind(std::weak_ptr<lifetime_owner_t> owner, callable_t&& callback) -> void
{
    _callback = [owner, callback{ std::forward<decltype(callback)>(callback) }] (arg_ts&&... args) mutable -> std::optional<return_t>
    {
        if (auto locked_owner{ owner.lock() })
        {
            if constexpr (std::is_member_function_pointer_v<decltype(callback)>)
            {
                return std::invoke_r<return_t>(callback, *locked_owner, std::forward<arg_ts>(args)...);
            }
            else
            {
                return std::invoke_r<return_t>(callback, std::forward<arg_ts>(args)...);
            }
        }

        return std::nullopt;
    };
}


template <typename return_t, typename... arg_ts>
template <typename... ts>
    requires (sizeof...(ts) == sizeof...(arg_ts)) && (std::convertible_to<ts, arg_ts> && ...)
auto delegate<return_t(arg_ts...)>::try_execute(ts&&... args) const -> std::optional<return_t>
{
    return _callback ? std::invoke(_callback, std::forward<ts>(args)...) : std::nullopt;
}


template <typename return_t, typename... arg_ts>
template <typename... ts>
    requires (sizeof...(ts) == sizeof...(arg_ts)) && (std::convertible_to<ts, arg_ts> && ...)
auto delegate<return_t(arg_ts...)>::execute(ts&&... args) const -> return_t
{
    assert(_callback && "unbound delegate");

    return std::invoke(_callback, std::forward<ts>(args)...).value();
}


template <typename return_t, typename... arg_ts>
auto delegate<return_t(arg_ts...)>::is_bound() const -> bool
{
    return static_cast<bool>(_callback);
}

}
