// Copyright (C) 2019-2020 Adam Lugowski
// All Rights Reserved.

#define CATCH_CONFIG_ENABLE_ALL_STRINGMAKERS

#include "../../../test_dependencies/catch.hpp"

#include "quadmat/quadmat.h"

#include "../../../test_utilities/testing_utilities.h"

using Catch::Matchers::Equals;
using Catch::Matchers::UnorderedEquals;

/**
 * Canned matrices
 */
static const auto kMultiplyProblems = GetMultiplyProblems<double, Index>(); // NOLINT(cert-err58-cpp)
static const int kNumMultiplyProblems = kMultiplyProblems.size();

struct Subdivision {
    bool subdivide_left;
    bool subdivide_right;
    std::string description;
};

TEST_CASE("Multiply") {
    SECTION("DCSC Block Pair") {
        // get the problem
        int problem_num = GENERATE(range(0, kNumMultiplyProblems));
        const MultiplyProblem<double, Index>& problem = kMultiplyProblems[problem_num];

        SECTION(problem.description) {
            auto a = DcscBlockFactory<double, Index>(problem.a.sorted_tuples.size(),
                                                     problem.a.sorted_tuples).Finish();
            auto b = DcscBlockFactory<double, Index>(problem.b.sorted_tuples.size(),
                                                     problem.b.sorted_tuples).Finish();

            Shape result_shape = {
                    .nrows = problem.a.shape.nrows,
                    .ncols = problem.b.shape.ncols
            };

            auto result = MultiplyPair<
                    DcscBlock<double, Index>,
                    DcscBlock<double, Index>,
                    Index,
                    PlusTimesSemiring<double>,
                    SparseSpa<Index, PlusTimesSemiring<double>, DefaultConfig>,
                    DefaultConfig>(a.get(), b.get(), result_shape);

            // test the result tuples
            {
                auto sorted_range = result->Tuples();
                std::vector <std::tuple<Index, Index, double>> v(sorted_range.begin(), sorted_range.end());
                REQUIRE_THAT(Matrix(problem.result.shape, TreeNode<double>(result)), MatrixEquals(problem.result));
            }
        }
    }

    SECTION("Simple Trees") {
        auto sub_type = GENERATE(
            Subdivision{false, false, "leaf * leaf"},
            Subdivision{true, false, "single inner * leaf"},
            Subdivision{false, true, "leaf * single inner"},
            Subdivision{true, true, "single inner * single inner"}
        );

        SECTION(sub_type.description) {
            // get the problem
            int problem_num = GENERATE(range(0, kNumMultiplyProblems));
            const MultiplyProblem<double, Index>& problem = kMultiplyProblems[problem_num];

            SECTION(problem.description) {
                auto a = SingleLeafMatrixFromTuples<double>(problem.a.shape,
                                                            problem.a.sorted_tuples.size(),
                                                            problem.a.sorted_tuples);
                auto b = SingleLeafMatrixFromTuples<double>(problem.b.shape,
                                                            problem.b.sorted_tuples.size(),
                                                            problem.b.sorted_tuples);

                // make sure the matrices look how this test assumes they do
                REQUIRE(IsLeaf(a.GetRootBC()->GetChild(0)));
                REQUIRE(IsLeaf(b.GetRootBC()->GetChild(0)));

                // subdivide
                if (sub_type.subdivide_left) {
                    SubdivideLeaf(a.GetRootBC(), 0, a.GetShape());
                }
                if (sub_type.subdivide_right) {
                    SubdivideLeaf(b.GetRootBC(), 0, b.GetShape());
                }

                // multiply
                auto result = Multiply<PlusTimesSemiring<double>>(a, b);

                // test the result
                REQUIRE_THAT(result, MatrixEquals(problem.result));

                // the entire result tree can be safely destroyed
                result.ParallelDestroy(1);
            }
        }
    }

    SECTION("LeafSplitThreshold=4") {
        auto sub_type = GENERATE(
                // leaf * leaf already handled in another test case
                Subdivision{true, false, "tree * leaf"},
                Subdivision{false, true, "leaf * tree"},
                Subdivision{true, true, "tree * tree"}
        );

        SECTION(sub_type.description) {
            // get the problem
            int problem_num = GENERATE(range(0, kNumMultiplyProblems));
            const MultiplyProblem<double, Index> &problem = kMultiplyProblems[problem_num];

            SECTION(problem.description) {
                Matrix<double, ConfigSplit4> a{problem.a.shape}, b{problem.b.shape};

                // construct matrix a as either a tree or a single leaf
                if (sub_type.subdivide_left) {
                    a = MatrixFromTuples<double, ConfigSplit4>(problem.a.shape,
                                                               problem.a.sorted_tuples.size(),
                                                               problem.a.sorted_tuples);
                } else {
                    a = SingleLeafMatrixFromTuples<double, ConfigSplit4>(problem.a.shape,
                                                                         problem.a.sorted_tuples.size(),
                                                                         problem.a.sorted_tuples);
                }

                // construct matrix b as either a tree or a single leaf
                if (sub_type.subdivide_right) {
                    b = MatrixFromTuples<double, ConfigSplit4>(problem.b.shape,
                                                               problem.b.sorted_tuples.size(),
                                                               problem.b.sorted_tuples);
                } else {
                    b = SingleLeafMatrixFromTuples<double, ConfigSplit4>(problem.b.shape,
                                                                         problem.b.sorted_tuples.size(),
                                                                         problem.b.sorted_tuples);
                }
                REQUIRE("" == SanityCheck(a)); // NOLINT(readability-container-size-empty)
                REQUIRE("" == SanityCheck(b)); // NOLINT(readability-container-size-empty)

                // multiply
                auto result = Multiply<PlusTimesSemiring<double>>(a, b);

                REQUIRE("" == SanityCheck(result)); // NOLINT(readability-container-size-empty)

                // test the result
                REQUIRE_THAT(result, MatrixEquals(problem.result, ConfigSplit4()));

                // the entire result tree can be safely destroyed
                result.ParallelDestroy(1);
            }
        }
    }

    SECTION("Forced errors") {
        // hit some edge cases and use artificial constructions to force errors to get 100% line coverage

        auto future_node = TreeNode<double>(std::make_shared<FutureBlock<double>>());
        Matrix<double> future_matrix_10x10{{10, 10}, future_node};
        auto problem_10x10 = kMultiplyProblems[1];
        auto problem_4x4 = kMultiplyProblems[3];

        auto matrix_10x10 = SingleLeafMatrixFromTuples<double>(problem_10x10.a.shape,
                                                               problem_10x10.a.sorted_tuples.size(),
                                                               problem_10x10.a.sorted_tuples);
        auto matrix_4x4 = SingleLeafMatrixFromTuples<double>(problem_4x4.a.shape,
                                                             problem_4x4.a.sorted_tuples.size(),
                                                             problem_4x4.a.sorted_tuples);

        // future blocks are not implemented
        SECTION("future blocks"){
            REQUIRE_THROWS_AS(Multiply<PlusTimesSemiring<double>>(future_matrix_10x10, matrix_10x10),
                              NotImplemented);
        }

        // dimension mismatch
        SECTION("dimension mismatches"){
            REQUIRE_THROWS_AS(Multiply<PlusTimesSemiring<double>>(matrix_4x4, matrix_10x10), NodeTypeMismatch);
        }

        SECTION("recurse corruption") {
            Matrix<double> empty_matrix_10x10{{10, 10}};
            auto matrix_inner_10x10 = SingleLeafMatrixFromTuples<double>(problem_10x10.a.shape,
                                                                         problem_10x10.a.sorted_tuples.size(),
                                                                         problem_10x10.a.sorted_tuples);
            SubdivideLeaf(matrix_inner_10x10.GetRootBC(), 0, matrix_inner_10x10.GetShape());

            Matrix<double> ret{{10, 10}};

            {
                DirectTaskQueue<DefaultConfig> queue;

                // setup multiply job
                MultiplyTask<PlusTimesSemiring<double>, DefaultConfig> job(
                    queue,
                    PairSet<double, double, DefaultConfig>{
                        matrix_inner_10x10.GetRootBC()->GetChild(0),
                        empty_matrix_10x10.GetRootBC()->GetChild(0),
                        matrix_inner_10x10.GetShape(),
                        empty_matrix_10x10.GetShape(),
                        matrix_inner_10x10.GetRootBC()->GetDiscriminatingBit(),
                        empty_matrix_10x10.GetRootBC()->GetDiscriminatingBit()
                    },
                    ret.GetRootBC(), 0, {0, 0}, ret.GetShape());

                REQUIRE_THROWS_AS(job.Run(false), NodeTypeMismatch);
            }

            {
                DirectTaskQueue<DefaultConfig> queue;

                // zero dimension test
                MultiplyTask<PlusTimesSemiring<double>, DefaultConfig> job_zero_row(
                    queue,
                    PairSet<double, double, DefaultConfig>{
                        matrix_inner_10x10.GetRootBC()->GetChild(0),
                        empty_matrix_10x10.GetRootBC()->GetChild(0),
                        matrix_inner_10x10.GetShape(),
                        empty_matrix_10x10.GetShape(),
                        matrix_inner_10x10.GetRootBC()->GetDiscriminatingBit(),
                        empty_matrix_10x10.GetRootBC()->GetDiscriminatingBit()
                    },
                    ret.GetRootBC(), 0, {0, 0}, Shape{0, 10});

                REQUIRE_THROWS_AS(job_zero_row.Run(false), NodeTypeMismatch);
            }

            {
                DirectTaskQueue<DefaultConfig> queue;

                // zero dimension test
                MultiplyTask<PlusTimesSemiring<double>, DefaultConfig> job_zero_col(
                    queue,
                    PairSet<double, double, DefaultConfig>{
                        matrix_inner_10x10.GetRootBC()->GetChild(0),
                        empty_matrix_10x10.GetRootBC()->GetChild(0),
                        matrix_inner_10x10.GetShape(),
                        empty_matrix_10x10.GetShape(),
                        matrix_inner_10x10.GetRootBC()->GetDiscriminatingBit(),
                        empty_matrix_10x10.GetRootBC()->GetDiscriminatingBit()
                    },
                    ret.GetRootBC(), 0, {0, 0}, Shape{10, 0});

                REQUIRE_THROWS_AS(job_zero_col.Run(false), NodeTypeMismatch);
            }
        }
    }
}
