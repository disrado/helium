#include "core/event_bus.hpp"

#include <catch2/catch_test_macros.hpp>

#include <string>


namespace
{

auto free_function(bool) -> void
{
    //
}

}

TEST_CASE("event_bus")
{
    using std::string_literals::operator ""s;

    struct system_created
    {
        std::string system_name;
    };

    struct state_changed
    {
        int32_t new_state;
    };

    SECTION("instantiation")
    {
        he::event_bus{};
    }

    SECTION("binding free function")
    {
        he::event_bus{}.on<bool>(&free_function);
    }

    SECTION("binding lambda")
    {
        he::event_bus{}.on<bool>([] (bool) {});
    }

    SECTION("binding std::function")
    {
        he::event_bus{}.on<bool>(std::function<void(bool)>{});
    }

    SECTION("binding method with weak owner")
    {
        struct owner final
        {
            auto callable(bool) -> void {};
        };

        auto lifetime_owner{ std::make_shared<owner>() };

        he::event_bus{}.on<bool>(std::weak_ptr{ lifetime_owner }, &owner::callable);
    }

    SECTION("binding _delegate")
    {
        he::event_bus{}.on<bool>(he::delegate<bool>{ [] (bool) {} });
    }

    SECTION("binding and emitting")
    {
        auto event_bus{ he::event_bus{} };

        auto system_name{ ""s };

        event_bus.on<system_created>([&system_name] (const system_created& event) { system_name = event.system_name; });

        REQUIRE(event_bus.emit(system_created{ .system_name = "backend_system"s }));

        REQUIRE(system_name == "backend_system"s);
    }

    SECTION("emitting an lvalue")
    {
        auto event_bus{ he::event_bus{} };

        auto system_name{ ""s };

        event_bus.on<system_created>([&system_name] (const system_created& event) { system_name = event.system_name; });

        auto event{ system_created{ .system_name = "backend_system"s } };
        REQUIRE(event_bus.emit(event));

        REQUIRE(system_name == "backend_system"s);
    }

    SECTION("interface ignores cvref of event type")
    {
        auto event_bus{ he::event_bus{} };

        auto plain_fired{ false };
        auto const_fired{ false };
        auto ref_fired{ false };
        auto rvalue_fired{ false };

        const auto plain_handle{ event_bus.on<system_created>([&plain_fired] (const auto&) { plain_fired = true; }) };
        const auto const_handle{ event_bus.on<const system_created>([&const_fired] (const auto&) { const_fired = true; }) };
        const auto ref_handle{ event_bus.on<system_created&>([&ref_fired] (const auto&) { ref_fired = true; }) };
        const auto rvalue_handle{ event_bus.on<system_created&&>([&rvalue_fired] (const auto&) { rvalue_fired = true; }) };

        auto event{ system_created{} };
        REQUIRE(event_bus.emit<system_created&>(event));
        REQUIRE(plain_fired);
        REQUIRE(const_fired);
        REQUIRE(ref_fired);
        REQUIRE(rvalue_fired);

        REQUIRE(event_bus.unbind<system_created&&>(plain_handle));
        REQUIRE(event_bus.unbind<const system_created>(ref_handle));
        REQUIRE(event_bus.unbind<system_created&>(const_handle));
        REQUIRE(event_bus.unbind<system_created>(rvalue_handle));
    }

    SECTION("emitting multiple events")
    {
        auto event_bus{ he::event_bus{} };

        auto counter{ 0ul };

        event_bus.on<system_created>([&counter] (const auto&) { counter++; });
        event_bus.on<state_changed>([&counter] (const auto&) { counter++; });

        REQUIRE(event_bus.emit(system_created{}));
        REQUIRE(event_bus.emit(state_changed{}));

        REQUIRE(counter == 2);
    }

    SECTION("multiple listeners, same type")
    {
        auto event_bus{ he::event_bus{} };

        auto first_fired{ false };
        auto second_fired{ false };

        event_bus.on<system_created>([&first_fired] (const auto&) { first_fired = true; });
        event_bus.on<system_created>([&second_fired] (const auto&) { second_fired = true; });

        REQUIRE(event_bus.emit(system_created{}));

        REQUIRE(first_fired);
        REQUIRE(second_fired);
    }

    SECTION("unbinding")
    {
        auto event_bus{ he::event_bus{} };

        auto counter{ 0ul };

        const auto system_created_handle{ event_bus.on<system_created>([&counter] (const auto&) { counter++; }) };
        const auto state_changed_handle{ event_bus.on<state_changed>([&counter] (const auto&) { counter++; }) };

        event_bus.emit(system_created{});
        event_bus.emit(state_changed{});

        REQUIRE(counter == 2);

        REQUIRE(event_bus.unbind<system_created>(system_created_handle));
        REQUIRE(event_bus.unbind<state_changed>(state_changed_handle));

        REQUIRE_FALSE(event_bus.emit(system_created{}));
        REQUIRE_FALSE(event_bus.emit(state_changed{}));

        REQUIRE(counter == 2);
    }

    SECTION("unregistered event type")
    {
        auto event_bus{ he::event_bus{} };

        REQUIRE_FALSE(event_bus.emit(system_created{}));
        REQUIRE_FALSE(event_bus.unbind<system_created>(he::event_bus::handle{}));
    }
}
