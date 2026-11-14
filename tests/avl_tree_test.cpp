#include "avl_test_access.hpp"

#include <gtest/gtest.h>

#include <array>
#include <limits>
#include <random>
#include <set>
#include <vector>


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

TEST(avl_tree_test, rotations_preserve_references) {
    const std::array<std::array<int, 3>, 4> orders = {{
        {30, 20, 10}, {10, 20, 30}, {30, 10, 20}, {10, 30, 20}
    }};

    for (const auto& order : orders) {
        SCOPED_TRACE(::testing::PrintToString(order));
        avl_tree<int> tree;
        tree.insert(order[0]);
        tree.insert(order[1]);
        auto first = tree.find(order[0]);
        auto second = tree.find(order[1]);
        ASSERT_TRUE(first);
        ASSERT_TRUE(second);
        const int* first_address = &first->get();
        const int* second_address = &second->get();

        tree.insert(order[2]);

        auto first_after = tree.find(order[0]);
        auto second_after = tree.find(order[1]);
        ASSERT_TRUE(first_after);
        ASSERT_TRUE(second_after);
        EXPECT_EQ(&first_after->get(), first_address);
        EXPECT_EQ(&second_after->get(), second_address);
        EXPECT_EQ(first->get(), order[0]);
        EXPECT_EQ(second->get(), order[1]);
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

TEST(avl_tree_test, random_operations_match_set) {
    avl_tree<int> tree;
    std::set<int> expected;
    std::mt19937 random(42);
    std::uniform_int_distribution<int> keys(-100, 100);
    std::uniform_int_distribution<int> operations(0, 2);

    for (int step = 0; step < 2000; ++step) {
        SCOPED_TRACE(step);
        int value = keys(random);
        switch (operations(random)) {
            case 0:
                EXPECT_EQ(tree.insert(value), expected.insert(value).second);
                break;
            case 1:
                EXPECT_EQ(tree.erase(value), expected.erase(value) != 0);
                break;
            case 2:
                EXPECT_EQ(tree.contains(value), expected.contains(value));
                break;
        }

        std::vector<int> values;
        tree.traverse(traversal_order::left_root_right,
            [&](const int& item) { values.push_back(item); });
        EXPECT_EQ(values, (std::vector<int>(expected.begin(), expected.end())));
        avl_tree_test_access::check(tree);
    }
}

TEST(avl_tree_test, integer_limits) {
    avl_tree<int> tree;
    int low = std::numeric_limits<int>::min();
    int high = std::numeric_limits<int>::max();

    EXPECT_TRUE(tree.insert(low));
    EXPECT_TRUE(tree.insert(high));
    EXPECT_TRUE(tree.insert(0));
    EXPECT_TRUE(tree.contains(low));
    EXPECT_TRUE(tree.contains(high));
    EXPECT_TRUE(tree.erase(low));
    EXPECT_TRUE(tree.erase(high));
    avl_tree_test_access::check(tree);
}
