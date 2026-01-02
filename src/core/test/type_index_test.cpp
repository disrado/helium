#include "core/type_index.hpp"

#include "catch_amalgamated.hpp"


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
