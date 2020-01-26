// Copyright (C) 2019-2020 Adam Lugowski
// All Rights Reserved.

#include <iostream>
#include <benchmark/benchmark.h>

#include "quadmat/quadmat.h"

using namespace quadmat;

#include "../test/test_utilities/problems.h"

// ClangTidy complains about static fields
#pragma clang diagnostic push
#pragma ide diagnostic ignored "cert-err58-cpp"

/**
 * Read problems
 */
static const auto kMultiplyProblems = GetFsMultiplyProblems("medium");

static FsMultiplyProblem FindProblem(const std::string& problem_name) {
    auto pos = std::find_if(std::begin(kMultiplyProblems), std::end(kMultiplyProblems),
                            [&](const FsMultiplyProblem & problem) -> bool { return problem.description == problem_name; });

    if (pos == std::end(kMultiplyProblems)) {
        return {};
    } else {
        return *pos;
    }
}

/**
 * Test effect of LeafSplitThreshold on matrix multiply
 */
static void BM_Multiply(benchmark::State& state, const std::string& problem_name) {
    auto problem = FindProblem(problem_name);

    std::ifstream a_stream{problem.a_path};
    auto a = MatrixMarket::Load(a_stream);

    std::ifstream b_stream{problem.b_path};
    auto b = MatrixMarket::Load(b_stream);

    for (auto _ : state) {
        // multiply
        auto result = Multiply<PlusTimesSemiring<double>>(a, b);
        benchmark::DoNotOptimize(result);
    }
}

/**
 * Test effect of LeafSplitThreshold on matrix triple product
 */
static void BM_TripleProduct(benchmark::State& state, const std::string& problem_name) {
    auto problem = FindProblem(problem_name);

    std::ifstream a_stream{problem.a_path};
    auto a = MatrixMarket::Load(a_stream);

    std::ifstream b_stream{problem.b_path};
    auto b = MatrixMarket::Load(b_stream);

    std::ifstream c_stream{problem.c_path};
    auto c = MatrixMarket::Load(c_stream);

    for (auto _ : state) {
        // multiply
        auto result_ab = Multiply<PlusTimesSemiring<double>>(a, b);
        auto result = Multiply<PlusTimesSemiring<double>>(result_ab, c);
        benchmark::DoNotOptimize(result);
    }
}

// Common configurations of all the parameters.
static void CommonArguments(benchmark::internal::Benchmark* b) {
    b->Unit(benchmark::kMillisecond);
}

// Multiply benchmarks use medium test problems. These problems are large and are not included in Git.
// See test/README.md for how to generate these problems.
BENCHMARK_CAPTURE(BM_Multiply, row_perm_3Dtorus_50, std::string("row_perm 3Dtorus scale 50"))->Apply(CommonArguments);
BENCHMARK_CAPTURE(BM_Multiply, row_perm_3DtorusRP_50, std::string("row_perm 3DtorusRP scale 50"))->Apply(CommonArguments);
BENCHMARK_CAPTURE(BM_Multiply, square_3Dtorus_50, std::string("square 3Dtorus scale 50"))->Apply(CommonArguments);
BENCHMARK_CAPTURE(BM_Multiply, square_3DtorusRP_50, std::string("square 3DtorusRP scale 50"))->Apply(CommonArguments);
BENCHMARK_CAPTURE(BM_Multiply, square_ER_12, std::string("square ER scale 12"))->Apply(CommonArguments);

BENCHMARK_CAPTURE(BM_TripleProduct, submatrix_ER_12, std::string("submatrix ER scale 12"))->Apply(CommonArguments);

#pragma clang diagnostic pop

BENCHMARK_MAIN();
