#include "avl_tree.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <random>
#include <stdexcept>
#include <string_view>
#include <vector>

struct measurement {
    double time_ms;
    std::size_t checksum;
};

template<class Function>
measurement measure(Function&& function) {
    auto start = std::chrono::steady_clock::now();
    auto checksum = function();
    auto finish = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration<double, std::milli>(finish - start);
    return {elapsed.count(), checksum};
}

void run_benchmarks(std::ostream& output) {
    constexpr std::array sizes = {10000, 30000, 100000, 300000, 1000000};
    constexpr unsigned seed = 42;
    constexpr int repeats = 5;
    constexpr std::array operations = {"insert", "find_present", "find_absent", "erase"};

    output << "size,order,operation,repeat,seed,time_ms,checksum\n";
    output << std::fixed << std::setprecision(6);
    for (int size : sizes) {
        for (std::string_view order : {"ascending", "random"}) {
            std::vector<int> values(size);
            std::iota(values.begin(), values.end(), 0);
            if (order == "random") {
                std::mt19937 engine(seed);
                std::shuffle(values.begin(), values.end(), engine);
            }
            std::vector<int> missing(size);
            std::transform(values.begin(), values.end(), missing.begin(), [](int value) {
                return -value - 1;
            });

            for (int repeat = 1; repeat <= repeats; ++repeat) {
                avl_tree<int> tree;
                auto insertion = measure([&] {
                    std::size_t checksum = 0;
                    for (int value : values)
                        checksum += tree.insert(value);
                    return checksum;
                });
                auto present = measure([&] {
                    std::size_t checksum = 0;
                    for (int value : values)
                        checksum += tree.contains(value);
                    return checksum;
                });
                auto absent = measure([&] {
                    std::size_t checksum = 0;
                    for (int value : missing)
                        checksum += tree.contains(value);
                    return checksum;
                });
                auto removal = measure([&] {
                    std::size_t checksum = 0;
                    for (int value : values)
                        checksum += tree.erase(value);
                    return checksum;
                });

                auto expected = static_cast<std::size_t>(size);
                if (insertion.checksum != expected || present.checksum != expected ||
                    absent.checksum != 0 || removal.checksum != expected || !tree.empty())
                    throw std::runtime_error("Неверная контрольная сумма замера.");

                const std::array results = {insertion, present, absent, removal};
                for (std::size_t i = 0; i < results.size(); ++i) {
                    output << size << ',' << order << ',' << operations[i] << ','
                        << repeat << ',' << seed << ',' << results[i].time_ms << ','
                        << results[i].checksum << '\n';
                }
                std::cout << size << ' ' << order << ' ' << repeat << '/' << repeats
                    << '\n';
            }
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc > 2) {
        std::cerr << "Использование: avl_benchmark [output.csv]\n";
        return 1;
    }

    try {
        std::filesystem::path path = argc == 2
            ? argv[1] : "benchmarks/results/raw.csv";
        if (path.has_parent_path())
            std::filesystem::create_directories(path.parent_path());
        std::ofstream output;
        output.exceptions(std::ios::failbit | std::ios::badbit);
        output.open(path);
        run_benchmarks(output);
        output.close();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
