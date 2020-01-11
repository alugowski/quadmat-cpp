// Copyright (C) 2019-2020 Adam Lugowski
// All Rights Reserved.

#ifndef QUADMAT_PROBLEM_STRUCTS_H
#define QUADMAT_PROBLEM_STRUCTS_H

#include <tuple>
#include <vector>

/**
 * Describe a single matrix
 */
template <typename T, typename IT>
struct CannedMatrix {
    std::string description;
    Shape shape;
    std::vector<std::tuple<IT, IT, T>> sorted_tuples;
    std::string filename; // only set if available

    /**
     * @return a version of `sorted_tuples` where duplicates are summed
     */
    std::vector<std::tuple<IT, IT, T>> GetAccumulatedTuples() const {
        std::vector<std::tuple<IT, IT, T>> ret;

        bool first = true;
        IT last_row, last_col;
        for (auto tup : sorted_tuples) {
            auto [row, col, value] = tup;

            if (!first && last_row == row && last_col == col) {
                // same position as last entry, sum values
                ret.back() = std::tuple<IT, IT, T>(row, col, std::get<2>(ret.back()) + value);
            } else {
                ret.push_back(tup);
            }

            first = false;
            last_row = row;
            last_col = col;
        }

        return ret;
    }
};


/**
 * Describe a result = a * b problem
 */
template <typename T, typename IT>
struct MultiplyProblem {
    std::string description;
    CannedMatrix<T, IT> a;
    CannedMatrix<T, IT> b;
    CannedMatrix<T, IT> result;
};

#endif //QUADMAT_PROBLEM_STRUCTS_H
