#pragma once

#include <algorithm>
#include <cstddef>
#include <functional>
#include <memory>
#include <optional>
#include <utility>

struct avl_tree_test_access;

template<class T, class Compare = std::less<T>>
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

        std::unique_ptr<node> root;
        std::size_t size_ = 0;
        Compare compare;
        
    public:
        avl_tree() = default;
        ~avl_tree() = default;

        std::size_t size() const noexcept;
        bool empty() const noexcept;
        std::optional<std::reference_wrapper<const T>> find(const T& value) const;
        bool contains(const T& value) const;
        bool insert(const T& value);
        bool insert(T&& value);
        bool erase(const T& value);

    private:
      
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

        friend struct avl_tree_test_access;
};

template<class T, class Compare>
template<class U>
avl_tree<T, Compare>::node::node(U&& value) : value(std::forward<U>(value)) {}

template<class T, class Compare>
std::size_t avl_tree<T, Compare>::size() const noexcept {
    return size_;
}

template<class T, class Compare>
bool avl_tree<T, Compare>::empty() const noexcept {
    return size_ == 0;
}

template<class T, class Compare>
std::optional<std::reference_wrapper<const T>> avl_tree<T, Compare>::find(const T& value) const {
    const node* current = root.get();
    while (current) {
        if (compare(value, current->value))
            current = current->left.get();
        else if (compare(current->value, value))
            current = current->right.get();
        else
            return std::cref(current->value);
    }
    return std::nullopt;
}

template<class T, class Compare>
bool avl_tree<T, Compare>::contains(const T& value) const {
    return find(value).has_value();
}

template<class T, class Compare>
bool avl_tree<T, Compare>::insert(const T& value) {
    return insert_node(root, value);
}

template<class T, class Compare>
bool avl_tree<T, Compare>::insert(T&& value) {
    return insert_node(root, std::move(value));
}

template<class T, class Compare>
template<class U>
bool avl_tree<T, Compare>::insert_node(std::unique_ptr<node>& current, U&& value) {
    if (!current) {
        current = std::make_unique<node>(std::forward<U>(value));
        ++size_;
        return true;
    }

    bool inserted;
    if (compare(value, current->value))
        inserted = insert_node(current->left, std::forward<U>(value));
    else if (compare(current->value, value))
        inserted = insert_node(current->right, std::forward<U>(value));
    else
        return false;

    if (inserted) balance(current);
    return inserted;
}

template<class T, class Compare>
int avl_tree<T, Compare>::height(const std::unique_ptr<node>& current) noexcept {
    return current ? current->height : 0;
}

template<class T, class Compare>
int avl_tree<T, Compare>::balance_factor(const std::unique_ptr<node>& current) noexcept {
    return height(current->left) - height(current->right);
}

template<class T, class Compare>
void avl_tree<T, Compare>::update_height(node& current) noexcept {
    current.height = 1 + std::max(height(current.left), height(current.right));
}

template<class T, class Compare>
void avl_tree<T, Compare>::rotate_left(std::unique_ptr<node>& current) noexcept {
    auto next = std::move(current->right);
    current->right = std::move(next->left);
    update_height(*current);
    next->left = std::move(current);
    update_height(*next);
    current = std::move(next);
}

template<class T, class Compare>
void avl_tree<T, Compare>::rotate_right(std::unique_ptr<node>& current) noexcept {
    auto next = std::move(current->left);
    current->left = std::move(next->right);
    update_height(*current);
    next->right = std::move(current);
    update_height(*next);
    current = std::move(next);
}

template<class T, class Compare>
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

template<class T, class Compare>
bool avl_tree<T, Compare>::erase(const T& value) {
    bool erased = erase_node(root, value);
    if (erased) --size_;
    return erased;
}

template<class T, class Compare>
bool avl_tree<T, Compare>::erase_node(std::unique_ptr<node>& current, const T& value) {
    if (!current) return false;

    if (compare(value, current->value)) {
        if (!erase_node(current->left, value)) return false;
    }
    else if (compare(current->value, value)) {
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

template<class T, class Compare>
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
