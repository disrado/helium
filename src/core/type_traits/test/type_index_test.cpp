#include "core/type_traits/type_index.hpp"

#include <catch2/catch_test_macros.hpp>


TEST_CASE("type index", "[type_index]")
{
    REQUIRE(he::type_index<int>() == he::type_index<int>());
    REQUIRE(he::type_index<std::string>() == he::type_index<std::string>());

    REQUIRE(he::type_index<int>() != he::type_index<std::string>());

    REQUIRE(he::type_index<decltype([]{})>() != he::type_index<decltype([]{})>());

    REQUIRE(he::type_index<int>() != he::type_index<int&>());
    REQUIRE(he::type_index<int>() != he::type_index<int&&>());

    REQUIRE(he::type_index<int>() != he::type_index<const int>());
    REQUIRE(he::type_index<int>() != he::type_index<volatile int>());

    REQUIRE(he::type_index<int>() != he::type_index<int*>());

    REQUIRE(he::type_index<int*>() != he::type_index<const int[]>());
}

TEST_CASE("type_index_of")
{
    REQUIRE(he::type_index_of(std::size_t{}) == he::type_index_of(std::size_t{}));

    REQUIRE(he::type_index_of(std::size_t{}) != he::type_index_of(int{}));

    REQUIRE(he::type_index_of(std::string{}) != he::type_index<int*>());
}