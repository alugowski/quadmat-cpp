// Copyright (C) 2019 Adam Lugowski
// All Rights Reserved.

#ifndef QUADMAT_PROBLEM_GENERATOR_H
#define QUADMAT_PROBLEM_GENERATOR_H

#include <vector>

#include "quadmat/util/util.h"

using quadmat::index_t;

// TODO: figure out how to set the working directory through CMake
static const std::string test_cwd = "/Users/enos/projects/quadmat/test/";

/**
 * Describe a single matrix
 */
template <typename T, typename IT>
struct canned_matrix {
    std::string description;
    quadmat::shape_t shape;
    vector<std::tuple<IT, IT, T>> sorted_tuples;
    std::string filename; // only set if available
};

template <typename T, typename IT>
vector<canned_matrix<T, IT>> get_canned_matrices(bool only_with_files = false) {
    vector<canned_matrix<T, IT>> ret;

    if (!only_with_files) {
        ret.emplace_back(canned_matrix<T, IT>{
                .description = "empty matrix",
                .shape = {10, 10},
                .sorted_tuples = quadmat::simple_tuples_generator<T, IT>::EmptyMatrix()
        });
    }

    if (!only_with_files) {
        quadmat::identity_tuples_generator<T, IT> gen(10);
        ret.emplace_back(canned_matrix<T, IT>{
                .description = "10x10 identity matrix",
                .shape = {10, 10},
                .sorted_tuples = vector<std::tuple<IT, IT, T>>(gen.begin(), gen.end())
        });
    }

    {
        quadmat::full_tuples_generator<T, IT> gen({4, 4}, 1);
        ret.emplace_back(canned_matrix<T, IT>{
                .description = "4x4 full matrix",
                .shape = {4, 4},
                .sorted_tuples = vector<std::tuple<IT, IT, T>>(gen.begin(), gen.end()),
                .filename = "small_full_symmetric_pattern.mtx"
        });
    }

    {
        ret.emplace_back(canned_matrix<T, IT>{
                .description = "Kepner-Gilbert graph",
                .shape = quadmat::simple_tuples_generator<T, IT>::KepnerGilbertGraph_shape(),
                .sorted_tuples = quadmat::simple_tuples_generator<T, IT>::KepnerGilbertGraph(),
                .filename = "kepner_gilbert_graph.mtx"
        });
    }

    if (!only_with_files) {
        // same as above but with extra sparsity
        quadmat::shape_t orig_shape = quadmat::simple_tuples_generator<T, IT>::KepnerGilbertGraph_shape();

        auto orig_tuples = quadmat::simple_tuples_generator<T, IT>::KepnerGilbertGraph();

        vector<std::tuple<IT, IT, T>> tuples;
        std::transform(begin(orig_tuples), end(orig_tuples), std::back_inserter(tuples),
                       [](tuple<int, int, double> tup) -> tuple<int, int, double> {
                           return tuple<int, int, double>(2 * std::get<0>(tup), 2 * std::get<1>(tup), std::get<2>(tup));
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
    quadmat::shape_t a_shape;
    vector<std::tuple<IT, IT, T>> a_sorted_tuples;
    quadmat::shape_t b_shape;
    vector<std::tuple<IT, IT, T>> b_sorted_tuples;
    quadmat::shape_t result_shape;
    vector<std::tuple<IT, IT, T>> result_sorted_tuples;
};

template <typename T, typename IT>
vector<multiply_problem<T, IT>> get_multiply_problems() {
    vector<multiply_problem<T, IT>> ret;

    {
        ret.emplace_back(multiply_problem<T, IT>{
                .description = "empty square matrix squared",
                .a_shape = {10, 10},
                .a_sorted_tuples = quadmat::simple_tuples_generator<T, IT>::EmptyMatrix(),
                .b_shape = {10, 10},
                .b_sorted_tuples = quadmat::simple_tuples_generator<T, IT>::EmptyMatrix(),
                .result_shape = {10, 10},
                .result_sorted_tuples = quadmat::simple_tuples_generator<T, IT>::EmptyMatrix(),
        });
    }

    {
        quadmat::identity_tuples_generator<T, IT> gen(10);
        ret.emplace_back(multiply_problem<T, IT>{
                .description = "10x10 identity matrix squared",
                .a_shape = {10, 10},
                .a_sorted_tuples = vector<std::tuple<IT, IT, T>>(gen.begin(), gen.end()),
                .b_shape = {10, 10},
                .b_sorted_tuples = vector<std::tuple<IT, IT, T>>(gen.begin(), gen.end()),
                .result_shape = {10, 10},
                .result_sorted_tuples = vector<std::tuple<IT, IT, T>>(gen.begin(), gen.end()),
        });
    }

    {
        quadmat::full_tuples_generator<T, IT> gen_factor({4, 4}, 1);
        quadmat::full_tuples_generator<T, IT> gen_result({4, 4}, 4);

        ret.emplace_back(multiply_problem<T, IT>{
                .description = "4x4 full matrix squared",
                .a_shape = {4, 4},
                .a_sorted_tuples = vector<std::tuple<IT, IT, T>>(gen_factor.begin(), gen_factor.end()),
                .b_shape = {4, 4},
                .b_sorted_tuples = vector<std::tuple<IT, IT, T>>(gen_factor.begin(), gen_factor.end()),
                .result_shape = {4, 4},
                .result_sorted_tuples = vector<std::tuple<IT, IT, T>>(gen_result.begin(), gen_result.end()),
        });
    }

    {
        quadmat::identity_tuples_generator<T, IT> identity(7);

        ret.emplace_back(multiply_problem<T, IT>{
                .description = "identity * Kepner-Gilbert graph",
                .a_shape = {7, 7},
                .a_sorted_tuples = vector<std::tuple<IT, IT, T>>(identity.begin(), identity.end()),
                .b_shape = quadmat::simple_tuples_generator<T, IT>::KepnerGilbertGraph_shape(),
                .b_sorted_tuples = quadmat::simple_tuples_generator<T, IT>::KepnerGilbertGraph(),
                .result_shape = {7, 7},
                .result_sorted_tuples = quadmat::simple_tuples_generator<T, IT>::KepnerGilbertGraph(),
        });
    }

    {
        quadmat::simple_matrix_market_loader loader(test_cwd + "matrices/kepner_gilbert_graph_squared.mtx");

        // use triple_block to sort
        quadmat::triples_block<double, int> result_block(loader.get_shape());
        result_block.add(loader.tuples());
        auto sorted_result_range = result_block.sorted_tuples();

        ret.emplace_back(multiply_problem<T, IT>{
                .description = "Kepner-Gilbert graph squared",
                .a_shape = quadmat::simple_tuples_generator<T, IT>::KepnerGilbertGraph_shape(),
                .a_sorted_tuples = quadmat::simple_tuples_generator<T, IT>::KepnerGilbertGraph(),
                .b_shape = quadmat::simple_tuples_generator<T, IT>::KepnerGilbertGraph_shape(),
                .b_sorted_tuples = quadmat::simple_tuples_generator<T, IT>::KepnerGilbertGraph(),
                .result_shape = loader.get_shape(),
                .result_sorted_tuples = vector<std::tuple<IT, IT, T>>(sorted_result_range.begin(), sorted_result_range.end()),
        });
    }


    return ret;
}

#endif //QUADMAT_PROBLEM_GENERATOR_H
