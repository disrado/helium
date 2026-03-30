#include "engine/utils/type_traits/type_index.hpp"

#include <catch2/catch_test_macros.hpp>


TEST_CASE("type index", "[type_index]")
{
    REQUIRE(he::type_index<int>::value() == he::type_index<int>::value());
    REQUIRE(he::type_index<std::string>::value() == he::type_index<std::string>::value());

    REQUIRE(he::type_index<int>::value() != he::type_index<std::string>::value());

    REQUIRE(he::type_index<decltype([]{})>::value() != he::type_index<decltype([]{})>::value());

    REQUIRE(he::type_index<int>::value() != he::type_index<int&>::value());
    REQUIRE(he::type_index<int>::value() != he::type_index<int&&>::value());

    REQUIRE(he::type_index<int>::value() != he::type_index<const int>::value());
    REQUIRE(he::type_index<int>::value() != he::type_index<volatile int>::value());

    REQUIRE(he::type_index<int>::value() != he::type_index<int*>::value());

    REQUIRE(he::type_index<int*>::value() != he::type_index<const int[]>::value());
}

TEST_CASE("type_index_of")
{
    REQUIRE(he::type_index_of(std::size_t{}) == he::type_index_of(std::size_t{}));

    REQUIRE(he::type_index_of(std::size_t{}) != he::type_index_of(int{}));

    REQUIRE(he::type_index_of(std::string{}) != he::type_index<int*>::value());
}