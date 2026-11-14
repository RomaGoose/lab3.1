#include "avl_test_access.hpp"

#include <gtest/gtest.h>

#include <concepts>
#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

struct invalid_compare {
    void operator()(int, int) const {}
};

struct mutable_compare {
    bool operator()(int left, int right) { return left < right; }
};

template<class T, class Compare>
concept valid_tree = requires { typename avl_tree<T, Compare>; };

template<class Tree, class T>
concept can_insert_copy = requires(Tree& tree, const T& value) {
    tree.insert(value);
};

static_assert(!valid_tree<int, invalid_compare>);
static_assert(!valid_tree<int, mutable_compare>);
static_assert(!std::is_copy_constructible_v<avl_tree<std::unique_ptr<int>>>);
static_assert(!std::is_copy_assignable_v<avl_tree<std::unique_ptr<int>>>);
static_assert(std::is_move_constructible_v<avl_tree<std::unique_ptr<int>>>);
static_assert(std::is_move_assignable_v<avl_tree<std::unique_ptr<int>>>);
static_assert(!can_insert_copy<avl_tree<std::unique_ptr<int>>, std::unique_ptr<int>>);

struct int_compare {
    bool descending;

    bool operator()(int left, int right) const {
        return descending ? left > right : left < right;
    }
};

struct configured_compare {
    bool descending;

    configured_compare() = delete;
    explicit configured_compare(bool descending) : descending(descending) {}

    bool operator()(int left, int right) const {
        return descending ? left > right : left < right;
    }
};

struct move_only_compare {
    move_only_compare() = default;
    move_only_compare(const move_only_compare&) = delete;
    move_only_compare(move_only_compare&&) = default;
    move_only_compare& operator=(move_only_compare&&) = default;

    bool operator()(int left, int right) const { return left < right; }
};

struct throwing_move_compare {
    throwing_move_compare() = default;
    throwing_move_compare(const throwing_move_compare&) = default;
    throwing_move_compare(throwing_move_compare&&) noexcept(false) {}

    bool operator()(int left, int right) const { return left < right; }
};

struct throwing_swap_compare {
    bool operator()(int left, int right) const { return left < right; }
    friend void swap(throwing_swap_compare&, throwing_swap_compare&) noexcept(false) {}
};

struct copy_only {
    int key;

    explicit copy_only(int key) : key(key) {}
    copy_only(const copy_only&) = default;
    copy_only(copy_only&&) = delete;
};

struct key_compare {
    bool operator()(const auto& left, const auto& right) const {
        return left.key < right.key;
    }
};

static_assert(!std::default_initializable<avl_tree<int, configured_compare>>);
static_assert(std::constructible_from<
    avl_tree<int, configured_compare>, configured_compare>);
static_assert(!std::is_copy_constructible_v<avl_tree<int, move_only_compare>>);
static_assert(std::is_move_constructible_v<avl_tree<int, move_only_compare>>);
static_assert(std::is_nothrow_move_constructible_v<avl_tree<int>>);
static_assert(!std::is_nothrow_move_constructible_v<
    avl_tree<int, throwing_move_compare>>);
static_assert(!std::is_copy_assignable_v<avl_tree<int, throwing_swap_compare>>);
static_assert(!std::is_move_assignable_v<avl_tree<int, throwing_swap_compare>>);
static_assert(std::is_move_constructible_v<avl_tree<copy_only, key_compare>>);
static_assert(std::is_same_v<
    decltype(std::declval<const avl_tree<int>&>().find(0)),
    std::optional<std::reference_wrapper<const int>>>);

TEST(avl_value_test, comparator_with_state) {
    avl_tree<int, int_compare> tree(int_compare{true});
    for (int value : {2, 1, 3})
        tree.insert(value);

    EXPECT_TRUE(tree.contains(1));
    EXPECT_TRUE(tree.contains(3));
    EXPECT_TRUE(tree.erase(2));
    EXPECT_FALSE(tree.contains(2));
}

TEST(avl_value_test, copy_is_independent) {
    avl_tree<int, int_compare> original(int_compare{true});
    for (int value : {2, 1, 3})
        original.insert(value);

    avl_tree<int, int_compare> copy(original);
    original.erase(1);
    copy.insert(4);

    EXPECT_TRUE(copy.contains(1));
    EXPECT_FALSE(original.contains(4));
    EXPECT_TRUE(copy.contains(2));
    EXPECT_TRUE(copy.contains(3));
    EXPECT_EQ(copy.size(), 4u);
    avl_tree_test_access::check(original);
    avl_tree_test_access::check(copy);
}

TEST(avl_value_test, assignment_copies_comparator) {
    avl_tree<int, int_compare> source(int_compare{true});
    avl_tree<int, int_compare> target(int_compare{false});
    for (int value : {2, 1, 3})
        source.insert(value);
    target.insert(99);

    target = source;

    EXPECT_FALSE(target.contains(99));
    for (int value : {1, 2, 3})
        EXPECT_TRUE(target.contains(value));
    EXPECT_EQ(target.size(), 3u);
    source.erase(1);
    EXPECT_TRUE(target.contains(1));
}

TEST(avl_value_test, move_preserves_nodes_and_empties_source) {
    avl_tree<int, int_compare> source(int_compare{true});
    for (int value : {2, 1, 3})
        source.insert(value);
    auto found = source.find(2);
    ASSERT_TRUE(found);
    const int* address = &found->get();

    avl_tree<int, int_compare> moved(std::move(source));

    EXPECT_TRUE(source.empty());
    EXPECT_EQ(source.size(), 0u);
    auto result = moved.find(2);
    ASSERT_TRUE(result);
    EXPECT_EQ(&result->get(), address);
    EXPECT_TRUE(moved.contains(1));
    EXPECT_TRUE(moved.contains(3));
    source.insert(7);
    EXPECT_TRUE(source.contains(7));
}

TEST(avl_value_test, empty_copy_and_move) {
    avl_tree<int> source;
    avl_tree<int> copy(source);
    avl_tree<int> moved(std::move(source));

    EXPECT_TRUE(copy.empty());
    EXPECT_TRUE(moved.empty());
    EXPECT_TRUE(source.empty());
    EXPECT_EQ(copy.size(), 0u);
    EXPECT_EQ(moved.size(), 0u);
}

TEST(avl_value_test, self_assignment) {
    avl_tree<int> tree;
    for (int value : {2, 1, 3})
        tree.insert(value);

    auto& self = tree;
    tree = self;
    tree = std::move(self);

    std::vector<int> values;
    tree.traverse(traversal_order::left_root_right,
        [&](const int& value) { values.push_back(value); });
    EXPECT_EQ(values, (std::vector<int>{1, 2, 3}));
    EXPECT_EQ(tree.size(), 3u);
    avl_tree_test_access::check(tree);
}

TEST(avl_value_test, move_assignment_preserves_nodes) {
    avl_tree<int, int_compare> source(int_compare{true});
    avl_tree<int, int_compare> target(int_compare{false});
    for (int value : {2, 1, 3})
        source.insert(value);
    target.insert(99);
    auto found = source.find(2);
    ASSERT_TRUE(found);
    const int* address = &found->get();

    target = std::move(source);

    EXPECT_TRUE(source.empty());
    auto result = target.find(2);
    ASSERT_TRUE(result);
    EXPECT_EQ(&result->get(), address);
    EXPECT_FALSE(target.contains(99));
    avl_tree_test_access::check(target);
}

TEST(avl_value_test, swap_keeps_comparator_with_nodes) {
    avl_tree<int, int_compare> left(int_compare{false});
    avl_tree<int, int_compare> right(int_compare{true});
    for (int value : {1, 2, 3})
        left.insert(value);
    for (int value : {4, 5})
        right.insert(value);

    using std::swap;
    swap(left, right);

    EXPECT_EQ(left.size(), 2u);
    EXPECT_EQ(right.size(), 3u);
    avl_tree_test_access::check(left);
    avl_tree_test_access::check(right);
}

TEST(avl_value_test, move_tree_without_moving_elements) {
    avl_tree<copy_only, key_compare> source;
    const copy_only value(7);
    source.insert(value);

    avl_tree<copy_only, key_compare> moved(std::move(source));

    EXPECT_TRUE(source.empty());
    EXPECT_TRUE(moved.contains(value));
    avl_tree_test_access::check(moved);
}

TEST(avl_value_test, comparator_without_default_constructor) {
    avl_tree<int, configured_compare> configured(configured_compare{true});
    for (int value : {1, 2, 3})
        configured.insert(value);
    avl_tree_test_access::check(configured);
}

TEST(avl_value_test, move_only_comparator) {
    avl_tree<int, move_only_compare> source;
    source.insert(7);
    avl_tree<int, move_only_compare> moved(std::move(source));
    avl_tree<int, move_only_compare> target;
    target = std::move(moved);

    EXPECT_TRUE(source.empty());
    EXPECT_TRUE(moved.empty());
    EXPECT_TRUE(target.contains(7));
}

TEST(avl_value_test, strings) {
    avl_tree<std::string> words;
    for (const char* word : {"pear", "apple", "plum"})
        words.insert(word);
    std::vector<std::string> values;
    words.traverse(traversal_order::left_root_right,
        [&](const std::string& word) { values.push_back(word); });
    EXPECT_EQ(values, (std::vector<std::string>{"apple", "pear", "plum"}));
    avl_tree_test_access::check(words);
}

TEST(avl_value_test, real_numbers) {
    avl_tree<double> numbers;
    for (double value : {1.5, -2.5, 0.0})
        numbers.insert(value);
    EXPECT_TRUE(numbers.contains(-2.5));
    EXPECT_TRUE(numbers.erase(1.5));
    avl_tree_test_access::check(numbers);
}

struct const_compare {
    bool operator()(const int& left, const int& right) const {
        return left < right;
    }

    bool operator()(const int&, const int&) = delete;
    bool operator()(int&, int&) const = delete;
};

TEST(avl_value_test, comparator_and_arguments_are_const) {
    avl_tree<int, const_compare> tree;
    for (int value : {2, 1, 3})
        tree.insert(value);

    EXPECT_TRUE(tree.contains(1));
    EXPECT_TRUE(tree.erase(2));
    avl_tree_test_access::check(tree);
}

struct record {
    int key;
    std::string name;
    std::vector<int> entries;
};

TEST(avl_value_test, compare_records_by_key) {
    avl_tree<record, key_compare> tree;
    tree.insert(record{2, "first", {10, 20}});
    tree.insert(record{1, "left", {30}});
    tree.insert(record{3, "right", {40}});

    EXPECT_FALSE(tree.insert(record{2, "replacement", {99}}));
    auto found = tree.find(record{2, "", {}});
    ASSERT_TRUE(found);
    EXPECT_EQ(found->get().name, "first");
    EXPECT_EQ(found->get().entries, (std::vector<int>{10, 20}));
    EXPECT_TRUE(tree.erase(record{2, "", {}}));
    avl_tree_test_access::check(tree);
}

struct pointer_compare {
    bool operator()(const std::unique_ptr<int>& left,
        const std::unique_ptr<int>& right) const {
        return *left < *right;
    }
};

TEST(avl_value_test, move_only_elements) {
    avl_tree<std::unique_ptr<int>, pointer_compare> tree;
    auto value = std::make_unique<int>(2);
    tree.insert(std::move(value));
    tree.insert(std::make_unique<int>(1));
    tree.insert(std::make_unique<int>(3));

    EXPECT_EQ(value, nullptr);
    auto duplicate = std::make_unique<int>(2);
    EXPECT_FALSE(tree.insert(std::move(duplicate)));
    ASSERT_NE(duplicate, nullptr);
    EXPECT_EQ(*duplicate, 2);
    auto found = tree.find(std::make_unique<int>(2));
    ASSERT_TRUE(found);
    EXPECT_EQ(*found->get(), 2);
    EXPECT_TRUE(tree.erase(std::make_unique<int>(2)));
    avl_tree_test_access::check(tree);
}

struct checked_value {
    static inline int alive = 0;
    static inline int copies_left = -1;
    int key;

    explicit checked_value(int key) : key(key) { ++alive; }

    checked_value(const checked_value& other) : key(other.key) {
        if (copies_left == 0) throw std::runtime_error("copy failed");
        if (copies_left > 0) --copies_left;
        ++alive;
    }

    checked_value(checked_value&& other) noexcept
        : key(std::exchange(other.key, -1)) { ++alive; }

    ~checked_value() { --alive; }
};

TEST(avl_value_test, insertion_copy_failure_keeps_tree) {
    avl_tree<checked_value, key_compare> tree;
    for (int value : {2, 1, 3})
        tree.insert(checked_value(value));
    const checked_value value(4);
    int alive = checked_value::alive;

    checked_value::copies_left = 0;
    EXPECT_THROW(tree.insert(value), std::runtime_error);
    checked_value::copies_left = -1;

    EXPECT_EQ(checked_value::alive, alive);
    EXPECT_EQ(tree.size(), 3u);
    EXPECT_FALSE(tree.contains(value));
    avl_tree_test_access::check(tree);
}

TEST(avl_value_test, failed_copy_assignment_keeps_both_trees) {
    avl_tree<checked_value, key_compare> source;
    avl_tree<checked_value, key_compare> target;
    for (int value : {2, 1, 3})
        source.insert(checked_value(value));
    target.insert(checked_value(99));
    int alive = checked_value::alive;

    for (int limit : {0, 1, 2}) {
        SCOPED_TRACE(limit);
        checked_value::copies_left = limit;
        EXPECT_THROW(target = source, std::runtime_error);
        checked_value::copies_left = -1;

        EXPECT_EQ(checked_value::alive, alive);
        EXPECT_EQ(target.size(), 1u);
        EXPECT_TRUE(target.contains(checked_value(99)));
        EXPECT_EQ(source.size(), 3u);
        avl_tree_test_access::check(source);
        avl_tree_test_access::check(target);
    }
}

TEST(avl_value_test, destruction_and_erasure_release_values) {
    EXPECT_EQ(checked_value::alive, 0);
    {
        avl_tree<checked_value, key_compare> tree;
        for (int value : {2, 1, 3})
            tree.insert(checked_value(value));
        EXPECT_EQ(checked_value::alive, 3);
        tree.erase(checked_value(2));
        EXPECT_EQ(checked_value::alive, 2);
    }
    EXPECT_EQ(checked_value::alive, 0);
}

struct controlled_compare {
    std::shared_ptr<int> remaining;

    bool operator()(int left, int right) const {
        if (*remaining == 0) throw std::runtime_error("comparison failed");
        if (*remaining > 0) --*remaining;
        return left < right;
    }
};

TEST(avl_value_test, comparison_failure_keeps_tree) {
    auto remaining = std::make_shared<int>(-1);
    avl_tree<int, controlled_compare> tree(controlled_compare{remaining});
    for (int value : {4, 2, 6, 1, 3, 5, 7})
        tree.insert(value);

    for (int operation = 0; operation < 4; ++operation) {
        for (int limit = 0; limit < 6; ++limit) {
            SCOPED_TRACE(operation);
            SCOPED_TRACE(limit);
            *remaining = limit;
            switch (operation) {
                case 0:
                    EXPECT_THROW(tree.insert(8), std::runtime_error);
                    break;
                case 1:
                    EXPECT_THROW(tree.erase(7), std::runtime_error);
                    break;
                case 2:
                    EXPECT_THROW(tree.find(7), std::runtime_error);
                    break;
                case 3:
                    EXPECT_THROW(tree.contains(7), std::runtime_error);
                    break;
            }
            *remaining = -1;

            std::vector<int> values;
            tree.traverse(traversal_order::left_root_right,
                [&](const int& value) { values.push_back(value); });
            EXPECT_EQ(values, (std::vector<int>{1, 2, 3, 4, 5, 6, 7}));
            EXPECT_EQ(tree.size(), 7u);
            avl_tree_test_access::check(tree);
        }
    }
}
