// Copyright (C) 2019-2020 Adam Lugowski
// All Rights Reserved.

#include <benchmark/benchmark.h>

#include "quadmat/quadmat.h"

using namespace quadmat;

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
std::vector<IT> GenerateUniformIndices(std::size_t spa_size, double fill_fraction, int seed = 0) {
    std::size_t length = spa_size * fill_fraction;

    std::vector<IT> ret(length);

    auto rng = std::mt19937(seed);
    std::uniform_int_distribution<int32_t> dist {0, IT(spa_size - 1)};

    auto gen = [&dist, &rng](){
        return dist(rng);
    };

    generate(begin(ret), end(ret), gen);

    // quadmat columns are sorted by row index
    std::sort(begin(ret), end(ret));

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
        ret[i] = GenerateUniformIndices<IT>(spa_size, fill_fraction);
    }

    return ret;
}

/**
 * Fastest scatter possible.
 */
static void BM_ScatterRoofline(benchmark::State& state) {
    const std::size_t spa_size = state.range(0);
    double fill_fraction = state.range(1) / 1000.0;

    using IndexType = int32_t;
    using ValueType = double;

    const auto indices = GenerateUniformIndices<IndexType>(spa_size, fill_fraction);
    const std::vector<ValueType> values(indices.size(), 1.0);
    auto beta = RandomValue<ValueType>();

    double num_updates = 0;

    std::vector<ValueType> x(spa_size);
    for (auto _ : state) {
        for (auto i = 0; i < indices.size(); ++i) {
            x[indices[i]] = x[indices[i]] + beta * values[i];
        }
        num_updates += indices.size();
    }

    unsigned bits = 8u * sizeof(IndexType);
    state.counters["IndexBits"] = benchmark::Counter(bits);
    state.counters["Fill%"] = benchmark::Counter(fill_fraction * 100);
    state.counters["FLOPS"] = benchmark::Counter(2*num_updates, benchmark::Counter::kIsRate);
}

/**
 * Benchmark only the Scatter method of a spa.
 *
 * @tparam Spa
 * @param state
 */
template <typename Spa>
static void BM_SpaScatterOnly(benchmark::State& state) {
    const std::size_t spa_size = state.range(0);
    double fill_fraction = state.range(1) / 1000.0;
    int num_vecs = state.range(2);

    using IndexType = typename Spa::IndexType;
    using ValueType = typename Spa::ValueType;

    const auto indices_arr = GenerateMultipleUniformIndices<IndexType>(num_vecs, spa_size, fill_fraction);
    const std::vector<ValueType> values(indices_arr[0].size(), 1.0);
    auto beta = RandomValue<ValueType>();

    double num_updates = 0;

    Spa spa(spa_size);
    for (auto _ : state) {
        for (const auto& indices : indices_arr) {
            spa.Scatter(begin(indices), end(indices), begin(values), beta);
            num_updates += indices.size();
        }
    }

    unsigned bits = 8u * sizeof(IndexType);
    state.counters[std::to_string(bits) + "-bit"] = benchmark::Counter(bits); // for automatic grouping
    state.counters["IndexBits"] = benchmark::Counter(bits);
    state.counters["Fill%"] = benchmark::Counter(fill_fraction * 100);
    state.counters["FLOPS"] = benchmark::Counter(2*num_updates, benchmark::Counter::kIsRate);
}

/**
 * Benchmark full operation of a spa, including reading and clearing.
 *
 * @tparam Spa
 * @param state
 */
template <typename Spa>
static void BM_Spa(benchmark::State& state) {
    const std::size_t spa_size = state.range(0);
    double fill_fraction = state.range(1) / 1000.0;
    int num_vecs = state.range(2);

    using IndexType = typename Spa::IndexType;
    using ValueType = typename Spa::ValueType;

    const auto indices_arr = GenerateMultipleUniformIndices<IndexType>(num_vecs, spa_size, fill_fraction);
    const std::vector<ValueType> values(indices_arr[0].size(), 1.0);
    auto beta = RandomValue<ValueType>();

    double num_updates = 0;

    Spa spa(spa_size);
    for (auto _ : state) {
        std::vector<IndexType> out_rows;
        std::vector<ValueType> out_values;

        for (const auto& indices : indices_arr) {
            spa.Scatter(begin(indices), end(indices), begin(values), beta);
            num_updates += indices.size();
        }
        spa.EmplaceBackResult(out_rows, out_values);
        spa.Clear();

        benchmark::DoNotOptimize(out_rows.data());
        benchmark::DoNotOptimize(out_values.data());
    }

    unsigned bits = 8u * sizeof(IndexType);
    state.counters["IndexBits"] = benchmark::Counter(bits);
    state.counters["Fill%"] = benchmark::Counter(fill_fraction * 100);
    // FLOPS that the user sees. Includes all overhead.
    state.counters["VisFLOPS"] = benchmark::Counter(2*num_updates, benchmark::Counter::kIsRate);
}

#pragma clang diagnostic push
#pragma ide diagnostic ignored "cert-err58-cpp"

// Common configurations of all the parameters.
// *Small are small-spa configurations. This means a higher fill rate can be tested
// *Large are large spa configurations. Only test smaller fill rates because
//   a) large matrices are likely to have smaller fill rates anyway
//   b) the tests would otherwise take too long
const std::pair<int64_t, int64_t> kSpaSizeRange16 = {1u << 10u, 1u << 15u};
const std::pair<int64_t, int64_t> kSpaSizeRange32Small = {1u << 10u, 1u << 22u};
const std::pair<int64_t, int64_t> kSpaSizeRange32Large = {1u << 22u, 1u << 26u};

const std::pair<int64_t, int64_t> kFillFractionNumeratorRangeSmall = {1, 1000}; // 0.1% to 100%
const std::pair<int64_t, int64_t> kFillFractionNumeratorRangeLarge = {1, 50}; // 0.1% to 1%

const std::pair<int64_t, int64_t> kHowManySmall = {10, 10};
const std::pair<int64_t, int64_t> kHowManyLarge = {2, 2};

const std::vector<std::pair<int64_t, int64_t>> k16bit = {kSpaSizeRange16, kFillFractionNumeratorRangeSmall, kHowManySmall};
const std::vector<std::pair<int64_t, int64_t>> k32bitSmall = {kSpaSizeRange32Small, kFillFractionNumeratorRangeSmall, kHowManySmall};
const std::vector<std::pair<int64_t, int64_t>> k32bitLarge = {kSpaSizeRange32Large, kFillFractionNumeratorRangeLarge, kHowManyLarge};

// Roofline benchmark to see what the system is capable of best case
BENCHMARK(BM_ScatterRoofline)->Ranges(k32bitSmall);
BENCHMARK(BM_ScatterRoofline)->Ranges(k32bitLarge);

// Just the scatter operation but with the overhead of the spa
// For dense spa the scatter is the same, but it needs to keep track of which entries have been accessed
// For sparse spa, there is no relation.

// 16-bit spa
BENCHMARK_TEMPLATE(BM_SpaScatterOnly, DenseSpa<int16_t, PlusTimesSemiring<double>>)->Ranges(k16bit);
BENCHMARK_TEMPLATE(BM_SpaScatterOnly, MapSpa<int16_t, PlusTimesSemiring<double>>)->Ranges(k16bit);

// 32-bit spa
BENCHMARK_TEMPLATE(BM_SpaScatterOnly, DenseSpa<int32_t, PlusTimesSemiring<double>>)->Ranges(k32bitSmall);
BENCHMARK_TEMPLATE(BM_SpaScatterOnly, MapSpa<int32_t, PlusTimesSemiring<double>>)->Ranges(k32bitSmall);
BENCHMARK_TEMPLATE(BM_SpaScatterOnly, DenseSpa<int32_t, PlusTimesSemiring<double>>)->Ranges(k32bitLarge);
BENCHMARK_TEMPLATE(BM_SpaScatterOnly, MapSpa<int32_t, PlusTimesSemiring<double>>)->Ranges(k32bitLarge);

// Full use of the spa, including reading back results and clearing the spa.
BENCHMARK_TEMPLATE(BM_Spa, DenseSpa<int32_t, PlusTimesSemiring<double>>)->Ranges(k32bitSmall);
BENCHMARK_TEMPLATE(BM_Spa, MapSpa<int32_t, PlusTimesSemiring<double>>)->Ranges(k32bitSmall);
BENCHMARK_TEMPLATE(BM_Spa, DenseSpa<int32_t, PlusTimesSemiring<double>>)->Ranges(k32bitLarge);
BENCHMARK_TEMPLATE(BM_Spa, MapSpa<int32_t, PlusTimesSemiring<double>>)->Ranges(k32bitLarge);


#pragma clang diagnostic pop


BENCHMARK_MAIN();
