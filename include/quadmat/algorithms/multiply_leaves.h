// Copyright (C) 2019 Adam Lugowski
// All Rights Reserved.

#ifndef QUADMAT_MULTIPLY_LEAVES_H
#define QUADMAT_MULTIPLY_LEAVES_H

#include "quadmat/quadtree/leaf_blocks/dcsc_block.h"

namespace quadmat {

    /**
     * Multiply two leaf blocks
     */
    template <typename A_LEAF_TYPE, typename B_LEAF_TYPE, typename RET_IT, class SR, class SPA, typename CONFIG>
    std::shared_ptr<dcsc_block<typename SR::reduce_type, RET_IT, CONFIG>> multiply_pair(
            std::shared_ptr<A_LEAF_TYPE> a,
            std::shared_ptr<B_LEAF_TYPE> b,
            const shape_t& result_shape,
            const SR& semiring = SR()) {

        auto factory = dcsc_block_factory<typename SR::reduce_type, RET_IT, CONFIG>();

        // For each column j in block b
        SPA spa(result_shape.nrows, semiring);

        auto b_columns = b->columns();

        for (auto b_j = b_columns.begin(); b_j != b_columns.end(); ++b_j) {
            auto b_j_column = *b_j;

            auto b_j_row = b_j_column.rows_begin;
            auto b_j_row_end = b_j_column.rows_end;
            auto b_j_value = b_j_column.values_begin;

            // For each element i in j
            while (b_j_row != b_j_row_end) {
                // look up column j in block a
                auto a_i = a->column(*b_j_row);

                if (a_i != a->columns_end()) {
                    // perform the multiply and add in the SpA
                    auto a_i_column = *a_i;
                    spa.scatter(a_i_column.rows_begin, a_i_column.rows_end, a_i_column.values_begin, *b_j_value);
                }

                ++b_j_row;
                ++b_j_value;
            }

            // dump the spa into the result
            factory.add_spa(b_j_column.col, spa);
            spa.clear();
        }

        return factory.finish();
    }
}

#endif //QUADMAT_MULTIPLY_LEAVES_H
