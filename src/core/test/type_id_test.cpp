#include "core/type_id.hpp"

#include <catch2/catch_test_macros.hpp>


TEST_CASE("type id", "[type_id]")
{
	REQUIRE(he::type_id<int>().index() == he::type_id<int>().index());
	REQUIRE(he::type_id<int>().name() == he::type_id<int>().name());
	REQUIRE(he::type_id<int>() == he::type_id<int>());

	REQUIRE(he::type_id<std::string>().index() == he::type_id<std::string>().index());
	REQUIRE(he::type_id<std::string>().name() == he::type_id<std::string>().name());
	REQUIRE(he::type_id<std::string>() == he::type_id<std::string>());

	REQUIRE(he::type_id<int>().index() != he::type_id<std::size_t>().index());
	REQUIRE(he::type_id<int>().name() != he::type_id<std::size_t>().name());
	REQUIRE(he::type_id<int>() != he::type_id<std::size_t>());

	REQUIRE(he::type_id<int>() != he::type_id<int&>());
	REQUIRE(he::type_id<int>() != he::type_id<int&&>());

	REQUIRE(he::type_id<int>() != he::type_id<const int>());
	REQUIRE(he::type_id<int>() != he::type_id<volatile int>());

	REQUIRE(he::type_id<int>() != he::type_id<int*>());

	REQUIRE(he::type_id<int*>() != he::type_id<const int[]>());	
}