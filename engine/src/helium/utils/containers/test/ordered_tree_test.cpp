#include "helium/utils/containers/ordered_tree.hpp"

#include <catch2/catch_test_macros.hpp>

#include <ranges>

#include "utils/type_traits/type_id.hpp"


TEST_CASE("tree traits")
{
    using tree_t = he::ordered_tree<he::type_index_t, std::string>;

    STATIC_CHECK(std::ranges::bidirectional_range<tree_t>);
    STATIC_CHECK(std::ranges::viewable_range<tree_t>);
}

TEST_CASE("tree iterator traits")
{
    using tree_t = he::ordered_tree<he::type_index_t, int>;

    STATIC_CHECK(std::bidirectional_iterator<tree_t::iterator>);
    STATIC_CHECK(std::bidirectional_iterator<tree_t::const_iterator>);
}

TEST_CASE("tree interface")
{
    auto tree{ he::ordered_tree<he::type_index_t, int>{ he::type_index<int>(), 4 } };

    REQUIRE(tree.size() == 1);

    REQUIRE(tree.emplace(he::type_index<int>(), he::type_index<bool>(), int{ 8 }));
    REQUIRE(tree.size() == 2);

    REQUIRE(tree.emplace(he::type_index<int>(), he::type_index<double>(), int{ 22 }));
    REQUIRE(tree.size() == 3);

    REQUIRE_FALSE(tree.emplace(he::type_index<int>(), he::type_index<double>(), int{ 22 }));
    REQUIRE(tree.size() == 3);
}
