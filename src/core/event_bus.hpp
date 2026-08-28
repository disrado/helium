#pragma once

#include "core/delegate/multicast_delegate.hpp"
#include "core/type_traits/type_index.hpp"

#include <map>


namespace he
{

// Type-erased publisher/subscriber bus
// cv/ref-agnostic
class event_bus final
{
public:
    struct handle final
    {
        using handle_id_t = uint32_t;

        handle_id_t id;
    };

private:
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
    auto on(delegate<void(event_t)> delegate) -> handle;

    template <typename event_t>
    auto unbind(const handle& target_handle) -> bool;

    template <typename event_t>
    auto emit(event_t&& event) -> bool;

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
    using raw_event_t = std::remove_cvref_t<event_t>;

    return on<raw_event_t>(delegate<void(raw_event_t)>{ std::forward<callable_t>(callback) });
}

template <typename event_t, typename lifetime_owner_t, typename callable_t>
    requires delegates::lifetime_bound_bindable<callable_t, event_t>
auto event_bus::on(std::weak_ptr<lifetime_owner_t> owner, callable_t&& callback) -> handle
{
    using raw_event_t = std::remove_cvref_t<event_t>;

    return on<raw_event_t>(delegate<void(raw_event_t)>{ owner, std::forward<callable_t>(callback) });
}

template <typename event_t>
auto event_bus::on(delegate<void(event_t)> delegate) -> handle
{
    using raw_event_t = std::remove_cvref_t<event_t>;

    if (const auto event_id{ type_index<raw_event_t>() }; _events.contains(type_index<raw_event_t>()))
    {
        return handle{
            .id = access_slot<raw_event_t>().delegate.bind(std::move(delegate)).id
        };
    }
    else
    {
        auto slot{ std::make_shared<event_slot<raw_event_t>>() };

        const auto return_handle{ handle{
            .id = slot->delegate.bind(std::move(delegate)).id
        } };

        _events.emplace(event_id, std::move(slot));

        return return_handle;
    }
}

template <typename event_t>
auto event_bus::unbind(const handle& target_handle) -> bool
{
    using raw_event_t = std::remove_cvref_t<event_t>;

    if (const auto event_id{ type_index<raw_event_t>() }; _events.contains(type_index<raw_event_t>()))
    {
        const auto unbound{ access_slot<raw_event_t>().delegate.unbind(
            typename multicast_delegate<raw_event_t>::handle{
                .id = target_handle.id
            }) };

        if (!access_slot<raw_event_t>().delegate.is_bound())
        {
            _events.erase(event_id);
        }

        return unbound;
    }

    return false;
}

template <typename event_t>
auto event_bus::emit(event_t&& event) -> bool
{
    using raw_event_t = std::remove_cvref_t<event_t>;

    if (_events.contains(type_index<raw_event_t>()))
    {
        access_slot<raw_event_t>().delegate.execute(std::forward<raw_event_t>(event));

        return true;
    }

    return false;
}

template <typename event_t>
auto event_bus::access_slot() -> event_slot<event_t>&
{
    return *(static_cast<event_slot<event_t>*>(_events.at(type_index<event_t>()).get()));
}

}
