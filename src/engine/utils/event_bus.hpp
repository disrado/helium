#pragma once

#include "engine/utils/delegate/multicast_delegate.hpp"
#include "engine/utils/type_traits/type_index.hpp"

#include <map>


namespace he
{

class event_bus final
{
public:
    struct handle final
    {
        uint32_t id;
    };

private:
    using handle_id_t = uint32_t;

    struct event_slot_base
    {
        virtual ~event_slot_base() = default;
    };

    template <typename event_t>
    struct event_slot: public event_slot_base
    {
        he::multicast_delegate<event_t> delegate;
    };

public:
    template <typename event_t, typename callable_t>
        requires delegates::bindable<callable_t, event_t>
    auto on(callable_t&& callback) -> handle;

    template <typename event_t, typename lifetime_owner_t, typename callable_t>
        requires delegates::lifetime_bound_bindable<callable_t, event_t>
    auto on(std::weak_ptr<lifetime_owner_t> owner, callable_t&& callback) -> handle;

    template <typename event_t>
    auto on(delegate<event_t> delegate) -> handle;

    template <typename event_t>
    auto unbind(const handle& handle) -> bool;

    template <typename event_t>
    auto emit(auto&& event) -> bool;

private:
    template <typename event_t>
    auto access_slot() -> event_slot<event_t>&;

private:
    std::map<type_index_t, std::shared_ptr<event_slot_base>> _events;
};

template <typename event_t, typename callable_t>
    requires delegates::bindable<callable_t, event_t>
auto event_bus::on(callable_t&& callback) -> handle
{
    return on<event_t>(delegate<event_t>{ std::forward<callable_t>(callback) });
}

template <typename event_t, typename lifetime_owner_t, typename callable_t>
    requires delegates::lifetime_bound_bindable<callable_t, event_t>
auto event_bus::on(std::weak_ptr<lifetime_owner_t> owner, callable_t&& callback) -> handle
{
    return on<event_t>(delegate<event_t>{ owner, std::forward<callable_t>(callback) });
}

template <typename event_t>
auto event_bus::on(delegate<event_t> delegate) -> handle
{
    if (const auto event_id{ type_index<event_t>::value() }; _events.contains(type_index<event_t>::value()))
    {
        return handle{
            .id = access_slot<event_t>().delegate.bind(std::move(delegate)).id
        };
    }
    else
    {
        auto slot{ std::make_shared<event_slot<event_t>>() };

        const auto return_handle{ handle{
            .id = slot->delegate.bind(std::move(delegate)).id
        } };

        _events.emplace(event_id, std::move(slot));

        return return_handle;
    }
}

template <typename event_t>
auto event_bus::unbind(const handle& handle) -> bool
{
    if (const auto event_id{ type_index<event_t>::value() }; _events.contains(type_index<event_t>::value()))
    {
        const auto unbound{ access_slot<event_t>().delegate.unbind(
            typename multicast_delegate<event_t>::handle{
                .id = handle.id
            }) };

        if (!access_slot<event_t>().delegate.is_bound())
        {
            _events.erase(event_id);
        }

        return unbound;
    }

    return false;
}

template <typename event_t>
auto event_bus::emit(auto&& event) -> bool
{
    if (_events.contains(type_index<event_t>::value()))
    {
        access_slot<event_t>().delegate.execute(std::forward<event_t>(event));

        return true;
    }

    return false;
}

template <typename event_t>
auto event_bus::access_slot() -> event_slot<event_t>&
{
    return *(static_cast<event_slot<event_t>*>(_events.at(type_index<event_t>::value()).get()));
}

}
