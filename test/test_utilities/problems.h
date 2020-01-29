// Copyright (C) 2019-2020 Adam Lugowski
// All Rights Reserved.

#ifndef QUADMAT_PROBLEMS_H
#define QUADMAT_PROBLEMS_H

#include "problem_structs.h"
#include "problem_generator.h"
#include "problem_loader.h"

template <typename T, typename IT>
std::vector<MultiplyProblem<T, IT>> GetMultiplyProblems() {
    auto ret = GetCannedMultiplyProblems<T, IT>();

    // add fs problems
    auto fs_problems = LoadFsMultiplyProblems<T, IT>("unit");
    for (auto problem : fs_problems) {
        // add the problem
        ret.push_back(std::move(problem));

        const auto& orig = ret.back();

        if (orig.description.rfind("square ER", 0) == 0 || orig.description.find("graph") != std::string::npos) {
            // add a version blown-up to 32-bits
            ret.push_back(ExpandMultiplyProblem(orig, 10000));

            if (orig.description.find("graph") != std::string::npos) {
                // add a version blown-up to require sparse SpA
                ret.push_back(ExpandMultiplyProblem(orig, 500000));
            }
        }
    }

    return ret;
}

#endif //QUADMAT_PROBLEMS_H
