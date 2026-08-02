#pragma once

#include <algorithm>
#include <functional>
#include <memory>


namespace he
{

/*
 * rooted tree with unique keys which preserves children elements order
 * traversal starts from root and ends with the rights-most element iterating over children in a left-to-right manner
 */
template <typename key_t, typename value_t>
class ordered_tree final
{
public:
    using key_type = key_t;
    using value_type = value_t;

private:
    struct node final
    {
    public:
        using value_type = std::pair<ordered_tree::key_type, ordered_tree::value_type>;

    public:
        node* parent;
        std::vector<std::unique_ptr<node>> children;

        value_type value;
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

        explicit iterator_base(node* root, node* tree_node);

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
        node* _root{ nullptr };
        node* _node{ nullptr };
    };

public:
    using iterator = iterator_base<typename node::value_type, typename node::value_type*, typename node::value_type&>;
    using const_iterator = iterator_base<typename node::value_type, const typename node::value_type*, const typename node::value_type&>;

public:
    explicit ordered_tree(key_type root_key, value_type root_value);

    ordered_tree(const ordered_tree& other);
    ordered_tree(ordered_tree&& other) noexcept;

    auto operator =(ordered_tree other) -> ordered_tree&;

public:
    auto emplace(const key_type& parent_key, const key_type& key, auto&&... args) -> bool;

    auto erase(const key_type& key) -> bool;

    auto clear() -> void;

    auto find(const key_type& key) -> value_type*;

    auto find(const key_type& key) const -> const value_type*;

    auto contains(const key_type& key) const -> bool;

    auto size() const noexcept -> std::size_t;

    auto is_root(const key_type& key) const -> bool;

    auto is_child_of(const key_type& key, const key_type& parent_key) const -> bool;
    auto is_parent_of(const key_type& key, const key_type& child_key) const -> bool;

    auto is_descendant_of(const key_type& child_key, const key_type& parent_key) const -> bool;
    auto is_ancestor_of(const key_type& parent_key, const key_type& child_key) const -> bool;

    auto swap(ordered_tree& other) noexcept -> void;

    auto begin() noexcept -> iterator;
    auto end() noexcept -> iterator;

    auto begin() const noexcept -> const_iterator;
    auto end() const noexcept -> const_iterator;

    auto cbegin() const noexcept -> const_iterator;
    auto cend() const noexcept -> const_iterator;

    auto rbegin() noexcept -> std::reverse_iterator<iterator>;
    auto rend() noexcept -> std::reverse_iterator<iterator>;

    auto crbegin() noexcept -> std::reverse_iterator<const_iterator>;
    auto crend() noexcept -> std::reverse_iterator<const_iterator>;

private:
    auto find_node(node& root, const key_type& target_key) const -> node*;

    static auto get_last_node(node& root) -> node&;

    static auto count_descendants(const node& root) -> std::size_t;

    static auto copy_tree(const node& from, node* parent) -> std::unique_ptr<node>;

private:
    std::unique_ptr<node> _root;
    std::size_t _size{ 0 };
};

template <typename k, typename v>
ordered_tree<k, v>::ordered_tree(key_type root_key, value_type root_value)
    : _root{ std::make_unique<node>(
        node{
            .parent = nullptr,
            .children = {},
            .value = std::pair{ std::move(root_key), std::move(root_value) }
        }) }
    , _size{ 1 }
{
}

template <typename k, typename v>
ordered_tree<k, v>::ordered_tree([[maybe_unused]] const ordered_tree& other)
    : _root{ copy_tree(*other._root, nullptr) }
    , _size{ other._size }
{
}

template <typename k, typename v>
ordered_tree<k, v>::ordered_tree(ordered_tree&& other) noexcept
{
    swap(other);
}

template <typename k, typename v>
auto ordered_tree<k, v>::operator =(ordered_tree other) -> ordered_tree&
{
    swap(other);

    return *this;
}

template <typename k, typename v>
auto ordered_tree<k, v>::emplace(const key_type& parent_key, const key_type& key, auto&&... args) -> bool
{
    if (find_node(*_root, key))
    {
        return false;
    }

    auto* parent{ find_node(*_root, parent_key) };
    if (!parent)
    {
        return false;
    }

    parent->children.emplace_back(
        std::make_unique<node>(
            node{
                .parent = parent,
                .children = {},
                .value = std::pair{ key, value_type{ std::forward<decltype(args)>(args)... } }
            }
        ));

    _size++;

    return true;
}

template <typename k, typename v>
auto ordered_tree<k, v>::erase(const key_type& key) -> bool
{
    if (auto* found{ find_node(*_root, key) })
    {
        if (!found->parent)
        {
            return false;
        }

        _size -= 1 + count_descendants(*found);

        std::erase_if(found->parent->children, [key] (const auto& child) { return child->value.first == key; });

        return true;
    }

    return false;
}

template <typename k, typename v>
auto ordered_tree<k, v>::clear() -> void
{
    _root->children.clear();

    _size = 1;
}

template <typename k, typename v>
auto ordered_tree<k, v>::find(const key_type& key) -> value_type*
{
    for (auto& [node_key, node_value]: *this)
    {
        if (node_key == key)
        {
            return &node_value;
        }
    }

    return nullptr;
}

template <typename k, typename v>
auto ordered_tree<k, v>::find(const key_type& key) const -> const value_type*
{
    return const_cast<ordered_tree<k, v>*>(this)->find(key);
}

template <typename k, typename v>
auto ordered_tree<k, v>::contains(const key_type& key) const -> bool
{
    return find(key);
}

template <typename k, typename v>
auto ordered_tree<k, v>::size() const noexcept -> std::size_t
{
    return _size;
}

template <typename k, typename v>
auto ordered_tree<k, v>::is_root(const key_type& key) const -> bool
{
    return _root->value.first == key;
}

template <typename k, typename v>
auto ordered_tree<k, v>::is_child_of(const key_type& key, const key_type& parent_key) const -> bool
{
    if (const auto* found{ find_node(const_cast<node&>(*_root), key) })
    {
        return found->parent && found->parent->value.first == parent_key;
    }

    return false;
}

template <typename k, typename v>
auto ordered_tree<k, v>::is_parent_of(const key_type& key, const key_type& child_key) const -> bool
{
    if (const auto* found{ find_node(const_cast<node&>(*_root), key) })
    {
        auto found_child{ std::ranges::find_if(found->children, [child_key] (auto& node) { return node->value.first == child_key; }) };
        return found_child != std::ranges::end(found->children);
    }

    return false;
}

template <typename k, typename v>
auto ordered_tree<k, v>::is_descendant_of(const key_type& key, const key_type& descendant_key) const -> bool
{
    if (key == descendant_key)
    {
        return false;
    }

    if (const auto* found{ find_node(const_cast<node&>(*_root), descendant_key) })
    {
        for (const auto& child: found->children)
        {
            if (find_node(*child, key))
            {
                return true;
            }
        }
    }

    return false;
}

template <typename k, typename v>
auto ordered_tree<k, v>::is_ancestor_of(const key_type& key, const key_type& ancestor_key) const -> bool
{
    if (key == ancestor_key)
    {
        return false;
    }

    if (const auto* found{ find_node(const_cast<node&>(*_root), ancestor_key) })
    {
        for (auto* ancestor{ found->parent }; ancestor; ancestor = ancestor->parent)
        {
            if (ancestor->value.first == key)
            {
                return true;
            }
        }
    }

    return false;
}

template <typename k, typename v>
auto ordered_tree<k, v>::swap(ordered_tree& other) noexcept -> void
{
    std::swap(_root, other._root);
    std::swap(_size, other._size);
}

template <typename k, typename v>
auto ordered_tree<k, v>::begin() noexcept -> iterator
{
    return iterator{ _root.get(), _root.get() };
}

template <typename k, typename v>
auto ordered_tree<k, v>::end() noexcept -> iterator
{
    return iterator{ _root.get(), nullptr };
}

template <typename k, typename v>
auto ordered_tree<k, v>::begin() const noexcept -> const_iterator
{
    return const_iterator{ _root.get(), _root.get() };
}

template <typename k, typename v>
auto ordered_tree<k, v>::end() const noexcept -> const_iterator
{
    return const_iterator{ _root.get(), nullptr };
}

template <typename k, typename v>
auto ordered_tree<k, v>::cbegin() const noexcept -> const_iterator
{
    return const_iterator{ _root.get(), _root.get() };
}

template <typename k, typename v>
auto ordered_tree<k, v>::cend() const noexcept -> const_iterator
{
    return const_iterator{ _root.get(), nullptr };
}

template <typename k, typename v>
auto ordered_tree<k, v>::rbegin() noexcept -> std::reverse_iterator<iterator>
{
    return std::reverse_iterator<iterator>(end());
}

template <typename k, typename v>
auto ordered_tree<k, v>::rend() noexcept -> std::reverse_iterator<iterator>
{
    return std::reverse_iterator<iterator>(begin());
}

template <typename k, typename v>
auto ordered_tree<k, v>::crbegin() noexcept -> std::reverse_iterator<const_iterator>
{
    return std::reverse_iterator<const_iterator>(cend());
}

template <typename k, typename v>
auto ordered_tree<k, v>::crend() noexcept -> std::reverse_iterator<const_iterator>
{
    return std::reverse_iterator<const_iterator>(cbegin());
}

template <typename k, typename v>
auto ordered_tree<k, v>::find_node(node& root, const key_type& target_key) const -> node*
{
    if (root.value.first == target_key)
    {
        return &root;
    }

    for (auto& node: root.children)
    {
        if (auto* found{ find_node(*node.get(), target_key) })
        {
            return found;
        }
    }

    return nullptr;
}

template <typename k, typename v>
auto ordered_tree<k, v>::get_last_node(node& root) -> node&
{
    if (root.children.empty())
    {
        return root;
    }

    return get_last_node(*root.children.back());
}

template <typename k, typename v>
auto ordered_tree<k, v>::count_descendants(const node& root) -> std::size_t
{
    auto counter{ root.children.size() };

    for (const auto& child: root.children)
    {
        counter += count_descendants(*child.get());
    }

    return counter;
}

template <typename k, typename v>
auto ordered_tree<k, v>::copy_tree(const node& from, node* parent) -> std::unique_ptr<node>
{
    auto copy{ std::make_unique<node>(
        node{
            .parent = parent,
            .children = {},
            .value = from.value
        }) };

    for (const auto& child: from.children)
    {
        copy->children.emplace_back(copy_tree(*child, copy.get()));
    }

    return copy;
}

template <typename k, typename v>
template <typename it_value_t, typename it_ptr_t, typename it_ref_t>
ordered_tree<k, v>::iterator_base<it_value_t, it_ptr_t, it_ref_t>::iterator_base(node* root, node* tree_node)
    : _root{ root }
    , _node{ tree_node }
{
}

template <typename k, typename v>
template <typename it_value_t, typename it_ptr_t, typename it_ref_t>
auto ordered_tree<k, v>::iterator_base<it_value_t, it_ptr_t, it_ref_t>::operator *() const noexcept -> reference
{
    return _node->value;
}

template <typename k, typename v>
template <typename it_value_t, typename it_ptr_t, typename it_ref_t>
auto ordered_tree<k, v>::iterator_base<it_value_t, it_ptr_t, it_ref_t>::operator ->() const noexcept -> pointer
{
    return &_node->value;
}

template <typename k, typename v>
template <typename it_value_t, typename it_ptr_t, typename it_ref_t>
auto ordered_tree<k, v>::iterator_base<it_value_t, it_ptr_t, it_ref_t>::operator ++() noexcept -> iterator_base&
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

    for (auto* ancestor{ _node }; ancestor->parent; ancestor = ancestor->parent)
    {
        auto self{ std::ranges::find_if(ancestor->parent->children, [ancestor] (auto& node) { return node.get() == ancestor; }) };
        if (*self != ancestor->parent->children.back())
        {
            _node = (++self)->get();
            return *this;
        }
    }

    _node = nullptr;

    return *this;
}

template <typename k, typename v>
template <typename it_value_t, typename it_ptr_t, typename it_ref_t>
auto ordered_tree<k, v>::iterator_base<it_value_t, it_ptr_t, it_ref_t>::operator ++(int) noexcept -> iterator_base
{
    auto tmp{ *this };

    operator++();

    return tmp;
}

template <typename k, typename v>
template <typename it_value_t, typename it_ptr_t, typename it_ref_t>
auto ordered_tree<k, v>::iterator_base<it_value_t, it_ptr_t, it_ref_t>::operator --() noexcept -> iterator_base&
{
    if (!_node)
    {
        _node = &get_last_node(*_root);
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
        _node = &get_last_node(*((--self)->get()));
    }
    else
    {
        _node = _node->parent;
    }

    return *this;
}

template <typename k, typename v>
template <typename it_value_t, typename it_ptr_t, typename it_ref_t>
auto ordered_tree<k, v>::iterator_base<it_value_t, it_ptr_t, it_ref_t>::operator --(int) noexcept -> iterator_base
{
    auto tmp{ *this };

    operator--();

    return tmp;
}

template <typename k, typename v>
template <typename it_value_t, typename it_ptr_t, typename it_ref_t>
auto ordered_tree<k, v>::iterator_base<it_value_t, it_ptr_t, it_ref_t>::operator ==(std::default_sentinel_t) const noexcept -> bool
{
    return !static_cast<bool>(_node);
}

template <typename k, typename v>
template <typename it_value_t, typename it_ptr_t, typename it_ref_t>
ordered_tree<k, v>::iterator_base<it_value_t, it_ptr_t, it_ref_t>::operator bool() const noexcept
{
    return static_cast<bool>(_node);
}

}
