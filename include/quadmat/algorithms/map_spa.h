// Copyright (C) 2019-2020 Adam Lugowski
// All Rights Reserved.

#ifndef QUADMAT_SPARSE_SPA_H
#define QUADMAT_SPARSE_SPA_H

#include <map>

#include "quadmat/config.h"

namespace quadmat {

    /**
     * A SpA implemented using an ordered map.
     *
     * The primary benefit of this over DenseSpa is that MapSpa is O(k) where k is the number of non-null elements
     * added to the spa. DenseSpa is O(n) where n is the size of the spa, regardless of how many elements are set.
     */
    template <typename IT, typename Semiring, typename Config=DefaultConfig>
    class MapSpa {
    public:
        using IndexType = IT;
        using ValueType = typename Semiring::ReduceType;

        /**
         * @param size ignored here. Used in dense SpA.
         */
        explicit MapSpa(size_t size, const Semiring& semiring = Semiring()) : semiring_(semiring) {}

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
                Update(*rows_iter, *values_iter);

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
        void Scatter(RowIterator rows_iter, const RowIterator& rows_end, ValueIterator values_iter, const typename Semiring::MapTypeB& beta) {
            while (rows_iter != rows_end) {
                Update(*rows_iter, semiring_.Multiply(*values_iter, beta));

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
        void Gather(RowVector& row_ind, ValueVector& values) {
            for (auto it : m_) {
                row_ind.emplace_back(it.first);
                values.emplace_back(it.second);
            }
        }

        /**
         * @return true if the SpA has no elements set.
         */
        [[nodiscard]] bool IsEmpty() const {
            return m_.empty();
        }

        /**
         * Clear the SpA for reuse.
         */
        void Clear() {
            m_.clear();
        }

    protected:

        /**
         * Perform a single update.
         *
         * Users should use the batch functions for performance reasons.
         *
         * @param key
         * @param value
         */
        void Update(const IT key, const typename Semiring::ReduceType& value) {
            auto it = m_.find(key);
            if (it == m_.end()) {
                // insert
                m_.emplace_hint(it, key, value);
            } else {
                // replace
                it->second = semiring_.Add(it->second, value);
            }
        }

        const Semiring& semiring_;
        std::map<IT, typename Semiring::ReduceType, std::less<IT>, typename Config::template TempAllocator<std::pair<const IT, typename Semiring::ReduceType>>> m_;
    };
}

#endif //QUADMAT_SPARSE_SPA_H
