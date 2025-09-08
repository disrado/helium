#include "core/core_macro.hpp"
#include "core/type_name.hpp"

#include <catch2/catch_test_macros.hpp>


TEST_CASE("value type", "[type_name]")
{
	REQUIRE(he::type_name<int>::value() == std::string_view{ "int" });
}

TEST_CASE("const-qualified type", "[type_name]")
{
	REQUIRE(he::type_name<const int>::value() == std::string_view{ "const int" });
}

TEST_CASE("reference type", "[type_name]")
{
	#if GCC_COMPILER
		const auto name{ std::string_view{ "int&" } };
	#elif CLANG_COMPILER
		const auto name{ std::string_view{ "int &" } };
	#elif MSVC_COMPILER
		const auto name{ std::string_view{ "int&" } };
	#endif

	REQUIRE(he::type_name<int&>::value() == name);
}

TEST_CASE("pointer type", "[type_name]")
{
	#if GCC_COMPILER
		const auto name{std::string_view{ "int*" } };
	#elif CLANG_COMPILER
		const auto name{ std::string_view{ "int *" } };
	#elif MSVC_COMPILER
		const auto name{std::string_view{ "int*" } };
	#endif

	REQUIRE(he::type_name<int*>::value() == name);
}

TEST_CASE("type is not decayed", "[type_name]")
{
	#if GCC_COMPILER
		const auto name{std::string_view{ "int []" } };
	#elif CLANG_COMPILER
		const auto name{ std::string_view{ "int[]" } };
	#elif MSVC_COMPILER
		const auto name{std::string_view{ "int[]" } };
	#endif

	REQUIRE(he::type_name<int[]>::value() == name);
}
