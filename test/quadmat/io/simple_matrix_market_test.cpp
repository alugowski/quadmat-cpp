// Copyright (C) 2019 Adam Lugowski
// All Rights Reserved.

#include "../../catch.hpp"

#include "quadmat/quadmat.h"

#include "../../testing_utilities.h"

using namespace quadmat;

using Catch::Matchers::UnorderedEquals;

/**
 * Canned matrices
 */
static const auto canned_matrices_with_files = get_canned_matrices<double, index_t>(true); // NOLINT(cert-err58-cpp)
static const int num_canned_matrices_with_files = canned_matrices_with_files.size();

static const auto canned_matrices = get_canned_matrices<double, index_t>(); // NOLINT(cert-err58-cpp)
static const int num_canned_matrices = canned_matrices.size();

TEST_CASE("I/O - Matrix Market") {
    SECTION("load") {
        // get the problem
        int problem_num = GENERATE(range(0, num_canned_matrices_with_files));
        const canned_matrix<double, index_t>& problem = canned_matrices_with_files[problem_num];

        SECTION(problem.description) {
            std::ifstream infile{test_cwd + "matrices/" + problem.filename};
            auto mat = matrix_market::load(infile);

            REQUIRE(mat.get_shape() == problem.shape);

            REQUIRE_THAT(mat, MatrixEquals(problem));
        }
    }
    SECTION("load - invalid inputs") {
        {
            simple_matrix_market_loader loader{ignoring_error_consumer()};
            loader.load(test_cwd + "matrices/" + "invalid_bad_banner.mtx");
            REQUIRE(!loader.is_load_successful());
        }
        {
            simple_matrix_market_loader loader{ignoring_error_consumer()};
            loader.load(test_cwd + "matrices/" + "invalid_indices_out_of_range_1.mtx");
            REQUIRE(!loader.is_load_successful());
        }

        std::string filename = GENERATE(as<std::string>{},
                                        "doesnt_exist.mtx",
                                        "invalid_array.mtx",
                                        "invalid_bad_banner.mtx",
                                        "invalid_bad_field.mtx",
                                        "invalid_bad_format.mtx",
                                        "invalid_bad_object.mtx",
                                        "invalid_bad_symmetry.mtx",
                                        "invalid_complex.mtx",
                                        "invalid_hermitian.mtx",
                                        "invalid_indices_out_of_range_1.mtx",
                                        "invalid_indices_out_of_range_2.mtx",
                                        "invalid_truncated_header.mtx",
                                        "invalid_truncated_line_1.mtx",
                                        "invalid_truncated_line_2.mtx",
                                        "invalid_truncated_lines.mtx",
                                        "invalid_vector.mtx"
        );

        SECTION(filename) {
            REQUIRE_THROWS(simple_matrix_market_loader().load(test_cwd + "matrices/" + filename));
        }
    }

    SECTION("save") {
        // get the problem
        int problem_num = GENERATE(range(0, num_canned_matrices));
        const canned_matrix<double, index_t>& problem = canned_matrices[problem_num];

        SECTION(problem.description) {
            // construct a tight matrix
            auto mat = matrix_from_tuples<double, config_split_4>(problem.shape,
                                                                  problem.sorted_tuples.size(),
                                                                  problem.sorted_tuples);

            // save
            std::ostringstream oss;
            REQUIRE(matrix_market::save(mat, oss));

            // load
            std::istringstream iss{oss.str()};
            auto loaded_mat = matrix_market::load<double, config_split_4>(iss);

            REQUIRE_THAT(mat, MatrixEquals(loaded_mat));
        }
    }
}
