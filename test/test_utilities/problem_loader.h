// Copyright (C) 2019-2020 Adam Lugowski
// All Rights Reserved.

#ifndef QUADMAT_PROBLEM_LOADER_H
#define QUADMAT_PROBLEM_LOADER_H

#include "problem_structs.h"
#include "tuple_dumper.h"

#if !QUADMAT_USE_GHC_FILESYSTEM && (defined(__cplusplus) && __cplusplus >= 201703L && defined(__has_include))
    #if __has_include(<filesystem>)
        #define GHC_USE_STD_FS
        #include <filesystem>
        namespace fs = std::filesystem;
    #endif
#endif
#ifndef GHC_USE_STD_FS
    #include "../test_dependencies/ghc/filesystem.hpp"
    namespace fs = ghc::filesystem;
#endif

// TODO: figure out how to set the working directory through CMake
static const std::string kTestCwd = "/Users/enos/projects/quadmat/test/";
static const std::string kTestMatrixDir = kTestCwd + "matrices/";
static const std::string kUnitTestMatrixDir = kTestMatrixDir + "unit/";

/**
 * Describe a multiply problem specified as matrix files on the filesystem.
 */
struct FsMultiplyProblem {
    std::string description;
    std::string a_path;
    std::string b_path;
    std::string c_path;
    std::string product_ab_path;
    std::string product_abc_path;
};

/**
 * Find the paths of multiply problem matrices.
 *
 * @param which one of "unit" or "medium"
 * @return
 */
inline std::vector<FsMultiplyProblem> GetFsMultiplyProblems(std::string which = "unit") {
    const fs::path multiply_dir{kTestMatrixDir + which + "/multiply/" };

    std::vector<FsMultiplyProblem> ret;

    for (const auto& problem_entry : fs::directory_iterator(multiply_dir)) {
        if (!problem_entry.is_directory()) {
            continue;
        }

        FsMultiplyProblem problem;

        problem.description = problem_entry.path().filename().string();

        // read the matrices inside.
        fs::path a_path = problem_entry.path() / "a.mtx";
        fs::path b_path = problem_entry.path() / "b.mtx";
        fs::path c_path = problem_entry.path() / "c.mtx";
        fs::path product_ab = problem_entry.path() / "product_ab.mtx";
        fs::path product_abc = problem_entry.path() / "product_abc.mtx";

        if (fs::exists(a_path) && fs::is_regular_file(a_path)) {
            problem.a_path = a_path;
        } else {
            std::cerr << "Can't find expected matrix file " << a_path << std::endl;
            break;
        }

        if (fs::exists(b_path) && fs::is_regular_file(b_path)) {
            problem.b_path = b_path;
        } else {
            std::cerr << "Can't find expected matrix file " << b_path << std::endl;
            break;
        }

        if (fs::exists(c_path) && fs::is_regular_file(c_path)) {
            problem.c_path = c_path;
        }

        if (fs::exists(product_ab) && fs::is_regular_file(product_ab)) {
            problem.product_ab_path = product_ab;
        }
        if (fs::exists(product_abc) && fs::is_regular_file(product_abc)) {
            problem.product_abc_path = product_abc;
        }

        if (problem.product_ab_path.empty() && problem.product_abc_path.empty()) {
            std::cerr << "Can't find expected product matrix files " << product_ab << " or " << product_abc << std::endl;
            break;
        }

        ret.push_back(problem);
    }

    return ret;
}

/**
 * Load a matrix market file into a canned matrix.
 *
 * @tparam T
 * @tparam IT
 * @param cm matrix to load into.
 */
template <typename T, typename IT>
void LoadCannedMatrix(fs::path path, CannedMatrix<T, IT>& cm) {

    // use a triples block to sort tuples
    TriplesBlock<T, IT> tb;

    {
        std::ifstream infile{path};
        auto mat = MatrixMarket::Load<T>(infile);

        cm.shape = mat.GetShape();

        tb.Add(DumpTuples(mat));
    }

    auto sorted_tuple_range = tb.SortedTuples();

    std::copy(std::begin(sorted_tuple_range), std::end(sorted_tuple_range), std::back_inserter(cm.sorted_tuples));
}

/**
 * Read problems from filesystem.
 *
 * This method depends on the QuadMat matrix market routines. If those don't work make those unit tests work first.
 *
 * @tparam T
 * @tparam IT
 * @param which Which tests to read from. I.e. one of "unit" or "medium".
 */
template <typename T, typename IT>
std::vector<MultiplyProblem<T, IT>> LoadFsMultiplyProblems(std::string which = "unit") {
    std::vector<MultiplyProblem<T, IT>> ret;

    for (FsMultiplyProblem fs_problem : GetFsMultiplyProblems(which)) {
        MultiplyProblem<T, IT> problem;

        if (fs_problem.product_ab_path.empty()) {
            // skip triple products
            continue;
        }

        problem.description = fs_problem.description;
        LoadCannedMatrix(fs_problem.a_path, problem.a);
        LoadCannedMatrix(fs_problem.b_path, problem.b);
        LoadCannedMatrix(fs_problem.product_ab_path, problem.result);

        ret.push_back(problem);
    }

    return ret;
}

#endif //QUADMAT_PROBLEM_LOADER_H
