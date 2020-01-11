// Copyright (C) 2019-2020 Adam Lugowski
// All Rights Reserved.

#ifndef QUADMAT_PROBLEM_GENERATOR_H
#define QUADMAT_PROBLEM_GENERATOR_H

#include "quadmat/quadmat.h"

using namespace quadmat;

#include "problem_structs.h"

/**
 * Blow up matrix dimensions.
 */
template <typename T, typename IT>
CannedMatrix<T, IT> ExpandMatrix(const CannedMatrix<T, IT>& orig, int64_t factor) {
    Shape new_shape = {
            .nrows = orig.shape.nrows * factor,
            .ncols = orig.shape.ncols * factor
    };

    std::vector<std::tuple<IT, IT, T>> new_tuples;
    std::transform(begin(orig.sorted_tuples), end(orig.sorted_tuples), std::back_inserter(new_tuples),
                   [&](std::tuple<IT, IT, T> tup) -> std::tuple<IT, IT, T> {
                       return std::tuple<IT, IT, T>(factor * std::get<0>(tup), factor * std::get<1>(tup), std::get<2>(tup));
                   });

    return CannedMatrix<T, IT>{
        .description = Join::ToString(orig.description, " expanded by ", factor, "x to ", new_shape.ToString()),
        .shape = new_shape,
        .sorted_tuples = new_tuples
    };
}

template <typename T, typename IT>
std::vector<CannedMatrix<T, IT>> GetCannedMatrices(bool only_with_files = false) {
    std::vector<CannedMatrix<T, IT>> ret;

    if (!only_with_files) {
        ret.emplace_back(CannedMatrix<T, IT>{
                .description = "10x10 empty matrix",
                .shape = {10, 10},
                .sorted_tuples = SimpleTuplesGenerator<T, IT>::GetEmptyTuples()
        });
    }

    if (!only_with_files) {
        IdentityTuplesGenerator<T, IT> gen(10);
        ret.emplace_back(CannedMatrix<T, IT>{
                .description = "10x10 identity matrix",
                .shape = {10, 10},
                .sorted_tuples = std::vector<std::tuple<IT, IT, T>>(gen.begin(), gen.end())
        });
    }

    if (!only_with_files) {
        IdentityTuplesGenerator<T, IT> gen(10);
        std::vector<std::tuple<IT, IT, T>> tuples;
        for (auto tup : gen) {
            tuples.push_back(tup);
            tuples.push_back(tup);
        }

        ret.emplace_back(CannedMatrix<T, IT>{
                .description = "10x10 identity matrix with every entry duplicated",
                .shape = {10, 10},
                .sorted_tuples = tuples
        });
    }

    {
        FullTuplesGenerator<T, IT> gen({4, 4}, 1);
        ret.emplace_back(CannedMatrix<T, IT>{
                .description = "4x4 full matrix",
                .shape = {4, 4},
                .sorted_tuples = std::vector<std::tuple<IT, IT, T>>(gen.begin(), gen.end()),
                .filename = "small_full_symmetric_pattern.mtx"
        });
    }

    {
        std::vector<std::tuple<IT, IT, T>> skew_symmetric_tuples{
                {1, 0, -2},
                {2, 0, 45},
                {0, 1, 2},
                {2, 1, 4},
                {0, 2, -45},
                {1, 2, -4},
        };

        ret.emplace_back(CannedMatrix<T, IT>{
                .description = "3x3 skew-symmetric matrix",
                .shape = {3, 3},
                .sorted_tuples = skew_symmetric_tuples,
                .filename = "skew_symmetric.mtx"
        });
    }

    {
        ret.emplace_back(CannedMatrix<T, IT>{
                .description = "Kepner-Gilbert graph",
                .shape = SimpleTuplesGenerator<T, IT>::GetKepnerGilbertGraphShape(),
                .sorted_tuples = SimpleTuplesGenerator<T, IT>::GetKepnerGilbertGraphTuples(),
                .filename = "kepner_gilbert_graph.mtx"
        });
    }

    if (!only_with_files) {
        const auto& orig = ret.back();

        // expand into 32-bit indices
        ret.push_back(ExpandMatrix(orig, 10000));

        // expand into 64-bit indices and sparse SpA
        ret.push_back(ExpandMatrix(orig, 5000000000));
    }

    if (!only_with_files) {
        // same as above but with extra sparsity
        Shape orig_shape = SimpleTuplesGenerator<T, IT>::GetKepnerGilbertGraphShape();

        auto orig_tuples = SimpleTuplesGenerator<T, IT>::GetKepnerGilbertGraphTuples();

        std::vector<std::tuple<IT, IT, T>> tuples;
        std::transform(begin(orig_tuples), end(orig_tuples), std::back_inserter(tuples),
                       [](std::tuple<IT, IT, T> tup) -> std::tuple<IT, IT, T> {
                           return std::tuple<IT, IT, T>(2 * std::get<0>(tup), 2 * std::get<1>(tup), std::get<2>(tup));
                       });

        ret.emplace_back(CannedMatrix<T, IT>{
                .description = "Double Sparsity Kepner-Gilbert graph",
                .shape = {orig_shape.nrows*2 + 1, orig_shape.ncols*2 + 1},
                .sorted_tuples = tuples
        });
    }

    return ret;
}

/**
 * Blow up dimensions of a multiplication problem
 */
template <typename T, typename IT>
MultiplyProblem<T, IT> ExpandMultiplyProblem(const MultiplyProblem<T, IT>& orig, int factor) {
    return MultiplyProblem<T, IT>{
            .description = Join::ToString(orig.description, " expanded by ", factor, "x"),
            .a = ExpandMatrix(orig.a, factor),
            .b = ExpandMatrix(orig.b, factor),
            .result = ExpandMatrix(orig.result, factor),
    };
}

template <typename T, typename IT>
std::vector<MultiplyProblem<T, IT>> GetCannedMultiplyProblems() {
    std::vector<MultiplyProblem<T, IT>> ret;

    {
        ret.emplace_back(MultiplyProblem<T, IT>{
                .description = "empty square matrix squared",
                .a = {
                        .shape = {10, 10},
                        .sorted_tuples = SimpleTuplesGenerator<T, IT>::GetEmptyTuples()
                },
                .b = {
                        .shape = {10, 10},
                        .sorted_tuples = SimpleTuplesGenerator<T, IT>::GetEmptyTuples()
                },
                .result = {
                        .shape = {10, 10},
                        .sorted_tuples = SimpleTuplesGenerator<T, IT>::GetEmptyTuples()
                },
        });
    }

    {
        IdentityTuplesGenerator<T, IT> gen(10);
        ret.emplace_back(MultiplyProblem<T, IT>{
                .description = "10x10 identity matrix squared",
                .a = {
                        .shape = {10, 10},
                        .sorted_tuples = std::vector<std::tuple<IT, IT, T>>(gen.begin(), gen.end()),
                },
                .b = {
                        .shape = {10, 10},
                        .sorted_tuples = std::vector<std::tuple<IT, IT, T>>(gen.begin(), gen.end()),
                },
                .result = {
                        .shape = {10, 10},
                        .sorted_tuples = std::vector<std::tuple<IT, IT, T>>(gen.begin(), gen.end()),
                },
        });
    }

    {
        IdentityTuplesGenerator<T, IT> identity(4);
        std::vector<std::tuple<IT, IT, T>> quadrant_tuples{
                {1, 0, 1.0},
                {3, 0, 1.0},
                {0, 3, 1.0},
                {2, 3, 1.0}
        };

        ret.emplace_back(MultiplyProblem<T, IT>{
                .description = "4x4 top with empty columns * identity",
                .a = {
                        .shape = {4, 4},
                        .sorted_tuples = quadrant_tuples,
                },
                .b = {
                        .shape = {4, 4},
                        .sorted_tuples = std::vector<std::tuple<IT, IT, T>>(identity.begin(), identity.end()),
                },
                .result = {
                        .shape = {4, 4},
                        .sorted_tuples = quadrant_tuples,
                },
        });
    }

    {
        FullTuplesGenerator<T, IT> gen_factor({4, 4}, 1);
        FullTuplesGenerator<T, IT> gen_result({4, 4}, 4);

        ret.emplace_back(MultiplyProblem<T, IT>{
                .description = "4x4 full matrix squared",
                .a = {
                        .shape = {4, 4},
                        .sorted_tuples = std::vector<std::tuple<IT, IT, T>>(gen_factor.begin(), gen_factor.end()),
                },
                .b = {
                        .shape = {4, 4},
                        .sorted_tuples = std::vector<std::tuple<IT, IT, T>>(gen_factor.begin(), gen_factor.end()),
                },
                .result = {
                        .shape = {4, 4},
                        .sorted_tuples = std::vector<std::tuple<IT, IT, T>>(gen_result.begin(), gen_result.end()),
                },
        });
    }

    {
        IdentityTuplesGenerator<T, IT> identity(7);

        ret.emplace_back(MultiplyProblem<T, IT>{
                .description = "identity * Kepner-Gilbert graph",
                .a = {
                        .shape = {7, 7},
                        .sorted_tuples = std::vector<std::tuple<IT, IT, T>>(identity.begin(), identity.end()),
                },
                .b = {
                        .shape = SimpleTuplesGenerator<T, IT>::GetKepnerGilbertGraphShape(),
                        .sorted_tuples = SimpleTuplesGenerator<T, IT>::GetKepnerGilbertGraphTuples(),
                },
                .result = {
                        .shape = SimpleTuplesGenerator<T, IT>::GetKepnerGilbertGraphShape(),
                        .sorted_tuples = SimpleTuplesGenerator<T, IT>::GetKepnerGilbertGraphTuples(),
                },
        });
    }

    {
        IdentityTuplesGenerator<T, IT> identity(7);

        ret.emplace_back(MultiplyProblem<T, IT>{
                .description = "Kepner-Gilbert graph * identity",
                .a = {
                        .shape = SimpleTuplesGenerator<T, IT>::GetKepnerGilbertGraphShape(),
                        .sorted_tuples = SimpleTuplesGenerator<T, IT>::GetKepnerGilbertGraphTuples(),
                },
                .b = {
                        .shape = {7, 7},
                        .sorted_tuples = std::vector<std::tuple<IT, IT, T>>(identity.begin(), identity.end()),
                },
                .result = {
                        .shape = SimpleTuplesGenerator<T, IT>::GetKepnerGilbertGraphShape(),
                        .sorted_tuples = SimpleTuplesGenerator<T, IT>::GetKepnerGilbertGraphTuples(),
                },
        });
    }

    {
        IdentityTuplesGenerator<T, IT> identity(10);

        ret.emplace_back(MultiplyProblem<T, IT>{
                .description = "10x10 Kepner-Gilbert graph * identity",
                .a = {
                        .shape = {10, 10},
                        .sorted_tuples = SimpleTuplesGenerator<T, IT>::GetKepnerGilbertGraphTuples(),
                },
                .b = {
                        .shape = {10, 10},
                        .sorted_tuples = std::vector<std::tuple<IT, IT, T>>(identity.begin(), identity.end()),
                },
                .result = {
                        .shape = {10, 10},
                        .sorted_tuples = SimpleTuplesGenerator<T, IT>::GetKepnerGilbertGraphTuples(),
                },
        });
    }

    {
        std::vector<std::tuple<IT, IT, T>> row_vector;
        std::vector<std::tuple<IT, IT, T>> col_vector;
        const size_t length = 16;

        for (IT i = 0; i < length; i++) {
            row_vector.emplace_back(0, i, 1);
            col_vector.emplace_back(i, 0, 1);
        }

        std::vector<std::tuple<IT, IT, T>> dot_product{
                {0, 0, length}
        };

        FullTuplesGenerator<T, IT> cross_product({length, length}, 1);

        // dot product: row vector * col vector
        ret.emplace_back(MultiplyProblem<T, IT>{
                .description = Join::ToString("vector dot product length ", length),
                .a = {
                        .shape = {1, length},
                        .sorted_tuples = row_vector,
                },
                .b = {
                        .shape = {length, 1},
                        .sorted_tuples = col_vector,
                },
                .result = {
                        .shape = {1, 1},
                        .sorted_tuples = dot_product,
                },
        });

        // cross product: col vector * row vector
        ret.emplace_back(MultiplyProblem<T, IT>{
                .description = Join::ToString("vector cross product length ", length),
                .a = {
                        .shape = {length, 1},
                        .sorted_tuples = col_vector,
                },
                .b = {
                        .shape = {1, length},
                        .sorted_tuples = row_vector,
                },
                .result = {
                        .shape = {length, length},
                        .sorted_tuples = std::vector<std::tuple<IT, IT, T>>(cross_product.begin(), cross_product.end()),
                },
        });
    }

    return ret;
}

#endif //QUADMAT_PROBLEM_GENERATOR_H
