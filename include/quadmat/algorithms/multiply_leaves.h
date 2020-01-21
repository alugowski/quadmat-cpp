// Copyright (C) 2019-2020 Adam Lugowski
// All Rights Reserved.

#ifndef QUADMAT_MULTIPLY_LEAVES_H
#define QUADMAT_MULTIPLY_LEAVES_H

#include "quadmat/quadtree/leaf_blocks/dcsc_block.h"

namespace quadmat {

    /**
     * Multiply two leaf blocks
     */
    template <typename LeafTypeA, typename LeafTypeB, typename RetIT, class Semiring, class Spa, typename Config>
    std::shared_ptr<DcscBlock<typename Semiring::ReduceType, RetIT, Config>> MultiplyPair(
            std::shared_ptr<LeafTypeA> a,
            std::shared_ptr<LeafTypeB> b,
            const Shape& result_shape,
            const Semiring& semiring = Semiring()) {

        auto factory = DcscBlockFactory<typename Semiring::ReduceType, RetIT, Config>();

        // For each column j in block b
        Spa spa(result_shape.nrows, semiring);

        auto b_columns = b->GetColumns();

        for (auto b_j = b_columns.begin(); b_j != b_columns.end(); ++b_j) {
            auto b_j_column = *b_j;

            auto b_j_row = b_j_column.rows_begin;
            auto b_j_row_end = b_j_column.rows_end;
            auto b_j_value = b_j_column.values_begin;

            // For each element i in j
            while (b_j_row != b_j_row_end) {
                // look up column j in block a
                auto a_i_column = a->GetColumn(*b_j_row);

                if (a_i_column.IsColFound()) {
                    // perform the multiply and add in the SpA
                    spa.Scatter(a_i_column.GetRowsBegin(), a_i_column.GetRowsEnd(), a_i_column.GetValuesBegin(), *b_j_value);
                }

                ++b_j_row;
                ++b_j_value;
            }

            // dump the spa into the result
            factory.AddColumnFromSpa(b_j_column.col, spa);
            spa.Clear();
        }

        return factory.Finish();
    }
}

#endif //QUADMAT_MULTIPLY_LEAVES_H
