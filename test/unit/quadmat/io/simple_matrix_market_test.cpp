// Copyright (C) 2019-2020 Adam Lugowski
// All Rights Reserved.

#include "../../../test_dependencies/catch.hpp"

#include "quadmat/quadmat.h"

#include "../../../test_utilities/testing_utilities.h"

using namespace quadmat;

using Catch::Matchers::UnorderedEquals;

/**
 * Canned matrices
 */
static const auto kCannedMatricesWithFiles = GetCannedMatrices<double, Index>(true); // NOLINT(cert-err58-cpp)
static const int kNumCannedMatricesWithFiles = kCannedMatricesWithFiles.size();

static const auto kCannedMatrices = GetCannedMatrices<double, Index>(); // NOLINT(cert-err58-cpp)
static const int kNumCannedMatrices = kCannedMatrices.size();

TEST_CASE("I/O - Matrix Market") {
    SECTION("Load") {
        // get the problem
        int problem_num = GENERATE(range(0, kNumCannedMatricesWithFiles));
        const CannedMatrix<double, Index>& problem = kCannedMatricesWithFiles[problem_num];

        SECTION(problem.description) {
            std::ifstream infile{kUnitTestMatrixDir + problem.filename};
            auto mat = MatrixMarket::Load(infile);

            REQUIRE(mat.GetShape() == problem.shape);

            REQUIRE_THAT(mat, MatrixEquals(problem));
        }
    }
    SECTION("Load - invalid inputs") {
        {
            SimpleMatrixMarketLoader loader{IgnoringErrorConsumer()};
            loader.Load(kUnitTestMatrixDir + "invalid_bad_banner.mtx");
            REQUIRE(!loader.IsLoadSuccessful());
        }
        {
            SimpleMatrixMarketLoader loader{IgnoringErrorConsumer()};
            loader.Load(kUnitTestMatrixDir + "invalid_indices_out_of_range_1.mtx");
            REQUIRE(!loader.IsLoadSuccessful());
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
            REQUIRE_THROWS(SimpleMatrixMarketLoader().Load(kUnitTestMatrixDir + filename));
        }
    }

    SECTION("Save") {
        // get the problem
        int problem_num = GENERATE(range(0, kNumCannedMatrices));
        const CannedMatrix<double, Index>& problem = kCannedMatrices[problem_num];

        SECTION(problem.description) {
            // construct a tight matrix
            auto mat = MatrixFromTuples<double, ConfigSplit4>(problem.shape,
                                                              problem.sorted_tuples.size(),
                                                              problem.sorted_tuples);

            // save
            std::ostringstream oss;
            REQUIRE(MatrixMarket::Save(mat, oss));

            // load
            std::istringstream iss{oss.str()};
            auto loaded_mat = MatrixMarket::Load<double, ConfigSplit4>(iss);

            REQUIRE_THAT(mat, MatrixEquals(loaded_mat));
        }
    }
}
