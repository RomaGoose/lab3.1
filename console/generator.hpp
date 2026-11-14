#pragma once

#include "avl_tree.hpp"

#include <cstddef>
#include <cstdint>
#include <random>
#include <stdexcept>

inline avl_tree<int> generate_tree(
    std::size_t count, int minimum, int maximum, std::uint32_t seed
) {
    if (minimum > maximum)
        throw std::invalid_argument("Нижняя граница больше верхней.");

    auto capacity = static_cast<std::uint64_t>(
        static_cast<std::int64_t>(maximum) - minimum + 1
    );
    if (count > capacity)
        throw std::invalid_argument("В диапазоне недостаточно уникальных чисел.");

    std::mt19937 engine(seed);
    std::uniform_int_distribution<int> distribution(minimum, maximum);
    avl_tree<int> tree;
    while (tree.size() < count)
        tree.insert(distribution(engine));
    return tree;
}
