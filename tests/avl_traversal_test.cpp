#include "avl_tree.hpp"

#include <gtest/gtest.h>

#include <array>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <vector>

TEST(avl_traversal_test, all_orders) {
    avl_tree<int> tree;
    for (int value : {4, 2, 6, 1, 3, 5, 7})
        tree.insert(value);

    const std::vector<std::pair<traversal_order, std::vector<int>>> cases = {
        {traversal_order::root_left_right, {4, 2, 1, 3, 6, 5, 7}},
        {traversal_order::root_right_left, {4, 6, 7, 5, 2, 3, 1}},
        {traversal_order::left_right_root, {1, 3, 2, 5, 7, 6, 4}},
        {traversal_order::left_root_right, {1, 2, 3, 4, 5, 6, 7}},
        {traversal_order::right_left_root, {7, 5, 6, 3, 1, 2, 4}},
        {traversal_order::right_root_left, {7, 6, 5, 4, 3, 2, 1}}
    };

    const auto& view = tree;
    for (const auto& [order, expected] : cases) {
        SCOPED_TRACE(static_cast<int>(order));
        std::vector<int> values;
        view.traverse(order, [&](const int& value) { values.push_back(value); });
        EXPECT_EQ(values, expected);
    }
}

TEST(avl_traversal_test, empty_and_single_element) {
    const std::array orders = {
        traversal_order::root_left_right, traversal_order::root_right_left,
        traversal_order::left_right_root, traversal_order::left_root_right,
        traversal_order::right_left_root, traversal_order::right_root_left
    };

    avl_tree<int> tree;
    for (auto order : orders) {
        int calls = 0;
        tree.traverse(order, [&](const int&) { ++calls; });
        EXPECT_EQ(calls, 0);
    }

    tree.insert(42);
    for (auto order : orders) {
        std::vector<int> values;
        tree.traverse(order, [&](const int& value) { values.push_back(value); });
        EXPECT_EQ(values, (std::vector<int>{42}));
    }
}

TEST(avl_traversal_test, invalid_order) {
    avl_tree<int> tree;
    int calls = 0;
    auto visitor = [&](const int&) { ++calls; };

    EXPECT_THROW(tree.traverse(static_cast<traversal_order>(-1), visitor),
        std::invalid_argument);
    tree.insert(1);
    EXPECT_THROW(tree.traverse(static_cast<traversal_order>(6), visitor),
        std::invalid_argument);
    EXPECT_EQ(calls, 0);
}

TEST(avl_traversal_test, move_only_visitor_receives_const_reference) {
    avl_tree<int> tree;
    for (int value : {2, 1, 3})
        tree.insert(value);

    int sum = 0;
    auto visitor = [scale = std::make_unique<int>(2), &sum](auto& value) {
        static_assert(std::is_same_v<decltype(value), const int&>);
        sum += value * *scale;
    };

    tree.traverse(traversal_order::left_root_right, visitor);
    EXPECT_EQ(sum, 12);
}

TEST(avl_traversal_test, visitor_exception_stops_traversal) {
    avl_tree<int> tree;
    for (int value : {2, 1, 3})
        tree.insert(value);

    int calls = 0;
    auto visitor = [&](const int&) {
        if (++calls == 2) throw std::runtime_error("visitor failed");
    };

    EXPECT_THROW(tree.traverse(traversal_order::left_root_right, visitor),
        std::runtime_error);
    EXPECT_EQ(calls, 2);
    std::vector<int> values;
    tree.traverse(traversal_order::left_root_right,
        [&](const int& value) { values.push_back(value); });
    EXPECT_EQ(values, (std::vector<int>{1, 2, 3}));
}
