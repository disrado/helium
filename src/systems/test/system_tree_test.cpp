#include "systems/system_tree.hpp"

#include <catch2/catch_test_macros.hpp>


namespace
{

class tagged final: public he::subsystem
{
public:
    explicit tagged(int tag)
        : tag{ tag }
    {
    }

    int tag;
};

class ticker final: public he::subsystem
{
public:
    auto tick(double dt) -> void override
    {
        ticks++;
        last_dt = dt;
    }

    int ticks{ 0 };
    double last_dt{ 0.0 };
};

class unregistered final: public he::subsystem
{
};

}


TEST_CASE("system_tree")
{
    using he::system_tree;

    using root = system_tree::root;

    SECTION("add/get/try_get/contains/remove")
    {
        REQUIRE_FALSE(system_tree::contains<tagged>());
        REQUIRE(system_tree::try_get<tagged>() == nullptr);

        auto& added{ system_tree::add<root, tagged>(42) };
        REQUIRE(added.tag == 42);

        REQUIRE(system_tree::contains<tagged>());

        auto& fetched{ system_tree::get<tagged>() };
        REQUIRE(&fetched == &added);

        auto* found{ system_tree::try_get<tagged>() };
        REQUIRE(found == &added);

        REQUIRE(system_tree::remove<tagged>());
        REQUIRE_FALSE(system_tree::contains<tagged>());
        REQUIRE_FALSE(system_tree::remove<tagged>());
    }

    SECTION("try_get on unregistered")
    {
        REQUIRE(system_tree::try_get<unregistered>() == nullptr);
    }

    SECTION("tick")
    {
        auto& sub{ system_tree::add<root, ticker>() };

        system_tree::instance().tick(0.25);

        REQUIRE(sub.ticks == 1);
        REQUIRE(sub.last_dt == 0.25);

        system_tree::remove<ticker>();
    }
}
