// Copyright (C) 2019 Adam Lugowski
// All Rights Reserved.

#ifndef QUADMAT_PROBLEM_GENERATOR_H
#define QUADMAT_PROBLEM_GENERATOR_H

#include <vector>

#include "quadmat/util/util.h"

using namespace quadmat;

// TODO: figure out how to set the working directory through CMake
static const std::string test_cwd = "/Users/enos/projects/quadmat/test/";

/**
 * Describe a single matrix
 */
template <typename T, typename IT>
struct canned_matrix {
    std::string description;
    shape_t shape;
    vector<std::tuple<IT, IT, T>> sorted_tuples;
    std::string filename; // only set if available

    /**
     * @return a version of `sorted_tuples` where duplicates are summed
     */
    vector<std::tuple<IT, IT, T>> accumulated_tuples() const {
        vector<std::tuple<IT, IT, T>> ret;

        bool first = true;
        IT last_row, last_col;
        for (auto tup : sorted_tuples) {
            auto [row, col, value] = tup;

            if (!first && last_row == row && last_col == col) {
                // same position as last entry, sum values
                ret.back() = std::tuple<IT, IT, T>(row, col, std::get<2>(ret.back()) + value);
            } else {
                ret.push_back(tup);
            }

            first = false;
            last_row = row;
            last_col = col;
        }

        return ret;
    }
};

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
        // expand into 32-bit indices
        ret.push_back(expand_matrix(ret.back(), 10000));
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
 * Describe a result = a * b problem
 */
template <typename T, typename IT>
struct multiply_problem {
    std::string description;
    canned_matrix<T, IT> a;
    canned_matrix<T, IT> b;
    canned_matrix<T, IT> result;
};


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
vector<multiply_problem<T, IT>> get_multiply_problems() {
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
        simple_matrix_market_loader loader;
        auto result_mat = loader.load(test_cwd + "matrices/kepner_gilbert_graph_squared.mtx");

        // use triple_block to sort
        triples_block<double, index_t> result_block;
        result_block.add(loader.tuples());
        auto sorted_result_range = result_block.sorted_tuples();

        ret.emplace_back(multiply_problem<T, IT>{
                .description = "Kepner-Gilbert graph squared",
                .a = {
                        .shape = simple_tuples_generator<T, IT>::KepnerGilbertGraph_shape(),
                        .sorted_tuples = simple_tuples_generator<T, IT>::KepnerGilbertGraph(),
                },
                .b = {
                        .shape = simple_tuples_generator<T, IT>::KepnerGilbertGraph_shape(),
                        .sorted_tuples = simple_tuples_generator<T, IT>::KepnerGilbertGraph(),
                },
                .result = {
                        .shape = result_mat.get_shape(),
                        .sorted_tuples = vector<std::tuple<IT, IT, T>>(sorted_result_range.begin(), sorted_result_range.end()),
                }
        });
    }

    {
        // expand into 32-bits
//        ret.push_back(expand_multiply_problem(ret.back(), 10000));
    }

    return ret;
}

#endif //QUADMAT_PROBLEM_GENERATOR_H
