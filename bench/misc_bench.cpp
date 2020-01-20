// Copyright (C) 2019-2020 Adam Lugowski
// All Rights Reserved.

#include <benchmark/benchmark.h>

#include <vector>

#include "benchmark_utilities.h"

// ClangTidy doesn't identify template uses
#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection"

// ClangTidy complains about static fields
#pragma clang diagnostic push
#pragma ide diagnostic ignored "cert-err58-cpp"


/**
 * Example benchmark
 */
static void BM_Example(benchmark::State& state) {
    std::size_t num_elements = 0;
    const auto vec = std::vector<int_fast32_t>(state.range(0));

    for (auto _ : state) {
        auto sum = vec[0];
        for (const auto& value : vec) {
            sum += value;
        }

        benchmark::DoNotOptimize(sum);
        num_elements += vec.size();
    }

    state.counters["Elements"] = benchmark::Counter(num_elements, benchmark::Counter::kIsRate);
    state.counters["Bytes"] = benchmark::Counter(double(sizeof(vec[0]) * num_elements), benchmark::Counter::kIsRate);
}

BENCHMARK(BM_Example)->Range(8, 1u << 15u);


#pragma clang diagnostic pop // static
#pragma clang diagnostic pop // bad unused method detection

BENCHMARK_MAIN();
