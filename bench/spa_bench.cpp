// Copyright (C) 2019-2020 Adam Lugowski
// All Rights Reserved.

#include <benchmark/benchmark.h>

#include "quadmat/quadmat.h"
#include "benchmark_utilities.h"

using namespace quadmat;

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
        spa.Gather(out_rows, out_values);
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

const auto kFillFractionNumeratorRangeSmall = {1, 10, 50, 500, 1000}; // 0.1% to 100%
const auto kFillFractionNumeratorRangeLarge = {1, 10, 50}; // 0.1% to 5%

const auto kNumVecsSmall = 10;
const auto kNumVecsLarge = 2;

static void SpaArguments16Bit(benchmark::internal::Benchmark* b) {
    b->ArgNames({"spa_size", "fill_frac*1000", "num_vecs"});

    for (auto spa_size : {
        1u << 10u,
        1u << 12u,
        1u << 15u
    }) {
        for (auto fill_frac_numerator : kFillFractionNumeratorRangeSmall) {
            auto num_vecs = kNumVecsSmall;

            b->Args({spa_size, fill_frac_numerator, num_vecs});
        }
    }
}

static void SpaArguments32Bit(benchmark::internal::Benchmark *b) {
    b->ArgNames({"spa_size", "fill_frac*1000", "num_vecs"});

    // Small-spa configurations. This means a higher fill rate can be tested
    for (auto spa_size : {
        1u << 10u,
        1u << 14u,
        1u << 18u,
        1u << 20u,
        1u << 22u,
    }) {
        for (auto fill_frac_numerator : kFillFractionNumeratorRangeSmall) {
            auto num_vecs = kNumVecsSmall;

            b->Args({spa_size, fill_frac_numerator, num_vecs});
        }
    }

    // Large spa configurations. Only test smaller fill rates because
    //   a) large matrices are likely to have smaller fill rates anyway
    //   b) the tests would otherwise take too long
    for (auto spa_size : {
        1u << 24u,
        1u << 26u,
    }) {
        for (auto fill_frac_numerator : kFillFractionNumeratorRangeLarge) {
            auto num_vecs = kNumVecsLarge;

            b->Args({spa_size, fill_frac_numerator, num_vecs});
        }
    }
}

// Roofline benchmark to see what the system is capable of best case
BENCHMARK(BM_ScatterRoofline)->Apply(SpaArguments32Bit);

// Just the scatter operation but with the overhead of the spa
// For dense spa the scatter is the same, but it needs to keep track of which entries have been accessed
// For sparse spa, there is no relation.

// 16-bit spa
BENCHMARK_TEMPLATE(BM_SpaScatterOnly, DenseSpa<int16_t, PlusTimesSemiring<double>>)->Apply(SpaArguments16Bit);
BENCHMARK_TEMPLATE(BM_SpaScatterOnly, MapSpa<int16_t, PlusTimesSemiring<double>>)->Apply(SpaArguments16Bit);

// 32-bit spa
BENCHMARK_TEMPLATE(BM_SpaScatterOnly, DenseSpa<int32_t, PlusTimesSemiring<double>>)->Apply(SpaArguments32Bit);
BENCHMARK_TEMPLATE(BM_SpaScatterOnly, MapSpa<int32_t, PlusTimesSemiring<double>>)->Apply(SpaArguments32Bit);

// Full use of the spa, including reading back results and clearing the spa.
BENCHMARK_TEMPLATE(BM_Spa, DenseSpa<int32_t, PlusTimesSemiring<double>>)->Apply(SpaArguments32Bit);
BENCHMARK_TEMPLATE(BM_Spa, MapSpa<int32_t, PlusTimesSemiring<double>>)->Apply(SpaArguments32Bit);


#pragma clang diagnostic pop


BENCHMARK_MAIN();
