#include "helium/utils/type_traits/type_name.hpp"

#include <catch2/catch_test_macros.hpp>


TEST_CASE("type name", "[type_name]")
{
	REQUIRE(he::type_name<int>() == he::type_name<int>());
	REQUIRE(he::type_name<std::string>() == he::type_name<std::string>());

	REQUIRE(he::type_name<int>() != he::type_name<std::string>());

	REQUIRE(he::type_name<int>() != he::type_name<int&>());
	REQUIRE(he::type_name<int>() != he::type_name<int&&>());

	REQUIRE(he::type_name<int>() != he::type_name<const int>());
	REQUIRE(he::type_name<int>() != he::type_name<volatile int>());

	REQUIRE(he::type_name<int>() != he::type_name<int*>());

	REQUIRE(he::type_name<int*>() != he::type_name<const int[]>());
}

TEST_CASE("type_name_of")
{
    static constexpr const auto str{ std::string_view{ "" } };
    const auto size{ std::size_t{ 0 } };

    REQUIRE(he::type_name_of(str) == he::type_name_of(str));

    REQUIRE(he::type_name_of(std::string{}) == he::type_name_of(std::string{}));

	REQUIRE(he::type_name_of(str) != he::type_name_of(size));

    REQUIRE(he::type_name_of(int{}) != he::type_name_of(bool{}));
}

TEST_CASE("name_of")
{
    static constexpr const auto str{ std::string_view{ "" } };
    const auto size{ std::size_t{ 0 } };

    REQUIRE(he::name_of(str) == he::name_of(str));

    REQUIRE(he::name_of(std::string{}) == he::name_of(std::string{}));

    REQUIRE(he::name_of(str) != he::name_of(size));

    REQUIRE(he::name_of(int{}) != he::name_of(bool{}));
}
