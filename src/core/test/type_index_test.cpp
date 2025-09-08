#include "core/type_index.hpp"

#include <catch2/catch_test_macros.hpp>

#include <memory>


TEST_CASE("sequencial index generation", "[type_index]")
{
	REQUIRE(he::type_index<int>::value() == 0);
	REQUIRE(he::type_index<float>::value() == 1);
	REQUIRE(he::type_index<double>::value() == 2);
	REQUIRE(he::type_index<std::size_t>::value() == 3);
	REQUIRE(he::type_index<std::unique_ptr<void>>::value() == 4);

	REQUIRE(he::type_index<std::unique_ptr<void>>::value() == 4);
	REQUIRE(he::type_index<std::size_t>::value() == 3);
	REQUIRE(he::type_index<double>::value() == 2);
	REQUIRE(he::type_index<float>::value() == 1);
	REQUIRE(he::type_index<int>::value() == 0);

	REQUIRE(he::type_index<const int>::value() == 5);
	REQUIRE(he::type_index<int&>::value() == 6);
	REQUIRE(he::type_index<int&&>::value() == 7);
	REQUIRE(he::type_index<const int&>::value() == 8);
	REQUIRE(he::type_index<const int&&>::value() == 9);

	REQUIRE(he::type_index<int*>::value() == 10);
	REQUIRE(he::type_index<const int*>::value() == 11);
	REQUIRE(he::type_index<const int*&>::value() == 12);
	REQUIRE(he::type_index<const int**>::value() == 13);
	REQUIRE(he::type_index<const int[]>::value() == 14);
}
