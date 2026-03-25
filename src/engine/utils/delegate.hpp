#pragma once

#include <concepts>
#include <memory>
#include <functional>


namespace he
{
template <typename... args_ts>
class delegate final
{
public:
    delegate() = default;

    delegate(const delegate&) = default;
    delegate& operator =(const delegate&) = default;

    delegate(const delegate&&) = default;
    delegate& operator =(const delegate&&) = default;

    explicit delegate(std::invocable<args_ts...> auto&& callback);

    template <typename caller_t>
    delegate(std::weak_ptr<caller_t> caller, auto&& callback);

    template <typename caller_t>
    delegate(std::shared_ptr<caller_t> caller, auto&& callback);

public:
    auto bind(std::invocable<args_ts...> auto&& callback) -> void;

    template <typename caller_t>
    auto bind(std::weak_ptr<caller_t> caller, auto&& callback) -> void;

    template <typename caller_t>
    auto bind(std::shared_ptr<caller_t> caller, auto&& callback) -> void;

    [[nodiscard]] auto try_execute(args_ts... args) -> bool;

    auto execute(args_ts... args) -> void;

    auto is_bound() -> bool;

private:
    std::function<bool(args_ts...)> _callback;
};

template <typename... args_ts>
delegate<args_ts...>::delegate(std::invocable<args_ts...> auto&& callback)
{
    bind(std::forward<decltype(callback)>(callback));
}

template <typename... args_ts>
template <typename caller_t>
delegate<args_ts...>::delegate(std::weak_ptr<caller_t> caller, auto&& callback)
{
    bind(caller, std::forward<decltype(callback)>(callback));
}

template <typename... args_ts>
template <typename caller_t>
delegate<args_ts...>::delegate(std::shared_ptr<caller_t> caller, auto&& callback)
{
    bind(caller, std::forward<decltype(callback)>(callback));
}

template <typename... args_ts>
auto delegate<args_ts...>::bind(std::invocable<args_ts...> auto&& callback) -> void
{
    _callback = [callback = std::forward<decltype(callback)>(callback)] (args_ts... args)
    {
        std::invoke(callback, std::forward<args_ts>(args)...);

        return true;
    };
}

template <typename... args_ts>
template <typename caller_t>
auto delegate<args_ts...>::bind(std::weak_ptr<caller_t> caller, auto&& callback) -> void
{
    _callback = [weak_caller = caller, callback = std::forward<decltype(callback)>(callback)] (args_ts&&... args)
    {
        if (auto shared_caller{ weak_caller.lock() })
        {
            if constexpr (std::is_member_function_pointer_v<decltype(callback)>)
            {
                std::invoke(callback, *shared_caller, std::forward<args_ts>(args)...);
            }
            else
            {
                std::invoke(callback, std::forward<args_ts>(args)...);
            }

            return true;
        }

        return false;
    };
}

template <typename... args_ts>
template <typename caller_t>
auto delegate<args_ts...>::bind(std::shared_ptr<caller_t> caller, auto&& callback) -> void
{
    bind(std::weak_ptr{ caller }, std::forward<decltype(callback)>(callback));
}

template <typename... args_ts>
auto delegate<args_ts...>::try_execute(args_ts... args) -> bool
{
    if (!_callback)
    {
        return false;
    }

    return std::invoke(_callback, std::forward<args_ts>(args)...);
}

template <typename... args_ts>
auto delegate<args_ts...>::execute(args_ts... args) -> void
{
    if (_callback)
    {
        std::invoke(_callback, std::forward<args_ts>(args)...);
    }
}

template <typename... args_ts>
auto delegate<args_ts...>::is_bound() -> bool
{
    return static_cast<bool>(_callback);
}
}
