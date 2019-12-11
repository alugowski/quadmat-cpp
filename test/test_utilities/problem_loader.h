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
 * Describe a multiply problem specified as matrix files on the filesystem.
 */
struct fs_multiply_problem {
    std::string description;
    std::string a_path;
    std::string b_path;
    std::string product_ab_path;
};

/**
 * Find the paths of multiply problem matrices.
 *
 * @param which one of "unit" or "medium"
 * @return
 */
inline vector<fs_multiply_problem> get_fs_multiply_problems(std::string which = "unit") {
    const fs::path multiply_dir{test_matrix_dir + which + "/multiply/" };

    vector<fs_multiply_problem> ret;

    for (const auto& problem_entry : fs::directory_iterator(multiply_dir)) {
        if (!problem_entry.is_directory()) {
            continue;
        }

        fs_multiply_problem problem;

        problem.description = problem_entry.path().filename().string();

        // read the matrices inside.
        fs::path a_path = problem_entry.path() / "a.mtx";
        fs::path b_path = problem_entry.path() / "b.mtx";
        fs::path product_ab = problem_entry.path() / "product_ab.mtx";

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

        if (fs::exists(product_ab) && fs::is_regular_file(product_ab)) {
            problem.product_ab_path = product_ab;
        } else {
            std::cerr << "Can't find expected matrix file " << product_ab << std::endl;
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
void load_canned_matrix(fs::path path, canned_matrix<T, IT>& cm) {

    // use a triples block to sort tuples
    triples_block<T, IT> tb;

    {
        std::ifstream infile{path};
        auto mat = matrix_market::load<T>(infile);

        cm.shape = mat.get_shape();

        tb.add(dump_tuples(mat));
    }

    auto sorted_tuple_range = tb.sorted_tuples();

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
vector<multiply_problem<T, IT>> load_fs_multiply_problems(std::string which = "unit") {
    vector<multiply_problem<T, IT>> ret;

    for (fs_multiply_problem fs_problem : get_fs_multiply_problems(which)) {
        multiply_problem<T, IT> problem;

        problem.description = fs_problem.description;
        load_canned_matrix(fs_problem.a_path, problem.a);
        load_canned_matrix(fs_problem.b_path, problem.b);
        load_canned_matrix(fs_problem.product_ab_path, problem.result);

        ret.push_back(problem);
    }

    return ret;
}

#endif //QUADMAT_PROBLEM_LOADER_H
