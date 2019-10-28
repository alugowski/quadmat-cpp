// Copyright (C) 2019 Adam Lugowski
// All Rights Reserved.

#include "catch.hpp"

#include "quadmat.h"

#include "problem_generator.h"

using Catch::Matchers::UnorderedEquals;

/**
 * Canned matrices
 */
static const auto canned_matrices = get_canned_matrices<double, int>(true); // NOLINT(cert-err58-cpp)
static const int num_canned_matrices = canned_matrices.size();

TEST_CASE("Matrix Market Loader") {
    SECTION("simple loader") {
        // get the problem
        int problem_num = GENERATE(range(0, num_canned_matrices));
        const canned_matrix<double, int>& problem = canned_matrices[problem_num];

        SECTION(problem.description) {

            quadmat::simple_matrix_market_loader loader(test_cwd + "matrices/" + problem.filename);

            REQUIRE(loader.is_load_successful());
            REQUIRE(loader.get_shape() == problem.shape);

            auto loaded_tuples = loader.tuples();
            vector<std::tuple<int, int, double>> v(begin(loaded_tuples), end(loaded_tuples));
            REQUIRE_THAT(v, UnorderedEquals(problem.sorted_tuples));
        }
    }
}
