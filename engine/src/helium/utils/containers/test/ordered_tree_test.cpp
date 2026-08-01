#include "helium/utils/containers/ordered_tree.hpp"

#include <catch2/catch_test_macros.hpp>

#include <ranges>
#include <string>
#include <vector>

#include "utils/type_traits/type_id.hpp"


using tree_t = he::ordered_tree<he::type_index_t, int>;


TEST_CASE("tree traits")
{
    using tree_t = he::ordered_tree<he::type_index_t, std::string>;

    STATIC_CHECK(std::ranges::common_range<tree_t>);
    STATIC_CHECK(std::ranges::bidirectional_range<tree_t>);
    STATIC_CHECK(std::ranges::viewable_range<tree_t>);
    STATIC_CHECK(std::ranges::sized_range<tree_t>);

    STATIC_CHECK(std::ranges::range<const tree_t>);
    STATIC_CHECK(std::ranges::bidirectional_range<const tree_t>);
    STATIC_CHECK(std::ranges::sized_range<const tree_t>);
}

TEST_CASE("tree iterator traits")
{
    STATIC_CHECK(std::bidirectional_iterator<tree_t::iterator>);
    STATIC_CHECK(std::bidirectional_iterator<tree_t::const_iterator>);

    STATIC_CHECK(std::sentinel_for<std::default_sentinel_t, tree_t::iterator>);
    STATIC_CHECK(std::sentinel_for<std::default_sentinel_t, tree_t::const_iterator>);

    STATIC_CHECK(std::semiregular<tree_t::iterator>);
    STATIC_CHECK(std::semiregular<tree_t::const_iterator>);
}

TEST_CASE("tree construction")
{
    SECTION("constructs with a single root")
    {
        auto tree{ tree_t{ he::type_index<int>(), 42 } };

        REQUIRE(tree.size() == 1);
        REQUIRE(tree.is_root(he::type_index<int>()));
        REQUIRE(tree.find(he::type_index<int>()) != nullptr);
        REQUIRE(*tree.find(he::type_index<int>()) == 42);
    }

    SECTION("copy construction performs a deep copy")
    {
        auto original{ tree_t{ he::type_index<int>(), 0 } };
        REQUIRE(original.emplace(he::type_index<int>(), he::type_index<bool>(), 1));

        auto copy{ original };

        REQUIRE(copy.size() == original.size());
        REQUIRE(copy.contains(he::type_index<bool>()));

        REQUIRE(copy.emplace(he::type_index<bool>(), he::type_index<float>(), 2));
        REQUIRE(copy.size() != original.size());
        REQUIRE_FALSE(original.contains(he::type_index<float>()));

        *copy.find(he::type_index<bool>()) = 99;
        REQUIRE(*original.find(he::type_index<bool>()) == 1);
    }

    SECTION("move construction transfers ownership")
    {
        auto source{ tree_t{ he::type_index<int>(), 0 } };
        REQUIRE(source.emplace(he::type_index<int>(), he::type_index<bool>(), 1));

        auto destination{ std::move(source) };

        REQUIRE(destination.size() == 2);
        REQUIRE(destination.contains(he::type_index<bool>()));
        REQUIRE(*destination.find(he::type_index<bool>()) == 1);
    }

    SECTION("copy assignment replaces contents and stays independent")
    {
        auto lhs{ tree_t{ he::type_index<int>(), 0 } };
        auto rhs{ tree_t{ he::type_index<double>(), 7 } };
        REQUIRE(rhs.emplace(he::type_index<double>(), he::type_index<char>(), 9));

        lhs = rhs;

        REQUIRE(lhs.size() == rhs.size());
        REQUIRE(lhs.is_root(he::type_index<double>()));
        REQUIRE(lhs.contains(he::type_index<char>()));

        REQUIRE(lhs.emplace(he::type_index<char>(), he::type_index<short>(), 1));
        REQUIRE_FALSE(rhs.contains(he::type_index<short>()));
    }

    SECTION("self assignment leaves the tree unchanged")
    {
        auto tree{ tree_t{ he::type_index<int>(), 0 } };
        REQUIRE(tree.emplace(he::type_index<int>(), he::type_index<bool>(), 1));

        auto& alias{ tree };
        tree = alias;

        REQUIRE(tree.size() == 2);
        REQUIRE(tree.contains(he::type_index<bool>()));
        REQUIRE(*tree.find(he::type_index<bool>()) == 1);
    }

    SECTION("move assignment transfers ownership")
    {
        auto lhs{ tree_t{ he::type_index<int>(), 0 } };
        auto rhs{ tree_t{ he::type_index<double>(), 7 } };
        REQUIRE(rhs.emplace(he::type_index<double>(), he::type_index<char>(), 9));

        lhs = std::move(rhs);

        REQUIRE(lhs.size() == 2);
        REQUIRE(lhs.is_root(he::type_index<double>()));
        REQUIRE(lhs.contains(he::type_index<char>()));
        REQUIRE(*lhs.find(he::type_index<char>()) == 9);
    }

    SECTION("swap exchanges contents")
    {
        auto a{ tree_t{ he::type_index<int>(), 1 } };
        auto b{ tree_t{ he::type_index<double>(), 2 } };
        REQUIRE(b.emplace(he::type_index<double>(), he::type_index<bool>(), 3));

        a.swap(b);

        REQUIRE(a.is_root(he::type_index<double>()));
        REQUIRE(a.contains(he::type_index<bool>()));
        REQUIRE(b.is_root(he::type_index<int>()));
        REQUIRE(b.size() == 1);
    }
}

TEST_CASE("tree emplace")
{
    auto tree{ tree_t{ he::type_index<int>(), 0 } };

    SECTION("succeeds under an existing parent and increments size")
    {
        REQUIRE(tree.size() == 1);

        REQUIRE(tree.emplace(he::type_index<int>(), he::type_index<bool>(), 1));
        REQUIRE(tree.size() == 2);
        REQUIRE(tree.contains(he::type_index<bool>()));
    }

    SECTION("fails when the parent key does not exist")
    {
        REQUIRE_FALSE(tree.emplace(he::type_index<double>(), he::type_index<bool>(), 1));
        REQUIRE(tree.size() == 1);
    }

    SECTION("fails when the key already exists under the same parent")
    {
        REQUIRE(tree.emplace(he::type_index<int>(), he::type_index<bool>(), 1));

        REQUIRE_FALSE(tree.emplace(he::type_index<int>(), he::type_index<bool>(), 2));
        REQUIRE(tree.size() == 2);
    }

    SECTION("fails when the key already exists under a different parent")
    {
        REQUIRE(tree.emplace(he::type_index<int>(), he::type_index<bool>(), 1));
        REQUIRE(tree.emplace(he::type_index<int>(), he::type_index<double>(), 2));

        REQUIRE_FALSE(tree.emplace(he::type_index<double>(), he::type_index<bool>(), 3));
        REQUIRE(tree.size() == 3);
    }
}

TEST_CASE("tree erase")
{
    auto tree{ tree_t{ he::type_index<int>(), 0 } };
    REQUIRE(tree.emplace(he::type_index<int>(), he::type_index<bool>(), 1));
    REQUIRE(tree.emplace(he::type_index<bool>(), he::type_index<float>(), 2));
    REQUIRE(tree.emplace(he::type_index<int>(), he::type_index<double>(), 3));

    REQUIRE(tree.size() == 4);

    SECTION("removes a leaf node")
    {
        REQUIRE(tree.erase(he::type_index<float>()));
        REQUIRE(tree.size() == 3);
        REQUIRE_FALSE(tree.contains(he::type_index<float>()));
        REQUIRE(tree.contains(he::type_index<bool>()));
    }

    SECTION("removes a whole subtree and adjusts size for every descendant")
    {
        REQUIRE(tree.erase(he::type_index<bool>()));
        REQUIRE(tree.size() == 2);
        REQUIRE_FALSE(tree.contains(he::type_index<bool>()));
        REQUIRE_FALSE(tree.contains(he::type_index<float>()));
        REQUIRE(tree.contains(he::type_index<double>()));
    }

    SECTION("fails for a key that does not exist")
    {
        REQUIRE_FALSE(tree.erase(he::type_index<char>()));
        REQUIRE(tree.size() == 4);
    }

    SECTION("fails for the root")
    {
        REQUIRE_FALSE(tree.erase(he::type_index<int>()));
        REQUIRE(tree.size() == 4);
        REQUIRE(tree.is_root(he::type_index<int>()));
    }

    SECTION("forward iteration order stays correct after erasing a leaf")
    {
        REQUIRE(tree.erase(he::type_index<float>()));

        auto keys{ std::vector<he::type_index_t>{} };
        for (const auto& [key, value] : tree)
        {
            keys.push_back(key);
        }

        REQUIRE(keys == std::vector<he::type_index_t>{
            he::type_index<int>(),
            he::type_index<bool>(),
            he::type_index<double>()
        });
    }

    SECTION("forward iteration order stays correct after erasing a subtree")
    {
        REQUIRE(tree.erase(he::type_index<bool>()));

        auto keys{ std::vector<he::type_index_t>{} };
        for (const auto& [key, value] : tree)
        {
            keys.push_back(key);
        }

        REQUIRE(keys == std::vector<he::type_index_t>{
            he::type_index<int>(),
            he::type_index<double>()
        });
    }

    SECTION("erased key can be re-emplaced")
    {
        REQUIRE(tree.erase(he::type_index<float>()));
        REQUIRE_FALSE(tree.contains(he::type_index<float>()));

        REQUIRE(tree.emplace(he::type_index<bool>(), he::type_index<float>(), 42));
        REQUIRE(tree.contains(he::type_index<float>()));
        REQUIRE(*tree.find(he::type_index<float>()) == 42);
    }
}

TEST_CASE("tree clear")
{
    auto tree{ tree_t{ he::type_index<int>(), 42 } };
    REQUIRE(tree.emplace(he::type_index<int>(), he::type_index<bool>(), 1));
    REQUIRE(tree.emplace(he::type_index<bool>(), he::type_index<float>(), 2));

    tree.clear();

    REQUIRE(tree.size() == 1);
    REQUIRE(tree.is_root(he::type_index<int>()));
    REQUIRE(*tree.find(he::type_index<int>()) == 42);
    REQUIRE_FALSE(tree.contains(he::type_index<bool>()));
    REQUIRE_FALSE(tree.contains(he::type_index<float>()));

    REQUIRE(tree.emplace(he::type_index<int>(), he::type_index<double>(), 5));
    REQUIRE(tree.size() == 2);
}

TEST_CASE("tree find and contains")
{
    auto tree{ tree_t{ he::type_index<int>(), 0 } };
    REQUIRE(tree.emplace(he::type_index<int>(), he::type_index<bool>(), 7));

    SECTION("find locates an existing key")
    {
        auto* value{ tree.find(he::type_index<bool>()) };

        REQUIRE(value != nullptr);
        REQUIRE(*value == 7);
    }

    SECTION("find returns nullptr for a missing key")
    {
        REQUIRE(tree.find(he::type_index<double>()) == nullptr);
    }

    SECTION("find allows mutation through the returned pointer")
    {
        *tree.find(he::type_index<bool>()) = 100;

        REQUIRE(*tree.find(he::type_index<bool>()) == 100);
    }

    SECTION("const find works on a const tree")
    {
        const auto& const_tree{ tree };

        REQUIRE(const_tree.find(he::type_index<bool>()) != nullptr);
        REQUIRE(*const_tree.find(he::type_index<bool>()) == 7);
        REQUIRE(const_tree.find(he::type_index<double>()) == nullptr);
    }

    SECTION("contains reflects find")
    {
        REQUIRE(tree.contains(he::type_index<bool>()));
        REQUIRE_FALSE(tree.contains(he::type_index<double>()));
    }
}

TEST_CASE("tree relationship queries")
{
    auto tree{ tree_t{ he::type_index<int>(), 0 } };
    REQUIRE(tree.emplace(he::type_index<int>(), he::type_index<bool>(), 1));
    REQUIRE(tree.emplace(he::type_index<bool>(), he::type_index<float>(), 2));
    REQUIRE(tree.emplace(he::type_index<int>(), he::type_index<double>(), 3));
    REQUIRE(tree.emplace(he::type_index<double>(), he::type_index<std::string>(), 4));

    const auto root_key{ he::type_index<int>() };
    const auto bool_key{ he::type_index<bool>() };
    const auto float_key{ he::type_index<float>() };
    const auto double_key{ he::type_index<double>() };
    const auto string_key{ he::type_index<std::string>() };

    SECTION("is_root")
    {
        REQUIRE(tree.is_root(root_key));
        REQUIRE_FALSE(tree.is_root(bool_key));
    }

    SECTION("is_child_of")
    {
        REQUIRE(tree.is_child_of(bool_key, root_key));
        REQUIRE(tree.is_child_of(float_key, bool_key));
        REQUIRE_FALSE(tree.is_child_of(float_key, root_key));
        REQUIRE_FALSE(tree.is_child_of(float_key, double_key));
    }

    SECTION("is_parent_of")
    {
        REQUIRE(tree.is_parent_of(root_key, bool_key));
        REQUIRE(tree.is_parent_of(bool_key, float_key));
        REQUIRE_FALSE(tree.is_parent_of(root_key, float_key));
        REQUIRE_FALSE(tree.is_parent_of(double_key, float_key));
    }

    SECTION("is_descendant_of")
    {
        REQUIRE(tree.is_descendant_of(float_key, bool_key));
        REQUIRE(tree.is_descendant_of(float_key, root_key));
        REQUIRE(tree.is_descendant_of(string_key, root_key));

        REQUIRE_FALSE(tree.is_descendant_of(string_key, bool_key));
        REQUIRE_FALSE(tree.is_descendant_of(root_key, bool_key));
        REQUIRE_FALSE(tree.is_descendant_of(bool_key, bool_key));
    }

    SECTION("is_ancestor_of")
    {
        REQUIRE(tree.is_ancestor_of(bool_key, float_key));
        REQUIRE(tree.is_ancestor_of(root_key, float_key));
        REQUIRE(tree.is_ancestor_of(root_key, string_key));

        REQUIRE_FALSE(tree.is_ancestor_of(bool_key, string_key));
        REQUIRE_FALSE(tree.is_ancestor_of(bool_key, root_key));
        REQUIRE_FALSE(tree.is_ancestor_of(bool_key, bool_key));
    }
}

TEST_CASE("tree forward iteration")
{
    auto tree{ tree_t{ he::type_index<int>(), 0 } };
    REQUIRE(tree.emplace(he::type_index<int>(), he::type_index<bool>(), 1));
    REQUIRE(tree.emplace(he::type_index<bool>(), he::type_index<float>(), 2));
    REQUIRE(tree.emplace(he::type_index<int>(), he::type_index<double>(), 3));
    REQUIRE(tree.emplace(he::type_index<double>(), he::type_index<std::string>(), 4));

    const auto expected_keys{ std::vector<he::type_index_t>{
        he::type_index<int>(),
        he::type_index<bool>(),
        he::type_index<float>(),
        he::type_index<double>(),
        he::type_index<std::string>()
    } };

    const auto expected_values{ std::vector<int>{ 0, 1, 2, 3, 4 } };

    SECTION("begin/end visits nodes in pre-order")
    {
        auto keys{ std::vector<he::type_index_t>{} };
        auto values{ std::vector<int>{} };

        for (auto it{ tree.begin() }; it != tree.end(); ++it)
        {
            keys.push_back(it->first);
            values.push_back(it->second);
        }

        REQUIRE(keys == expected_keys);
        REQUIRE(values == expected_values);
    }

    SECTION("range-based for visits nodes in pre-order")
    {
        auto keys{ std::vector<he::type_index_t>{} };

        for (const auto& [key, value] : tree)
        {
            keys.push_back(key);
        }

        REQUIRE(keys == expected_keys);
    }

    SECTION("const begin/end and cbegin/cend match pre-order")
    {
        const auto& const_tree{ tree };

        auto const_begin_keys{ std::vector<he::type_index_t>{} };
        for (auto it{ const_tree.begin() }; it != const_tree.end(); ++it)
        {
            const_begin_keys.push_back(it->first);
        }
        REQUIRE(const_begin_keys == expected_keys);

        auto cbegin_keys{ std::vector<he::type_index_t>{} };
        for (auto it{ tree.cbegin() }; it != tree.cend(); ++it)
        {
            cbegin_keys.push_back(it->first);
        }
        REQUIRE(cbegin_keys == expected_keys);

        auto const_range_keys{ std::vector<he::type_index_t>{} };
        for (const auto& [key, value] : const_tree)
        {
            const_range_keys.push_back(key);
        }
        REQUIRE(const_range_keys == expected_keys);
    }
}

TEST_CASE("tree reverse iteration")
{
    auto tree{ tree_t{ he::type_index<int>(), 0 } };
    REQUIRE(tree.emplace(he::type_index<int>(), he::type_index<bool>(), 1));
    REQUIRE(tree.emplace(he::type_index<bool>(), he::type_index<float>(), 2));
    REQUIRE(tree.emplace(he::type_index<int>(), he::type_index<double>(), 3));
    REQUIRE(tree.emplace(he::type_index<double>(), he::type_index<std::string>(), 4));

    const auto expected_reverse_keys{ std::vector<he::type_index_t>{
        he::type_index<std::string>(),
        he::type_index<double>(),
        he::type_index<float>(),
        he::type_index<bool>(),
        he::type_index<int>()
    } };

    SECTION("rbegin/rend visits nodes in reverse pre-order")
    {
        auto keys{ std::vector<he::type_index_t>{} };

        for (auto it{ tree.rbegin() }; it != tree.rend(); ++it)
        {
            keys.push_back(it->first);
        }

        REQUIRE(keys == expected_reverse_keys);
    }

    SECTION("crbegin/crend visits nodes in reverse pre-order")
    {
        auto keys{ std::vector<he::type_index_t>{} };

        for (auto it{ tree.crbegin() }; it != tree.crend(); ++it)
        {
            keys.push_back(it->first);
        }

        REQUIRE(keys == expected_reverse_keys);
    }

    SECTION("std::views::reverse matches manual reverse iteration")
    {
        auto keys{ std::vector<he::type_index_t>{} };

        for (const auto& [key, value] : tree | std::views::reverse)
        {
            keys.push_back(key);
        }

        REQUIRE(keys == expected_reverse_keys);
    }
}

TEST_CASE("tree with non-trivial value type")
{
    using tree_t = he::ordered_tree<he::type_index_t, std::string>;

    auto original{ tree_t{ he::type_index<int>(), std::string{ "root" } } };
    REQUIRE(original.emplace(he::type_index<int>(), he::type_index<bool>(), std::string{ "child" }));

    REQUIRE(original.size() == 2);
    REQUIRE(*original.find(he::type_index<int>()) == "root");
    REQUIRE(*original.find(he::type_index<bool>()) == "child");

    auto copy{ original };

    *copy.find(he::type_index<bool>()) = "mutated";
    REQUIRE(*copy.find(he::type_index<bool>()) == "mutated");
    REQUIRE(*original.find(he::type_index<bool>()) == "child");

    REQUIRE(copy.emplace(he::type_index<bool>(), he::type_index<double>(), std::string{ "grandchild" }));
    REQUIRE_FALSE(original.contains(he::type_index<double>()));
}
