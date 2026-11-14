#include "../console/generator.hpp"

#include <gtest/gtest.h>

#include <limits>
#include <sstream>
#include <vector>

TEST(generator_test, fills_inclusive_range_without_duplicates) {
    auto tree = generate_tree(10, -4, 5, 42);
    std::vector<int> values;
    tree.traverse(traversal_order::left_root_right, [&](int value) {
        values.push_back(value);
    });

    EXPECT_EQ(values, (std::vector<int>{-4, -3, -2, -1, 0, 1, 2, 3, 4, 5}));
}

TEST(generator_test, respects_count_and_range) {
    auto tree = generate_tree(10, -100, 100, 42);

    EXPECT_EQ(tree.size(), 10u);
    tree.traverse(traversal_order::left_root_right, [](int value) {
        EXPECT_GE(value, -100);
        EXPECT_LE(value, 100);
    });
}

TEST(generator_test, seed_reproduces_structure) {
    auto first = generate_tree(10, -100, 100, 42);
    generate_tree(10, -100, 100, 123);
    auto second = generate_tree(10, -100, 100, 42);
    std::ostringstream first_output;
    std::ostringstream second_output;
    auto formatter = [](std::ostream& stream, int value) { stream << value; };
    first.print_tree(first_output, formatter);
    second.print_tree(second_output, formatter);

    EXPECT_EQ(first_output.str(), second_output.str());
}

TEST(generator_test, zero_count_creates_empty_tree) {
    EXPECT_TRUE(generate_tree(0, 1, 10, 42).empty());
}

TEST(generator_test, rejects_invalid_parameters) {
    EXPECT_THROW(generate_tree(1, 5, 4, 42), std::invalid_argument);
    EXPECT_THROW(generate_tree(11, 1, 10, 42), std::invalid_argument);
}

TEST(generator_test, handles_integer_limits) {
    constexpr int minimum = std::numeric_limits<int>::min();
    constexpr int maximum = std::numeric_limits<int>::max();

    EXPECT_TRUE(generate_tree(1, minimum, minimum, 42).contains(minimum));
    EXPECT_TRUE(generate_tree(1, maximum, maximum, 42).contains(maximum));
    EXPECT_EQ(generate_tree(10, minimum, maximum, 42).size(), 10u);
}
