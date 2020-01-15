// Copyright (C) 2019-2020 Adam Lugowski
// All Rights Reserved.

#ifndef QUADMAT_BENCH_BENCHMARK_UTILITIES_H_
#define QUADMAT_BENCH_BENCHMARK_UTILITIES_H_

#include <random>
#include <vector>

/**
 * Generate a random value. Simulates beta values.
 */
template <typename T>
T RandomValue() {
    std::default_random_engine rng; // NOLINT(cert-msc32-c,cert-msc51-cpp)
    std::uniform_real_distribution<T> distribution(0, 100);
    return distribution(rng);
}

/**
 * Generate a vector of random ints from a random uniform distribution.
 *
 * Simulates the row indices of a column.
 */
template <typename IT>
std::vector<IT> GenerateUniformIndices(std::size_t spa_size, double fill_fraction, int seed = 0, bool sorted = true) {
    std::size_t length = spa_size * fill_fraction;

    std::vector<IT> ret(length);

    auto rng = std::mt19937(seed);
    std::uniform_int_distribution<int32_t> dist {0, IT(spa_size - 1)};

    auto gen = [&dist, &rng](){
        return dist(rng);
    };

    generate(begin(ret), end(ret), gen);

    // quadmat columns are sorted by row index
    if (sorted) {
        std::sort(begin(ret), end(ret));
    }

    return ret;
}

/**
 * Generates multiple vectors from GenerateUniformIndices().
 *
 * Simulates multiple columns.
 */
template <typename IT>
std::vector<std::vector<IT>> GenerateMultipleUniformIndices(int how_many, std::size_t spa_size, double fill_fraction) {
    std::vector<std::vector<IT>> ret(how_many);

    for (int i = 0; i < how_many; ++i) {
        ret[i] = GenerateUniformIndices<IT>(spa_size, fill_fraction, i);
    }

    return ret;
}

#endif //QUADMAT_BENCH_BENCHMARK_UTILITIES_H_
