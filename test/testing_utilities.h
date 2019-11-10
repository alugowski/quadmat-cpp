// Copyright (C) 2019 Adam Lugowski
// All Rights Reserved.

#ifndef QUADMAT_TESTING_UTILITIES_H
#define QUADMAT_TESTING_UTILITIES_H

#include "quadmat.h"

using quadmat::index_t;


/**
 * A node visitor that dumps tuples from nodes
 *
 * @tparam T
 */
template <typename T>
class tuple_dumper {
public:
    explicit tuple_dumper(vector<std::tuple<index_t, index_t, T>> &tuples) : tuples(tuples) {}

    template <typename LEAF>
    void operator()(quadmat::offset_t offsets, const std::shared_ptr<LEAF>& leaf) const {
        for (auto tup : leaf->tuples()) {
            tuples.emplace_back(
                    std::get<0>(tup) + offsets.row_offset,
                    std::get<1>(tup) + offsets.col_offset,
                    std::get<2>(tup)
            );
        }
    }
protected:
    vector<std::tuple<index_t, index_t, T>>& tuples;
};

#endif //QUADMAT_TESTING_UTILITIES_H
