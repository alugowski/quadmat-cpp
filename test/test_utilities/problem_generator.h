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
canned_matrix<T, IT> expand_matrix(const canned_matrix<T, IT>& orig, int factor) {
    shape_t new_shape = {
            .nrows = orig.shape.nrows * factor,
            .ncols = orig.shape.ncols * factor
    };

    vector<std::tuple<IT, IT, T>> new_tuples;
    std::transform(begin(orig.sorted_tuples), end(orig.sorted_tuples), std::back_inserter(new_tuples),
                   [&](tuple<IT, IT, T> tup) -> tuple<IT, IT, T> {
                       return tuple<IT, IT, T>(factor * std::get<0>(tup), factor * std::get<1>(tup), std::get<2>(tup));
                   });

    return canned_matrix<T, IT>{
        .description = orig.description + " expanded by " + std::to_string(factor) + "x to " + new_shape.to_string(),
        .shape = new_shape,
        .sorted_tuples = new_tuples
    };
}

template <typename T, typename IT>
vector<canned_matrix<T, IT>> get_canned_matrices(bool only_with_files = false) {
    vector<canned_matrix<T, IT>> ret;

    if (!only_with_files) {
        ret.emplace_back(canned_matrix<T, IT>{
                .description = "10x10 empty matrix",
                .shape = {10, 10},
                .sorted_tuples = simple_tuples_generator<T, IT>::EmptyMatrix()
        });
    }

    if (!only_with_files) {
        identity_tuples_generator<T, IT> gen(10);
        ret.emplace_back(canned_matrix<T, IT>{
                .description = "10x10 identity matrix",
                .shape = {10, 10},
                .sorted_tuples = vector<std::tuple<IT, IT, T>>(gen.begin(), gen.end())
        });
    }

    if (!only_with_files) {
        identity_tuples_generator<T, IT> gen(10);
        vector<std::tuple<IT, IT, T>> tuples;
        for (auto tup : gen) {
            tuples.push_back(tup);
            tuples.push_back(tup);
        }

        ret.emplace_back(canned_matrix<T, IT>{
                .description = "10x10 identity matrix with every entry duplicated",
                .shape = {10, 10},
                .sorted_tuples = tuples
        });
    }

    {
        full_tuples_generator<T, IT> gen({4, 4}, 1);
        ret.emplace_back(canned_matrix<T, IT>{
                .description = "4x4 full matrix",
                .shape = {4, 4},
                .sorted_tuples = vector<std::tuple<IT, IT, T>>(gen.begin(), gen.end()),
                .filename = "small_full_symmetric_pattern.mtx"
        });
    }

    {
        vector<tuple<IT, IT, T>> skew_symmetric_tuples{
                {1, 0, -2},
                {2, 0, 45},
                {0, 1, 2},
                {2, 1, 4},
                {0, 2, -45},
                {1, 2, -4},
        };

        ret.emplace_back(canned_matrix<T, IT>{
                .description = "3x3 skew-symmetric matrix",
                .shape = {3, 3},
                .sorted_tuples = skew_symmetric_tuples,
                .filename = "skew_symmetric.mtx"
        });
    }

    {
        ret.emplace_back(canned_matrix<T, IT>{
                .description = "Kepner-Gilbert graph",
                .shape = simple_tuples_generator<T, IT>::KepnerGilbertGraph_shape(),
                .sorted_tuples = simple_tuples_generator<T, IT>::KepnerGilbertGraph(),
                .filename = "kepner_gilbert_graph.mtx"
        });
    }

    if (!only_with_files) {
        const auto& orig = ret.back();

        // expand into 32-bit indices
        ret.push_back(expand_matrix(orig, 10000));

        // expand into sparse SpA
        ret.push_back(expand_matrix(orig, 5000000));
    }

    if (!only_with_files) {
        // same as above but with extra sparsity
        shape_t orig_shape = simple_tuples_generator<T, IT>::KepnerGilbertGraph_shape();

        auto orig_tuples = simple_tuples_generator<T, IT>::KepnerGilbertGraph();

        vector<std::tuple<IT, IT, T>> tuples;
        std::transform(begin(orig_tuples), end(orig_tuples), std::back_inserter(tuples),
                       [](tuple<IT, IT, T> tup) -> tuple<IT, IT, T> {
                           return tuple<IT, IT, T>(2 * std::get<0>(tup), 2 * std::get<1>(tup), std::get<2>(tup));
                       });

        ret.emplace_back(canned_matrix<T, IT>{
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
multiply_problem<T, IT> expand_multiply_problem(const multiply_problem<T, IT>& orig, int factor) {
    return multiply_problem<T, IT>{
            .description = orig.description + " expanded by " + std::to_string(factor) + "x",
            .a = expand_matrix(orig.a, factor),
            .b = expand_matrix(orig.b, factor),
            .result = expand_matrix(orig.result, factor),
    };
}

template <typename T, typename IT>
vector<multiply_problem<T, IT>> get_canned_multiply_problems() {
    vector<multiply_problem<T, IT>> ret;

    {
        ret.emplace_back(multiply_problem<T, IT>{
                .description = "empty square matrix squared",
                .a = {
                        .shape = {10, 10},
                        .sorted_tuples = simple_tuples_generator<T, IT>::EmptyMatrix()
                },
                .b = {
                        .shape = {10, 10},
                        .sorted_tuples = simple_tuples_generator<T, IT>::EmptyMatrix()
                },
                .result = {
                        .shape = {10, 10},
                        .sorted_tuples = simple_tuples_generator<T, IT>::EmptyMatrix()
                },
        });
    }

    {
        identity_tuples_generator<T, IT> gen(10);
        ret.emplace_back(multiply_problem<T, IT>{
                .description = "10x10 identity matrix squared",
                .a = {
                        .shape = {10, 10},
                        .sorted_tuples = vector<std::tuple<IT, IT, T>>(gen.begin(), gen.end()),
                },
                .b = {
                        .shape = {10, 10},
                        .sorted_tuples = vector<std::tuple<IT, IT, T>>(gen.begin(), gen.end()),
                },
                .result = {
                        .shape = {10, 10},
                        .sorted_tuples = vector<std::tuple<IT, IT, T>>(gen.begin(), gen.end()),
                },
        });
    }

    {
        identity_tuples_generator<T, IT> identity(4);
        vector<tuple<IT, IT, T>> quadrant_tuples{
                {1, 0, 1.0},
                {3, 0, 1.0},
                {0, 3, 1.0},
                {2, 3, 1.0}
        };

        ret.emplace_back(multiply_problem<T, IT>{
                .description = "4x4 top with empty columns * identity",
                .a = {
                        .shape = {4, 4},
                        .sorted_tuples = quadrant_tuples,
                },
                .b = {
                        .shape = {4, 4},
                        .sorted_tuples = vector<std::tuple<IT, IT, T>>(identity.begin(), identity.end()),
                },
                .result = {
                        .shape = {4, 4},
                        .sorted_tuples = quadrant_tuples,
                },
        });
    }

    {
        full_tuples_generator<T, IT> gen_factor({4, 4}, 1);
        full_tuples_generator<T, IT> gen_result({4, 4}, 4);

        ret.emplace_back(multiply_problem<T, IT>{
                .description = "4x4 full matrix squared",
                .a = {
                        .shape = {4, 4},
                        .sorted_tuples = vector<std::tuple<IT, IT, T>>(gen_factor.begin(), gen_factor.end()),
                },
                .b = {
                        .shape = {4, 4},
                        .sorted_tuples = vector<std::tuple<IT, IT, T>>(gen_factor.begin(), gen_factor.end()),
                },
                .result = {
                        .shape = {4, 4},
                        .sorted_tuples = vector<std::tuple<IT, IT, T>>(gen_result.begin(), gen_result.end()),
                },
        });
    }

    {
        identity_tuples_generator<T, IT> identity(7);

        ret.emplace_back(multiply_problem<T, IT>{
                .description = "identity * Kepner-Gilbert graph",
                .a = {
                        .shape = {7, 7},
                        .sorted_tuples = vector<std::tuple<IT, IT, T>>(identity.begin(), identity.end()),
                },
                .b = {
                        .shape = simple_tuples_generator<T, IT>::KepnerGilbertGraph_shape(),
                        .sorted_tuples = simple_tuples_generator<T, IT>::KepnerGilbertGraph(),
                },
                .result = {
                        .shape = simple_tuples_generator<T, IT>::KepnerGilbertGraph_shape(),
                        .sorted_tuples = simple_tuples_generator<T, IT>::KepnerGilbertGraph(),
                },
        });
    }

    {
        identity_tuples_generator<T, IT> identity(7);

        ret.emplace_back(multiply_problem<T, IT>{
                .description = "Kepner-Gilbert graph * identity",
                .a = {
                        .shape = simple_tuples_generator<T, IT>::KepnerGilbertGraph_shape(),
                        .sorted_tuples = simple_tuples_generator<T, IT>::KepnerGilbertGraph(),
                },
                .b = {
                        .shape = {7, 7},
                        .sorted_tuples = vector<std::tuple<IT, IT, T>>(identity.begin(), identity.end()),
                },
                .result = {
                        .shape = simple_tuples_generator<T, IT>::KepnerGilbertGraph_shape(),
                        .sorted_tuples = simple_tuples_generator<T, IT>::KepnerGilbertGraph(),
                },
        });
    }

    {
        identity_tuples_generator<T, IT> identity(10);

        ret.emplace_back(multiply_problem<T, IT>{
                .description = "10x10 Kepner-Gilbert graph * identity",
                .a = {
                        .shape = {10, 10},
                        .sorted_tuples = simple_tuples_generator<T, IT>::KepnerGilbertGraph(),
                },
                .b = {
                        .shape = {10, 10},
                        .sorted_tuples = vector<std::tuple<IT, IT, T>>(identity.begin(), identity.end()),
                },
                .result = {
                        .shape = {10, 10},
                        .sorted_tuples = simple_tuples_generator<T, IT>::KepnerGilbertGraph(),
                },
        });
    }

    {
        vector<std::tuple<IT, IT, T>> row_vector;
        vector<std::tuple<IT, IT, T>> col_vector;
        const size_t length = 16;

        for (IT i = 0; i < length; i++) {
            row_vector.emplace_back(0, i, 1);
            col_vector.emplace_back(i, 0, 1);
        }

        vector<std::tuple<IT, IT, T>> dot_product{
                {0, 0, length}
        };

        full_tuples_generator<T, IT> cross_product({length, length}, 1);

        // dot product: row vector * col vector
        ret.emplace_back(multiply_problem<T, IT>{
                .description = join::to_string("vector dot product length ", length),
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
        ret.emplace_back(multiply_problem<T, IT>{
                .description = join::to_string("vector cross product length ", length),
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
                        .sorted_tuples = vector<std::tuple<IT, IT, T>>(cross_product.begin(), cross_product.end()),
                },
        });
    }

    return ret;
}

#endif //QUADMAT_PROBLEM_GENERATOR_H
