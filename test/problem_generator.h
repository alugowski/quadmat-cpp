// Copyright (C) 2019 Adam Lugowski
// All Rights Reserved.

#ifndef QUADMAT_PROBLEM_GENERATOR_H
#define QUADMAT_PROBLEM_GENERATOR_H

#include <vector>

#include "util.h"

using quadmat::index_t;

template <typename T, typename IT>
struct canned_matrix {
    std::string description;
    quadmat::shape_t shape;
    vector<std::tuple<IT, IT, T>> sorted_tuples;
};

template <typename T, typename IT>
vector<canned_matrix<T, IT>> get_canned_matrices() {
    vector<canned_matrix<T, IT>> ret;

    {
        ret.emplace_back(canned_matrix<T, IT>{
                .description = "empty matrix",
                .shape = {10, 10},
                .sorted_tuples = quadmat::simple_tuples_generator<T, IT>::EmptyMatrix()
        });
    }

    {
        quadmat::identity_tuples_generator<T, IT> gen(10);
        ret.emplace_back(canned_matrix<T, IT>{
                .description = "n=10 identity matrix",
                .shape = {10, 10},
                .sorted_tuples = vector<std::tuple<IT, IT, T>>(gen.begin(), gen.end())
        });
    }

    {
        ret.emplace_back(canned_matrix<T, IT>{
                .description = "Kepner-Gilbert graph",
                .shape = quadmat::simple_tuples_generator<T, IT>::KepnerGilbertGraph_shape(),
                .sorted_tuples = quadmat::simple_tuples_generator<T, IT>::KepnerGilbertGraph()
        });
    }

    {
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

#endif //QUADMAT_PROBLEM_GENERATOR_H
