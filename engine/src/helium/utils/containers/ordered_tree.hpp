#pragma once

#include <algorithm>
#include <functional>
#include <map>
#include <memory>


namespace he
{
/*
 * rooted tree with unique keys which preserves children elements order
 * traversal starts from root and ends with the rights-most element iterating over children in a left-to-right manner
 */
template <typename key_t, typename value_t>
class ordered_tree
{
public:
    using key_type = key_t;
    using value_type = value_t;

private:
    struct node final
    {
        key_t key;
        value_t value;

        node* parent;
        std::vector<std::unique_ptr<node>> children;
    };

private:
    template <typename it_value_t, typename it_ptr_t, typename it_ref_t>
    struct iterator_base final
    {
    public:
        using value_type = it_value_t;
        using pointer = it_ptr_t;
        using reference = it_ref_t;
        using difference_type = std::ptrdiff_t;
        using category = std::bidirectional_iterator_tag;

    public:
        iterator_base() = default;

        explicit iterator_base(node* tree_node);

    public:
        auto operator *() const noexcept -> reference;
        auto operator ->() const noexcept -> pointer;

        auto operator ++() noexcept -> iterator_base&;
        auto operator ++(int) noexcept -> iterator_base;

        auto operator --() noexcept -> iterator_base&;
        auto operator --(int) noexcept -> iterator_base;

        auto operator ==(const iterator_base&) const noexcept -> bool = default;
        auto operator ==(std::default_sentinel_t) const noexcept -> bool;

        operator bool() const noexcept;

    private:
        node* _node{ nullptr };
    };

public:
    using iterator = iterator_base<value_type, value_type*, value_type&>;
    using const_iterator = iterator_base<value_type, const value_type*, const value_type&>;

public:
    explicit ordered_tree(key_type root_key, value_t root_value);

    // deep copy ctor/assignment
    // move ctor/assignment

public:
    auto emplace(const key_type& parent_key, const key_type& key, auto&&... args) -> bool;

    auto find(const key_type& key) -> value_type*;

    auto contains(const key_type& key) -> bool;

    auto size() const noexcept -> std::size_t;

    auto is_root(const key_type& key) const -> bool;

    auto is_child_of(const key_type& key, const key_type& parent_key) const -> bool;
    auto is_parent_of(const key_type& key, const key_type& child_key) const -> bool;

    auto is_descendant_of(const key_type& child_key, const key_type& parent_key) const -> bool;
    auto is_ancestor_of(const key_type& parent_key, const key_type& child_key) const -> bool;

    auto begin() noexcept -> iterator;
    auto end() noexcept -> std::default_sentinel_t;

    auto begin() const noexcept -> const_iterator;
    auto end() const noexcept -> std::default_sentinel_t;

    // rbegin
    // rend

    // crbegin
    // crend

private:
    auto find_node(node& root, const key_type& target_key) const -> node*;

private:
    std::unique_ptr<node> _root;
    std::size_t _size{ 0 };
};

template <typename key_t, typename value_t>
ordered_tree<key_t, value_t>::ordered_tree(key_t root_key, value_t root_value)
    : _root{ std::make_unique<node>(
        node{
            .key = std::move(root_key),
            .value = std::move(root_value),
            .parent = nullptr,
            .children = {}
        }) }
, _size{ 1 }
{
}

template <typename key_t, typename value_t>
auto ordered_tree<key_t, value_t>::emplace(const key_type& parent_key, const key_type& key, auto&&... args) -> bool
{
    auto* parent{ find_node(*_root, parent_key) };
    if (!parent)
    {
        return false;
    }

    auto self{ std::ranges::find_if(parent->children, [key] (const auto& child) { return child->key == key; }) };
    if (self != std::ranges::end(parent->children))
    {
        return false;
    }

    parent->children.emplace_back(
        std::make_unique<node>(
            node{
                .key = key,
                .value = value_type{ std::forward<decltype(args)>(args)... },
                .parent = parent,
                .children = {}
            }
        ));

    _size++;

    return true;
}

template <typename key_t, typename value_t>
auto ordered_tree<key_t, value_t>::find(const key_type& key) -> value_type*
{
    for (const auto& node : *this)
    {
        if (node.key == key)
        {
            return &node;
        }
    }

    return nullptr;
}

template <typename key_t, typename value_t>
auto ordered_tree<key_t, value_t>::contains(const key_type& key) -> bool
{
    return find(key);
}

template <typename key_t, typename value_t>
auto ordered_tree<key_t, value_t>::size() const noexcept -> std::size_t
{
    return _size;
}

template <typename key_t, typename value_t>
auto ordered_tree<key_t, value_t>::is_root(const key_type& key) const -> bool
{
    return _root->key == key;
}

template <typename key_t, typename value_t>
auto ordered_tree<key_t, value_t>::is_child_of(const key_type& key, const key_type& parent_key) const -> bool
{
    if (const auto* found{ find_node(*_root, key) })
    {
        return found->parent && found->parent->key == parent_key;
    }

    return false;
}

template <typename key_t, typename value_t>
auto ordered_tree<key_t, value_t>::is_parent_of(const key_type& key, const key_type& child_key) const -> bool
{
    if (const auto* found{ find_node(*_root, key) })
    {
        return std::ranges::find_if(found->children, [child_key] (auto& node) { return node->key == child_key; });
    }

    return false;
}

template <typename key_t, typename value_t>
auto ordered_tree<key_t, value_t>::is_descendant_of(const key_type& key, const key_type& descendant_key) const -> bool
{
    if (key == descendant_key)
    {
        return false;
    }

    if (const auto* found{ find_node(*_root, key) })
    {
        for (auto it{ const_iterator{ found } }; it; ++it)
        {
            if (it->key == descendant_key)
            {
                return true;
            }
        }
    }

    return false;
}

template <typename key_t, typename value_t>
auto ordered_tree<key_t, value_t>::is_ancestor_of(const key_type& key, const key_type& ancestor_key) const -> bool
{
    if (key == ancestor_key)
    {
        return false;
    }

    if (const auto* found{ find_node(*_root, key) })
    {
        for (auto it{ const_iterator{ found } }; it; --it)
        {
            if (it->key == ancestor_key)
            {
                return true;
            }
        }
    }

    return false;
}

template <typename key_t, typename value_t>
auto ordered_tree<key_t, value_t>::begin() noexcept -> iterator
{
    return _root ? iterator{ _root.get() } : iterator{};
}

template <typename key_t, typename value_t>
auto ordered_tree<key_t, value_t>::begin() const noexcept -> const_iterator
{
    return _root ? const_iterator{ _root.get() } : const_iterator{};
}

template <typename key_t, typename value_t>
auto ordered_tree<key_t, value_t>::end() noexcept -> std::default_sentinel_t
{
    return std::default_sentinel_t{};
}

template <typename key_t, typename value_t>
auto ordered_tree<key_t, value_t>::end() const noexcept -> std::default_sentinel_t
{
    return std::default_sentinel_t{};
}

template <typename key_t, typename value_t>
auto ordered_tree<key_t, value_t>::find_node(node& root, const key_type& target_key) const -> node*
{
    if (root.key == target_key)
    {
        return &root;
    }

    for (auto& node : root.children)
    {
        find_node(*node.get(), target_key);
    }

    return nullptr;
}

template <typename key_t, typename value_t>
template <typename it_value_t, typename it_ptr_t, typename it_ref_t>
ordered_tree<key_t, value_t>::iterator_base<it_value_t, it_ptr_t, it_ref_t>::iterator_base(node* tree_node)
    : _node{ tree_node }
{
}


template <typename key_t, typename value_t>
template <typename it_value_t, typename it_ptr_t, typename it_ref_t>
auto ordered_tree<key_t, value_t>::iterator_base<it_value_t, it_ptr_t, it_ref_t>::operator *() const noexcept -> reference
{
    return _node->value;
}

template <typename key_t, typename value_t>
template <typename it_value_t, typename it_ptr_t, typename it_ref_t>
auto ordered_tree<key_t, value_t>::iterator_base<it_value_t, it_ptr_t, it_ref_t>::operator ->() const noexcept -> pointer
{
    return &_node->value;
}

template <typename key_t, typename value_t>
template <typename it_value_t, typename it_ptr_t, typename it_ref_t>
auto ordered_tree<key_t, value_t>::iterator_base<it_value_t, it_ptr_t, it_ref_t>::operator ++() noexcept -> iterator_base&
{
    if (!_node)
    {
        return *this;
    }

    if (!_node->children.empty())
    {
        _node = _node->children.front().get();
        return *this;
    }

    if (!_node->parent)
    {
        _node = nullptr;
        return *this;
    }

    auto self{ std::ranges::find_if(_node->parent->children, [this] (auto& node) { return node.get() == _node; }) };
    if (*self != _node->parent->children.back())
    {
        _node = (++self)->get();
    }
    else
    {
        _node = nullptr;
    }

    return *this;
}

template <typename key_t, typename value_t>
template <typename it_value_t, typename it_ptr_t, typename it_ref_t>
auto ordered_tree<key_t, value_t>::iterator_base<it_value_t, it_ptr_t, it_ref_t>::operator ++(int) noexcept -> iterator_base
{
    auto tmp{ *this };

    operator++();

    return tmp;
}

template <typename key_t, typename value_t>
template <typename it_value_t, typename it_ptr_t, typename it_ref_t>
auto ordered_tree<key_t, value_t>::iterator_base<it_value_t, it_ptr_t, it_ref_t>::operator --() noexcept -> iterator_base&
{
    if (!_node)
    {
        return *this;
    }

    if (!_node->parent)
    {
        _node = nullptr;
        return *this;
    }

    auto self{ std::ranges::find_if(_node->parent->children, [this] (auto& node) { return node.get() == _node; }) };

    if (*self != _node->parent->children.front())
    {
        _node = (--self)->get();
    }
    else
    {
        _node = _node->parent;
    }

    return *this;
}

template <typename key_t, typename value_t>
template <typename it_value_t, typename it_ptr_t, typename it_ref_t>
auto ordered_tree<key_t, value_t>::iterator_base<it_value_t, it_ptr_t, it_ref_t>::operator --(int) noexcept -> iterator_base
{
    auto tmp{ *this };

    operator--();

    return tmp;
}

template <typename key_t, typename value_t>
template <typename it_value_t, typename it_ptr_t, typename it_ref_t>
auto ordered_tree<key_t, value_t>::iterator_base<it_value_t, it_ptr_t, it_ref_t>::operator ==(std::default_sentinel_t) const noexcept -> bool
{
    return !static_cast<bool>(_node);
}

template <typename key_t, typename value_t>
template <typename it_value_t, typename it_ptr_t, typename it_ref_t>
ordered_tree<key_t, value_t>::iterator_base<it_value_t, it_ptr_t, it_ref_t>::operator bool() const noexcept
{
    return static_cast<bool>(_node);
}

}
