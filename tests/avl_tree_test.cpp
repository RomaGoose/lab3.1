#include "avl_tree.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

struct avl_tree_test_access {
    template<class Node>
    static int check_node(const Node* current, std::vector<int>& values) {
        if (!current) return 0;

        int left_height = check_node(current->left.get(), values);
        values.push_back(current->value);
        int right_height = check_node(current->right.get(), values);
        int height = 1 + std::max(left_height, right_height);

        EXPECT_EQ(current->height, height);
        EXPECT_LE(std::abs(left_height - right_height), 1);
        return height;
    }

    template<class Compare>
    static void check(const avl_tree<int, Compare>& tree) {
        std::vector<int> values;
        check_node(tree.root.get(), values);
        EXPECT_EQ(values.size(), tree.size());
        for (std::size_t i = 1; i < values.size(); ++i)
            EXPECT_TRUE(tree.compare(values[i - 1], values[i]));
    }

    static int root_value(const avl_tree<int>& tree) {
        return tree.root->value;
    }
};

TEST(avl_tree_test, empty_tree) {
    avl_tree<int> tree;

    EXPECT_TRUE(tree.empty());
    EXPECT_EQ(tree.size(), 0u);
    EXPECT_FALSE(tree.contains(10));
    EXPECT_FALSE(tree.find(10).has_value());
}

TEST(avl_tree_test, insert_find_and_duplicate) {
    avl_tree<int> tree;
    const int value = 10;

    EXPECT_TRUE(tree.insert(value));
    EXPECT_TRUE(tree.insert(5));
    EXPECT_TRUE(tree.insert(15));
    EXPECT_FALSE(tree.insert(10));
    EXPECT_EQ(tree.size(), 3u);
    EXPECT_FALSE(tree.empty());
    for (int key : {5, 10, 15}) {
        ASSERT_TRUE(tree.find(key));
        EXPECT_EQ(tree.find(key)->get(), key);
        EXPECT_TRUE(tree.contains(key));
    }
    EXPECT_FALSE(tree.contains(7));
}

TEST(avl_tree_test, insertion_rotations) {
    const std::array<std::array<int, 3>, 4> orders = {{
        {30, 20, 10}, {10, 20, 30}, {30, 10, 20}, {10, 30, 20}
    }};

    for (const auto& order : orders) {
        SCOPED_TRACE(::testing::PrintToString(order));
        avl_tree<int> tree;
        for (int value : order) {
            ASSERT_TRUE(tree.insert(value));
            avl_tree_test_access::check(tree);
        }
        EXPECT_EQ(avl_tree_test_access::root_value(tree), 20);
        EXPECT_EQ(tree.size(), 3u);
    }
}

TEST(avl_tree_test, erase_leaf_one_child_and_root) {
    avl_tree<int> tree;
    EXPECT_FALSE(tree.erase(10));
    for (int value : {20, 10, 30, 25})
        tree.insert(value);

    EXPECT_TRUE(tree.erase(10));
    EXPECT_FALSE(tree.contains(10));
    avl_tree_test_access::check(tree);
    EXPECT_TRUE(tree.erase(30));
    EXPECT_TRUE(tree.contains(25));
    avl_tree_test_access::check(tree);
    EXPECT_TRUE(tree.erase(20));
    EXPECT_EQ(tree.size(), 1u);
    EXPECT_TRUE(tree.contains(25));
    avl_tree_test_access::check(tree);
    EXPECT_TRUE(tree.erase(25));
    EXPECT_FALSE(tree.erase(25));
    EXPECT_TRUE(tree.empty());
    avl_tree_test_access::check(tree);
}

TEST(avl_tree_test, erase_root_with_one_child) {
    for (int child : {10, 30}) {
        SCOPED_TRACE(child);
        avl_tree<int> tree;
        tree.insert(20);
        tree.insert(child);
        ASSERT_TRUE(tree.erase(20));
        EXPECT_EQ(tree.size(), 1u);
        EXPECT_EQ(avl_tree_test_access::root_value(tree), child);
        EXPECT_FALSE(tree.contains(20));
        avl_tree_test_access::check(tree);
    }
}

TEST(avl_tree_test, erase_two_children_preserves_other_nodes) {
    const std::vector<std::vector<int>> orders = {
        {20, 10, 30},
        {40, 20, 60, 10, 30, 50, 70, 45, 55, 47}
    };

    for (const auto& order : orders) {
        SCOPED_TRACE(::testing::PrintToString(order));
        avl_tree<int> tree;
        for (int value : order)
            tree.insert(value);

        int removed = order.front();
        std::vector<const int*> addresses;
        for (int value : order) {
            auto found = tree.find(value);
            ASSERT_TRUE(found);
            addresses.push_back(&found->get());
        }

        ASSERT_TRUE(tree.erase(removed));
        EXPECT_EQ(tree.size(), order.size() - 1);
        EXPECT_FALSE(tree.contains(removed));
        for (std::size_t i = 1; i < order.size(); ++i) {
            ASSERT_TRUE(tree.find(order[i]));
            EXPECT_EQ(&tree.find(order[i])->get(), addresses[i]);
            EXPECT_EQ(tree.find(order[i])->get(), order[i]);
        }
        avl_tree_test_access::check(tree);
    }
}

TEST(avl_tree_test, deletion_rotations) {
    const std::array<std::vector<int>, 4> orders = {{
        {4, 2, 5, 1, 3}, {2, 1, 4, 3, 5}, {5, 2, 6, 3}, {2, 1, 5, 4}
    }};
    const std::array<int, 4> removed = {5, 1, 6, 1};
    const std::array<int, 4> roots = {2, 4, 3, 4};

    for (std::size_t i = 0; i < orders.size(); ++i) {
        SCOPED_TRACE(i);
        avl_tree<int> tree;
        for (int value : orders[i])
            tree.insert(value);
        ASSERT_TRUE(tree.erase(removed[i]));
        EXPECT_EQ(avl_tree_test_access::root_value(tree), roots[i]);
        EXPECT_FALSE(tree.contains(removed[i]));
        avl_tree_test_access::check(tree);
    }
}

TEST(avl_tree_test, sorted_insertions_and_repeated_deletions) {
    avl_tree<int> tree;
    constexpr int count = 512;
    for (int value = 0; value < count; ++value) {
        ASSERT_TRUE(tree.insert(value));
        avl_tree_test_access::check(tree);
    }

    EXPECT_FALSE(tree.erase(-1));
    EXPECT_FALSE(tree.erase(count));
    EXPECT_EQ(tree.size(), static_cast<std::size_t>(count));
    for (int parity : {0, 1}) {
        for (int value = parity; value < count; value += 2) {
            ASSERT_TRUE(tree.erase(value));
            EXPECT_FALSE(tree.contains(value));
            avl_tree_test_access::check(tree);
        }
    }
    EXPECT_TRUE(tree.empty());
    ASSERT_TRUE(tree.insert(42));
    EXPECT_EQ(tree.size(), 1u);
    ASSERT_TRUE(tree.find(42));
    EXPECT_EQ(tree.find(42)->get(), 42);
    avl_tree_test_access::check(tree);
}
