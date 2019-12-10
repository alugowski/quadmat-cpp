// Copyright (C) 2019 Adam Lugowski
// All Rights Reserved.

#ifndef QUADMAT_PROBLEMS_H
#define QUADMAT_PROBLEMS_H

#include "problem_structs.h"
#include "problem_generator.h"
#include "problem_loader.h"

template <typename T, typename IT>
vector<multiply_problem<T, IT>> get_multiply_problems() {
    auto ret = get_canned_multiply_problems<T, IT>();

    // add fs problems
    auto fs_problems = get_fs_multiply_problems<T, IT>("unit");
    for (auto problem : fs_problems) {
        // add the problem
        ret.push_back(std::move(problem));

        // add a version blown-up to 32-bits
        ret.push_back(expand_multiply_problem(ret.back(), 10000));
    }

    return ret;
}

#endif //QUADMAT_PROBLEMS_H
