#pragma once

#include "core/delegate/delegate.hpp"

#include <algorithm>
#include <cstdint>


namespace he
{

template <typename... arg_ts>
class multicast_delegate final
{
private:
    using handle_id_t = uint32_t;

public:
    struct handle final
    {
        uint32_t id;
    };

private:
    struct bound_entry final
    {
        handle _handle;
        delegate<arg_ts...> _delegate;
    };

public:
    template <typename callable_t>
        requires delegates::bindable<callable_t, arg_ts...>
    auto bind(callable_t&& callback) -> handle;

    template <typename lifetime_owner_t, typename callable_t>
        requires delegates::lifetime_bound_bindable<callable_t, arg_ts...>
    auto bind(std::weak_ptr<lifetime_owner_t> owner, callable_t&& callback) -> handle;

    auto bind(he::delegate<arg_ts...> delegate) -> handle;

    auto unbind(const handle& target_handle) -> bool;

    auto unbind_all() -> void;

    auto execute(arg_ts&&... args) const -> bool;

    [[nodiscard]] auto is_bound() const -> bool;

private:
    auto generate_handle_id() -> handle_id_t;

private:
    // 1 prevents removal via default-initialized handles
    handle_id_t _handle_counter{ 1ul };

    std::vector<bound_entry> _bound_list;
};

template <typename... arg_ts>
template <typename callable_t>
    requires delegates::bindable<callable_t, arg_ts...>
auto multicast_delegate<arg_ts...>::bind(callable_t&& callback) -> handle
{
    return bind(delegate<arg_ts...>{ std::forward<callable_t>(callback) });
}

template <typename... arg_ts>
template <typename lifetime_owner_t, typename callable_t>
    requires delegates::lifetime_bound_bindable<callable_t, arg_ts...>
auto multicast_delegate<arg_ts...>::bind(std::weak_ptr<lifetime_owner_t> owner, callable_t&& callback) -> handle
{
    return bind(delegate<arg_ts...>{ owner, std::forward<callable_t>(callback) });
}

template <typename... arg_ts>
auto multicast_delegate<arg_ts...>::bind(he::delegate<arg_ts...> delegate) -> handle
{
    const auto& [new_handle, bound_delegate]{
        _bound_list.emplace_back(
            bound_entry{
                ._handle = {
                    .id = generate_handle_id()
                },
                ._delegate = std::move(delegate)
            })
    };

    return new_handle;
}

template <typename... arg_ts>
auto multicast_delegate<arg_ts...>::unbind(const handle& target_handle) -> bool
{
    return std::erase_if(
        _bound_list, [&target_handle] (const auto& entry)
        {
            return entry._handle.id == target_handle.id;
        });
}

template <typename... arg_ts>
auto multicast_delegate<arg_ts...>::unbind_all() -> void
{
    _bound_list.clear();
}

template <typename... arg_ts>
auto multicast_delegate<arg_ts...>::execute(arg_ts&&... args) const -> bool
{
    for (const auto& entry: _bound_list)
    {
        entry._delegate.execute(std::forward<arg_ts>(args)...);
    }

    return !_bound_list.empty();
}

template <typename... arg_ts>
auto multicast_delegate<arg_ts...>::is_bound() const -> bool
{
    return !_bound_list.empty();
}

template <typename... arg_ts>
auto multicast_delegate<arg_ts...>::generate_handle_id() -> handle_id_t
{
    return _handle_counter++;
}

}
