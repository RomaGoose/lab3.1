#pragma once

#include "avl_tree.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <vector>

struct avl_tree_test_access {
    template<class Node, class T>
    static int check_node(const Node* current, std::vector<const T*>& values) {
        if (!current) return 0;

        int left_height = check_node(current->left.get(), values);
        values.push_back(&current->value);
        int right_height = check_node(current->right.get(), values);
        int height = 1 + std::max(left_height, right_height);

        EXPECT_EQ(current->height, height);
        EXPECT_LE(std::abs(left_height - right_height), 1);
        return height;
    }

    template<class T, class Compare>
    static void check(const avl_tree<T, Compare>& tree) {
        std::vector<const T*> values;
        check_node(tree.root.get(), values);
        EXPECT_EQ(values.size(), tree.size());
        for (std::size_t i = 1; i < values.size(); ++i)
            EXPECT_TRUE(tree.compare(*values[i - 1], *values[i]));
    }

    static int root_value(const avl_tree<int>& tree) {
        return tree.root->value;
    }
};

