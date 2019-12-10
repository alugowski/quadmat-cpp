// Copyright (C) 2019 Adam Lugowski
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
static const std::string test_cwd = "/Users/enos/projects/quadmat/test/";
static const std::string test_matrix_dir = test_cwd + "matrices/";
static const std::string unittest_matrix_dir = test_matrix_dir + "unit/";

/**
 * Load a matrix market file into a canned matrix.
 *
 * @tparam T
 * @tparam IT
 * @param cm matrix to load into.
 */
template <typename T, typename IT>
void load_canned_matrix(fs::path path, canned_matrix<T, IT>& cm) {
    std::ifstream infile{path};

    // use a triples block to sort tuples
    triples_block<T, IT> tb;

    {
        auto mat = matrix_market::load(infile);

        cm.shape = mat.get_shape();

        tb.add(dump_tuples(mat));
    }

    auto sorted_tuple_range = tb.sorted_tuples();

    std::copy(std::begin(sorted_tuple_range), std::end(sorted_tuple_range), std::back_inserter(cm.sorted_tuples));
}

/**
 * Read problems from filesystem.
 *
 * This method depends on the QuadMat matrix market routines.
 *
 * @tparam T
 * @tparam IT
 * @param which Which tests to read from. I.e. one of "unit" or "medium".
 */
template <typename T, typename IT>
vector<multiply_problem<T, IT>> get_fs_multiply_problems(std::string which = "unit") {
    const fs::path multiply_dir{test_matrix_dir + which + "/multiply/" };

    vector<multiply_problem<T, IT>> ret;

    for (const auto& problem_entry : fs::directory_iterator(multiply_dir)) {
        if (!problem_entry.is_directory()) {
            continue;
        }

        multiply_problem<T, IT> problem;

        problem.description = problem_entry.path().filename().string();

        // read the matrices inside.
        fs::path a_path = problem_entry.path() / "a.mtx";
        fs::path b_path = problem_entry.path() / "b.mtx";
        fs::path result_path = problem_entry.path() / "product_ab.mtx";

        if (fs::exists(a_path) && fs::is_regular_file(a_path)) {
            load_canned_matrix(a_path, problem.a);
        } else {
            std::cerr << "Can't find expected matrix file " << a_path << std::endl;
            break;
        }

        if (fs::exists(b_path) && fs::is_regular_file(b_path)) {
            load_canned_matrix(b_path, problem.b);
        } else {
            std::cerr << "Can't find expected matrix file " << b_path << std::endl;
            break;
        }

        if (fs::exists(result_path) && fs::is_regular_file(result_path)) {
            load_canned_matrix(result_path, problem.result);
        } else {
            std::cerr << "Can't find expected matrix file " << result_path << std::endl;
            break;
        }

        ret.push_back(problem);
    }

    return ret;
}
#endif //QUADMAT_PROBLEM_LOADER_H
