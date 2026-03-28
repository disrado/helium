#include "engine/utils/event_bus.hpp"

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

    SECTION("binding delegate")
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

    SECTION("emitting multiple events")
    {
        auto event_bus{ he::event_bus{} };

        auto counter{ 0ul };

        event_bus.on<system_created>([&counter] (const auto& _) { counter++; });
        event_bus.on<state_changed>([&counter] (const auto& _) { counter++; });

        REQUIRE(event_bus.emit(system_created{}));
        REQUIRE(event_bus.emit(state_changed{}));

        REQUIRE(counter == 2);
    }

    SECTION("unbinding")
    {
        auto event_bus{ he::event_bus{} };

        auto counter{ 0ul };

        const auto system_created_handle{ event_bus.on<system_created>([&counter] (const auto& _) { counter++; }) };
        const auto state_changed_handle{ event_bus.on<state_changed>([&counter] (const auto& _) { counter++; }) };

        event_bus.emit(system_created{});
        event_bus.emit(state_changed{});

        REQUIRE(counter == 2);

        REQUIRE(event_bus.unbind<system_created>(system_created_handle));
        REQUIRE(event_bus.unbind<state_changed>(state_changed_handle));

        event_bus.emit(system_created{});
        event_bus.emit(state_changed{});

        REQUIRE(counter == 2);
    }
}
