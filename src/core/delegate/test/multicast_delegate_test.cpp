#include "core/delegate/multicast_delegate.hpp"

#include <catch2/catch_test_macros.hpp>


namespace
{

auto free_function() -> void
{
    //
}

}

TEST_CASE("multicast_delegate")
{
    struct owner final
    {
        auto callable() -> void {};
    };

    SECTION("instantiating via default constructor")
    {
        REQUIRE_FALSE(he::multicast_delegate{}.is_bound());
    }

    SECTION("binding free function")
    {
        auto delegate{ he::multicast_delegate{} };

        delegate.bind(&free_function);

        REQUIRE(delegate.is_bound());
    }

    SECTION("binding lambda")
    {
        auto delegate{ he::multicast_delegate{} };

        delegate.bind([] {});

        REQUIRE(delegate.is_bound());
    }

    SECTION("binding std::function")
    {
        auto delegate{ he::multicast_delegate{} };

        delegate.bind(std::function<void()>{});

        REQUIRE(delegate.is_bound());
    }

    SECTION("binding method with weak owner")
    {
        auto delegate{ he::multicast_delegate{} };

        auto lifetime_owner{ std::make_shared<owner>() };

        delegate.bind(std::weak_ptr{ lifetime_owner }, &owner::callable);

        REQUIRE(delegate.is_bound());
    }

    SECTION("binding _delegate")
    {
        auto delegate{ he::multicast_delegate<double, std::string&&>{} };

        delegate.bind(he::delegate<double, std::string&&>{ [] (double, std::string&&) {} });

        REQUIRE(delegate.is_bound());
    }

    SECTION("unbinding")
    {
        auto delegate{ he::multicast_delegate{} };

        auto lifetime_owner{ std::make_shared<owner>() };

        const auto first_handle{ delegate.bind(std::function<void()>{}) };

        const auto second_handle{ delegate.bind(std::function<void()>{}) };

        REQUIRE(delegate.unbind(second_handle));

        REQUIRE(delegate.is_bound());

        REQUIRE(delegate.unbind(first_handle));

        REQUIRE_FALSE(delegate.is_bound());
    }

    SECTION("unbinding default handle")
    {
        auto delegate{ he::multicast_delegate{} };

        delegate.bind([] {});

        REQUIRE_FALSE(delegate.unbind(decltype(delegate)::handle{}));
        REQUIRE(delegate.is_bound());
    }

    SECTION("unbinding twice")
    {
        auto delegate{ he::multicast_delegate{} };

        const auto handle{ delegate.bind([] {}) };

        REQUIRE(delegate.unbind(handle));
        REQUIRE_FALSE(delegate.unbind(handle));
    }

    SECTION("unbinding of all")
    {
        auto delegate{ he::multicast_delegate{} };

        delegate.bind([] {});
        delegate.bind(&free_function);
        delegate.bind(std::function<void()>{});

        REQUIRE(delegate.is_bound());

        delegate.unbind_all();

        REQUIRE_FALSE(delegate.is_bound());
    }

}

TEST_CASE("multicast_delegate execution")
{
    auto counter{ 0l };

    const auto increment_counter{ [&counter]
    {
        counter++;
    } };
    const auto decrement_counter{ [&counter]
    {
        counter--;
    } };

    SECTION("execution of unbound _delegate")
    {
        REQUIRE_FALSE(he::multicast_delegate{}.execute());
    }

    SECTION("execution of bound _delegate")
    {
        auto delegate{ he::multicast_delegate{} };

        delegate.bind(increment_counter);

        REQUIRE(delegate.execute());
    }

    SECTION("execution of multiple bounds")
    {
        auto delegate{ he::multicast_delegate{} };

        delegate.bind(increment_counter);
        delegate.bind(increment_counter);
        delegate.bind(decrement_counter);

        REQUIRE(delegate.execute());

        REQUIRE(counter == 1);
    }

    SECTION("bind order preserved during execution")
    {
        auto delegate{ he::multicast_delegate{} };

        auto sample{ std::string{} };

        delegate.bind(
            [&sample]
            {
                sample += "c";
            });
        delegate.bind(
            [&sample]
            {
                sample += "d";
            });
        delegate.bind(
            [&sample]
            {
                sample += "z";
            });
        delegate.bind(
            [&sample]
            {
                sample += "y";
            });

        REQUIRE(delegate.execute());

        REQUIRE(sample == "cdzy");
    }

    SECTION("execution prevented if lifetime owner is dead")
    {
        struct owner final
        {
        };

        auto first_lifetime_owner{ std::make_shared<owner>() };
        auto second_lifetime_owner{ std::make_shared<owner>() };

        auto delegate{ he::multicast_delegate{} };

        delegate.bind(std::weak_ptr{ first_lifetime_owner }, increment_counter);
        delegate.bind(std::weak_ptr{ second_lifetime_owner }, increment_counter);

        REQUIRE(delegate.is_bound());

        REQUIRE(delegate.execute());

        REQUIRE(counter == 2);

        second_lifetime_owner.reset();

        REQUIRE(delegate.execute());

        REQUIRE(counter == 3);
    }

}
