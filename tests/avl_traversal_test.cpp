#include "avl_tree.hpp"

#include <gtest/gtest.h>

#include <array>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <type_traits>
#include <vector>

TEST(avl_print_test, empty_tree) {
    const avl_tree<int> tree;
    std::ostringstream output;
    int calls = 0;
    tree.print_tree(output, [&](std::ostream& stream, const int& value) {
        ++calls;
        stream << value;
    });

    EXPECT_EQ(output.str(), "(пусто)\n");
    EXPECT_EQ(calls, 0);
}

TEST(avl_print_test, single_node) {
    avl_tree<int> tree;
    tree.insert(42);
    std::ostringstream output;
    tree.print_tree(output, [](std::ostream& stream, const int& value) {
        stream << value;
    });

    EXPECT_EQ(output.str(), "42\n");
}

TEST(avl_print_test, both_children_and_indentation) {
    avl_tree<int> tree;
    for (int value : {4, 2, 6, 1, 3, 5, 7})
        tree.insert(value);
    std::ostringstream output;
    tree.print_tree(output, [](std::ostream& stream, const int& value) {
        stream << value;
    });

    EXPECT_EQ(output.str(),
        "4\n"
        "├── L: 2\n"
        "│   ├── L: 1\n"
        "│   └── R: 3\n"
        "└── R: 6\n"
        "    ├── L: 5\n"
        "    └── R: 7\n");
}

TEST(avl_print_test, single_child_keeps_side_label) {
    for (int child : {1, 3}) {
        SCOPED_TRACE(child);
        avl_tree<int> tree;
        tree.insert(2);
        tree.insert(child);
        std::ostringstream output;
        tree.print_tree(output, [](std::ostream& stream, const int& value) {
            stream << value;
        });

        EXPECT_EQ(output.str(), child == 1 ? "2\n└── L: 1\n" : "2\n└── R: 3\n");
    }
}

TEST(avl_print_test, custom_formatter_without_copying) {
    struct item {
        int key;
        std::unique_ptr<int> payload;
    };
    auto compare = [](const item& left, const item& right) {
        return left.key < right.key;
    };
    avl_tree<item, decltype(compare)> tree(compare);
    tree.insert(item{2, std::make_unique<int>(20)});
    tree.insert(item{1, std::make_unique<int>(10)});

    auto formatter = [count = std::make_unique<int>(0)](
        std::ostream& stream, auto& value
    ) mutable {
        static_assert(std::is_same_v<decltype(value), const item&>);
        stream << ++*count << ": " << value.key << " [" << *value.payload << "]";
    };
    std::ostringstream output;
    const auto& view = tree;
    view.print_tree(output, formatter);

    EXPECT_EQ(output.str(), "1: 2 [20]\n└── L: 2: 1 [10]\n");
}

TEST(avl_print_test, formatter_exception_keeps_tree) {
    avl_tree<int> tree;
    for (int value : {2, 1, 3})
        tree.insert(value);

    int calls = 0;
    auto formatter = [&](std::ostream& stream, const int& value) {
        if (++calls == 2) throw std::runtime_error("formatter failed");
        stream << value;
    };
    std::ostringstream partial;
    EXPECT_THROW(tree.print_tree(partial, formatter), std::runtime_error);
    EXPECT_EQ(calls, 2);

    std::ostringstream output;
    tree.print_tree(output, [](std::ostream& stream, const int& value) {
        stream << value;
    });
    EXPECT_EQ(output.str(), "2\n├── L: 1\n└── R: 3\n");
}

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
