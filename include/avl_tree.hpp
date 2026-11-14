#pragma once

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <functional>
#include <memory>
#include <optional>
#include <ostream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

struct avl_tree_test_access;

enum class traversal_order {
    root_left_right,
    root_right_left,
    left_right_root,
    left_root_right,
    right_left_root,
    right_root_left
};

template<class Compare, class T>
concept avl_compare = std::strict_weak_order<const Compare&, const T&, const T&>;

template<class T, avl_compare<T> Compare = std::less<T>>
class avl_tree {
    private:
        struct node {
            T value;
            int height = 1;
            std::unique_ptr<node> left;
            std::unique_ptr<node> right;

            template<class U>
            explicit node(U&& value);
        };

        Compare compare{};
        std::unique_ptr<node> root;
        std::size_t size_ = 0;
        
    public:
        avl_tree() requires std::default_initializable<Compare> = default;
        
        explicit avl_tree(Compare compare)
            requires std::constructible_from<Compare, Compare&&>;
            
        avl_tree(const avl_tree& other)
            requires std::constructible_from<T, const T&> &&
                     std::constructible_from<Compare, const Compare&>;
                     
        avl_tree(avl_tree&& other)
            noexcept(std::is_nothrow_move_constructible_v<Compare>)
            requires std::constructible_from<Compare, Compare&&>;
            
        ~avl_tree() = default;

        avl_tree& operator=(avl_tree other) noexcept
            requires std::is_nothrow_swappable_v<Compare>;
            
        void swap(avl_tree& other) noexcept
            requires std::is_nothrow_swappable_v<Compare>;

        std::size_t size() const noexcept;
        bool empty() const noexcept;
        std::optional<std::reference_wrapper<const T>> find(const T& value) const;
        bool contains(const T& value) const;

        bool insert(const T& value)
            requires std::constructible_from<T, const T&>;
        bool insert(T&& value)
            requires std::constructible_from<T, T&&>;
        bool erase(const T& value);

        template<class Visitor>
            requires std::invocable<Visitor&, const T&>
        void traverse(traversal_order order, Visitor&& visitor) const;

        template<class Formatter>
            requires std::invocable<Formatter&, std::ostream&, const T&>
        void print_tree(std::ostream& stream, Formatter&& formatter) const;

    private:
        bool compare_values(const T& left, const T& right) const;

        template<class U>
        bool insert_node(std::unique_ptr<node>& current, U&& value);
        bool erase_node(std::unique_ptr<node>& current, const T& value);
        
        static int height(const std::unique_ptr<node>& current) noexcept;
        static int balance_factor(const std::unique_ptr<node>& current) noexcept;
        static void update_height(node& current) noexcept;

        static void rotate_left(std::unique_ptr<node>& current) noexcept;
        static void rotate_right(std::unique_ptr<node>& current) noexcept;
        static void balance(std::unique_ptr<node>& current) noexcept;

        static std::unique_ptr<node> detach_min(std::unique_ptr<node>& current) noexcept;

        static std::unique_ptr<node> clone(const node* current);

        template<class Visitor>
        static void traverse_node(
            const node* current, Visitor& visitor, int root_position, bool reverse
        );

        template<class Formatter>
        static void print_children(
            std::ostream& stream, const node* current,
            Formatter& formatter, const std::string& prefix
        );

        friend struct avl_tree_test_access;
};

template<class T, avl_compare<T> Compare>
template<class U>
avl_tree<T, Compare>::node::node(U&& value) : value(std::forward<U>(value)) {}

template<class T, avl_compare<T> Compare>
avl_tree<T, Compare>::avl_tree(Compare compare)
    requires std::constructible_from<Compare, Compare&&>
    : compare(std::move(compare)) {}

template<class T, avl_compare<T> Compare>
avl_tree<T, Compare>::avl_tree(const avl_tree& other)
    requires std::constructible_from<T, const T&> &&
             std::constructible_from<Compare, const Compare&>
    : compare(other.compare), root(clone(other.root.get())), size_(other.size_) {}

template<class T, avl_compare<T> Compare>
avl_tree<T, Compare>::avl_tree(avl_tree&& other)
    noexcept(std::is_nothrow_move_constructible_v<Compare>)
    requires std::constructible_from<Compare, Compare&&>
    : compare(std::move(other.compare)), root(std::move(other.root)),
      size_(std::exchange(other.size_, 0)) {}

template<class T, avl_compare<T> Compare>
avl_tree<T, Compare>& avl_tree<T, Compare>::operator=(avl_tree other) noexcept
    requires std::is_nothrow_swappable_v<Compare> {
    swap(other);
    return *this;
}

template<class T, avl_compare<T> Compare>
void avl_tree<T, Compare>::swap(avl_tree& other) noexcept
    requires std::is_nothrow_swappable_v<Compare> {
    using std::swap;
    swap(compare, other.compare);
    swap(root, other.root);
    swap(size_, other.size_);
}

template<class T, avl_compare<T> Compare>
void swap(avl_tree<T, Compare>& left, avl_tree<T, Compare>& right) noexcept
    requires std::is_nothrow_swappable_v<Compare> {
    left.swap(right);
}

template<class T, avl_compare<T> Compare>
std::unique_ptr<typename avl_tree<T, Compare>::node>
avl_tree<T, Compare>::clone(const node* current) {
    if (!current) return nullptr;

    auto result = std::make_unique<node>(current->value);
    result->height = current->height;
    result->left = clone(current->left.get());
    result->right = clone(current->right.get());
    return result;
}

template<class T, avl_compare<T> Compare>
bool avl_tree<T, Compare>::compare_values(const T& left, const T& right) const {
    return std::invoke(compare, left, right);
}

template<class T, avl_compare<T> Compare>
std::size_t avl_tree<T, Compare>::size() const noexcept {
    return size_;
}

template<class T, avl_compare<T> Compare>
bool avl_tree<T, Compare>::empty() const noexcept {
    return size_ == 0;
}

template<class T, avl_compare<T> Compare>
std::optional<std::reference_wrapper<const T>>
avl_tree<T, Compare>::find(const T& value) const {
    const node* current = root.get();
    while (current) {
        if (compare_values(value, current->value))
            current = current->left.get();
        else if (compare_values(current->value, value))
            current = current->right.get();
        else
            return std::cref(current->value);
    }
    return std::nullopt;
}

template<class T, avl_compare<T> Compare>
bool avl_tree<T, Compare>::contains(const T& value) const {
    return find(value).has_value();
}

template<class T, avl_compare<T> Compare>
bool avl_tree<T, Compare>::insert(const T& value)
    requires std::constructible_from<T, const T&> {
    return insert_node(root, value);
}

template<class T, avl_compare<T> Compare>
bool avl_tree<T, Compare>::insert(T&& value)
    requires std::constructible_from<T, T&&> {
    return insert_node(root, std::move(value));
}

template<class T, avl_compare<T> Compare>
template<class U>
bool avl_tree<T, Compare>::insert_node(std::unique_ptr<node>& current, U&& value) {
    if (!current) {
        current = std::make_unique<node>(std::forward<U>(value));
        ++size_;
        return true;
    }

    bool inserted;
    if (compare_values(value, current->value))
        inserted = insert_node(current->left, std::forward<U>(value));
    else if (compare_values(current->value, value))
        inserted = insert_node(current->right, std::forward<U>(value));
    else
        return false;

    if (inserted) balance(current);
    return inserted;
}

template<class T, avl_compare<T> Compare>
int avl_tree<T, Compare>::height(const std::unique_ptr<node>& current) noexcept {
    return current ? current->height : 0;
}

template<class T, avl_compare<T> Compare>
int avl_tree<T, Compare>::balance_factor(const std::unique_ptr<node>& current) noexcept {
    return height(current->left) - height(current->right);
}

template<class T, avl_compare<T> Compare>
void avl_tree<T, Compare>::update_height(node& current) noexcept {
    current.height = 1 + std::max(height(current.left), height(current.right));
}

template<class T, avl_compare<T> Compare>
void avl_tree<T, Compare>::rotate_left(std::unique_ptr<node>& current) noexcept {
    auto next = std::move(current->right);
    current->right = std::move(next->left);
    update_height(*current);
    next->left = std::move(current);
    update_height(*next);
    current = std::move(next);
}

template<class T, avl_compare<T> Compare>
void avl_tree<T, Compare>::rotate_right(std::unique_ptr<node>& current) noexcept {
    auto next = std::move(current->left);
    current->left = std::move(next->right);
    update_height(*current);
    next->right = std::move(current);
    update_height(*next);
    current = std::move(next);
}

template<class T, avl_compare<T> Compare>
void avl_tree<T, Compare>::balance(std::unique_ptr<node>& current) noexcept {
    update_height(*current);
    int factor = balance_factor(current);
    if (factor > 1) {
        if (balance_factor(current->left) < 0)
            rotate_left(current->left);
        rotate_right(current);
    }
    else if (factor < -1) {
        if (balance_factor(current->right) > 0)
            rotate_right(current->right);
        rotate_left(current);
    }
}

template<class T, avl_compare<T> Compare>
bool avl_tree<T, Compare>::erase(const T& value) {
    bool erased = erase_node(root, value);
    if (erased) --size_;
    return erased;
}

template<class T, avl_compare<T> Compare>
bool avl_tree<T, Compare>::erase_node(std::unique_ptr<node>& current, const T& value) {
    if (!current) return false;

    if (compare_values(value, current->value)) {
        if (!erase_node(current->left, value)) return false;
    }
    else if (compare_values(current->value, value)) {
        if (!erase_node(current->right, value)) return false;
    }
    else {
        auto removed = std::move(current);
        if (!removed->left)
            current = std::move(removed->right);
        else if (!removed->right)
            current = std::move(removed->left);
        else {
            auto next = detach_min(removed->right);
            next->left = std::move(removed->left);
            next->right = std::move(removed->right);
            current = std::move(next);
        }
    }

    if (current) balance(current);
    return true;
}

template<class T, avl_compare<T> Compare>
std::unique_ptr<typename avl_tree<T, Compare>::node>
avl_tree<T, Compare>::detach_min(std::unique_ptr<node>& current) noexcept {
    if (!current->left) {
        auto result = std::move(current);
        current = std::move(result->right);
        return result;
    }

    auto result = detach_min(current->left);
    balance(current);
    return result;
}

template<class T, avl_compare<T> Compare>
template<class Visitor>
    requires std::invocable<Visitor&, const T&>
void avl_tree<T, Compare>::traverse(traversal_order order, Visitor&& visitor) const {
    switch (order) {
        case traversal_order::root_left_right:
            traverse_node(root.get(), visitor, 0, false);
            break;
        case traversal_order::root_right_left:
            traverse_node(root.get(), visitor, 0, true);
            break;
        case traversal_order::left_root_right:
            traverse_node(root.get(), visitor, 1, false);
            break;
        case traversal_order::right_root_left:
            traverse_node(root.get(), visitor, 1, true);
            break;
        case traversal_order::left_right_root:
            traverse_node(root.get(), visitor, 2, false);
            break;
        case traversal_order::right_left_root:
            traverse_node(root.get(), visitor, 2, true);
            break;
        default:
            throw std::invalid_argument("invalid traversal order");
    }
}

template<class T, avl_compare<T> Compare>
template<class Visitor>
void avl_tree<T, Compare>::traverse_node(
    const node* current, Visitor& visitor, int root_position, bool reverse
) {
    if (!current) return;

    const node* first = current->left.get();
    const node* second = current->right.get();
    if (reverse) std::swap(first, second);

    if (root_position == 0) std::invoke(visitor, current->value);
    traverse_node(first, visitor, root_position, reverse);
    if (root_position == 1) std::invoke(visitor, current->value);
    traverse_node(second, visitor, root_position, reverse);
    if (root_position == 2) std::invoke(visitor, current->value);
}

template<class T, avl_compare<T> Compare>
template<class Formatter>
    requires std::invocable<Formatter&, std::ostream&, const T&>
void avl_tree<T, Compare>::print_tree(
    std::ostream& stream, Formatter&& formatter
) const {
    if (!root) {
        stream << "(пусто)\n";
        return;
    }

    std::invoke(formatter, stream, std::as_const(root->value));
    stream << '\n';
    print_children(stream, root.get(), formatter, "");
}

template<class T, avl_compare<T> Compare>
template<class Formatter>
void avl_tree<T, Compare>::print_children(
    std::ostream& stream, const node* current,
    Formatter& formatter, const std::string& prefix
) {
    const std::pair<const node*, const char*> children[] = {
        {current->left.get(), "L"}, {current->right.get(), "R"}
    };

    for (auto [child, side] : children) {
        if (!child) continue;

        bool last = !current->right || child == current->right.get();
        stream << prefix << (last ? "└── " : "├── ") << side << ": ";
        std::invoke(formatter, stream, child->value);
        stream << '\n';
        print_children(stream, child, formatter, prefix + (last ? "    " : "│   "));
    }
}
