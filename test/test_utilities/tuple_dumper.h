// Copyright (C) 2019-2020 Adam Lugowski
// All Rights Reserved.

#ifndef QUADMAT_TUPLE_DUMPER_H
#define QUADMAT_TUPLE_DUMPER_H

#include "quadmat/quadmat.h"

using namespace quadmat;

/**
 * A node visitor that dumps tuples from nodes
 *
 * @tparam T
 */
template <typename T>
class TupleDumper {
public:
    explicit TupleDumper(std::vector<std::tuple<Index, Index, T>> &tuples, std::mutex& m) : tuples_(tuples), mutex_(m) {}

    template <typename LeafType>
    void operator()(const std::shared_ptr<LeafType>& leaf, const Offset& offsets, const Shape& shape) const {
        std::lock_guard lock{mutex_};

        for (auto tup : leaf->Tuples()) {
            tuples_.emplace_back(
                    std::get<0>(tup) + offsets.row_offset,
                    std::get<1>(tup) + offsets.col_offset,
                    std::get<2>(tup)
            );
        }
    }
protected:
    std::vector<std::tuple<Index, Index, T>>& tuples_;
    std::mutex& mutex_;
};

/**
 * Utility to dump all tuples from a tree node and/or its children.
 * @param node
 * @return a vector of tuples. Tuples are not sorted.
 */
template <typename T, typename Config = DefaultConfig>
std::vector<std::tuple<Index, Index, T>> DumpTuples(TreeNode<T, Config> node) {
    std::vector<std::tuple<Index, Index, T>> v;
    std::mutex m;
    std::visit(GetLeafVisitor<double, Config>(TupleDumper<T>(v, m)), node);
    return v;
}

/**
 * Utility to dump all tuples from a matrix
 * @param mat
 * @return a vector of tuples. Tuples are not sorted.
 */
template <typename T, typename Config = DefaultConfig>
std::vector<std::tuple<Index, Index, T>> DumpTuples(Matrix<T, Config> mat) {
    return DumpTuples<T, Config>(mat.GetRootBC()->GetChild(0));
}

#endif //QUADMAT_TUPLE_DUMPER_H
