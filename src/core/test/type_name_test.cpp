#include "core/type_name.hpp"

#include "catch_amalgamated.hpp"


TEST_CASE("type name", "[type_name]")
{
	REQUIRE(he::type_name<int>::value() == he::type_name<int>::value());
	REQUIRE(he::type_name<std::string>::value() == he::type_name<std::string>::value());

	REQUIRE(he::type_name<int>::value() != he::type_name<std::string>::value());

	REQUIRE(he::type_name<decltype([]{})>::value() == he::type_name<decltype([]{})>::value());

	REQUIRE(he::type_name<int>::value() != he::type_name<int&>::value());
	REQUIRE(he::type_name<int>::value() != he::type_name<int&&>::value());

	REQUIRE(he::type_name<int>::value() != he::type_name<const int>::value());
	REQUIRE(he::type_name<int>::value() != he::type_name<volatile int>::value());

	REQUIRE(he::type_name<int>::value() != he::type_name<int*>::value());

	REQUIRE(he::type_name<int*>::value() != he::type_name<const int[]>::value());	
}
