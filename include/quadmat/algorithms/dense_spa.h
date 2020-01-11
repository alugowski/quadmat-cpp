// Copyright (C) 2019-2020 Adam Lugowski
// All Rights Reserved.

#ifndef QUADMAT_DENSE_SPA_H
#define QUADMAT_DENSE_SPA_H

#include "quadmat/config.h"

namespace quadmat {

    /**
     * A dense SpA is an array.
     */
    template <typename IT, typename Semiring, typename Config=DefaultConfig>
    class DenseSpa {
    public:
        /**
         * @param size ignored here. Used in dense SpA.
         */
        explicit DenseSpa(size_t size, const Semiring& semiring = Semiring()) : semiring_(semiring), x_(size), mark_(size) {
            w_.reserve(1024);
        }

        /**
         * Update the SpA with (row, value) pairs, i.e. an entire column.
         *
         * Useful for addition.
         *
         * @tparam RowIterator
         * @tparam ValueIterator
         * @param rows_iter input iterator. start of the rows range
         * @param rows_end input iterator. end of the rows range
         * @param values_iter input iterator. Start of the values range. End assumed to be values_iter + (rows_end - rows_iter)
         */
        template <typename RowIterator, typename ValueIterator>
        void Scatter(RowIterator rows_iter, const RowIterator& rows_end, ValueIterator values_iter) {
            while (rows_iter != rows_end) {
                const IT row = *rows_iter;

                x_[row] = semiring_.Add(x_[row], *values_iter);

                if (!mark_[row]) {
                    mark_[row] = true;
                    w_.emplace_back(row);
                }

                ++rows_iter;
                ++values_iter;
            }
        }

        /**
         * Update the SpA with (row, value) pairs, i.e. an entire column, where each value must be multiplied by a
         * single value first.
         *
         * Useful for multiplication.
         *
         * @tparam RowIterator
         * @tparam ValueIterator
         * @param rows_iter input iterator. start of the rows range
         * @param rows_end input iterator. end of the rows range
         * @param values_iter input iterator over SR::MapTypeA. Start of the values range, with end assumed to be values_iter + (rows_end - rows_iter)
         */
        template <typename RowIterator, typename ValueIterator>
        void Scatter(RowIterator rows_iter, const RowIterator& rows_end, ValueIterator values_iter, const typename Semiring::MapTypeB& b_val) {
            while (rows_iter != rows_end) {
                const IT row = *rows_iter;

                x_[row] = semiring_.Add(x_[row], semiring_.Multiply(*values_iter, b_val));

                if (!mark_[row]) {
                    mark_[row] = true;
                    w_.emplace_back(row);
                }

                ++rows_iter;
                ++values_iter;
            }
        }

        /**
         * Copy the contents of the SpA to a row index vector and a values vector. Contents are copied using emplace_back().
         * Copy is ordered by row.
         *
         * @tparam RowVector
         * @tparam ValueVector
         * @param row_ind vector to write row indices
         * @param values vector to write values
         */
        template <typename RowVector, typename ValueVector>
        void EmplaceBackResult(RowVector& row_ind, ValueVector& values) {
            std::sort(w_.begin(), w_.end());

            for (auto i : w_) {
                row_ind.emplace_back(i);
                values.emplace_back(x_[i]);
            }
        }

        /**
         * @return true if the SpA has no elements set.
         */
        [[nodiscard]] bool IsEmpty() const {
            return w_.empty();
        }

        /**
         * Clear the SpA for reuse.
         */
        void Clear() {
            for (auto i : w_) {
                x_[i] = typename Semiring::ReduceType();
                mark_[i] = false;
            }
            w_.clear();
        }

    protected:

        const Semiring& semiring_;

        /**
         * Dense array that holds the accumulated values.
         *
         * Size is spa size, i.e. number of rows in output matrix.
         */
        std::vector<typename Semiring::ReduceType, typename Config::template TempAllocator<typename Semiring::ReduceType>> x_;


        /**
         * Dense array of flags. x[i] is nonempty iff w[i] is true.
         *
         * Size is spa size, i.e. number of rows in output matrix.
         */
        std::vector<bool, typename Config::template TempAllocator<bool>> mark_;

        /**
         * List of i where mark[i] is true.
         */
        std::vector<IT, typename Config::template TempAllocator<IT>> w_;
    };
}

#endif //QUADMAT_DENSE_SPA_H
