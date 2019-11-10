// Copyright (C) 2019 Adam Lugowski
// All Rights Reserved.

#ifndef QUADMAT_SPA_H
#define QUADMAT_SPA_H

#include <map>

#include "config.h"

namespace quadmat {

    /**
     * A sparse SpA is essentially a map.
     */
    template <typename IT, typename SR, typename CONFIG=default_config>
    class sparse_spa {
    public:
        /**
         * @param size ignored here. Used in dense SpA.
         */
        explicit sparse_spa(size_t size, const SR& semiring = SR()) : semiring(semiring) {}

        /**
         * Update the SpA with (row, value) pairs, i.e. an entire column.
         *
         * Useful for addition.
         *
         * @tparam ROW_ITER
         * @tparam VAL_ITER
         * @param rows_iter input iterator. start of the rows range
         * @param rows_end input iterator. end of the rows range
         * @param values_iter input iterator. Start of the values range. End assumed to be values_iter + (rows_end - rows_iter)
         */
        template <typename ROW_ITER, typename VAL_ITER>
        void update(ROW_ITER rows_iter, const ROW_ITER& rows_end, VAL_ITER values_iter) {
            while (rows_iter != rows_end) {
                update(*rows_iter, *values_iter);

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
         * @tparam ROW_ITER
         * @tparam VAL_ITER
         * @param rows_iter input iterator. start of the rows range
         * @param rows_end input iterator. end of the rows range
         * @param values_iter input iterator over SR::map_type_l. Start of the values range, with end assumed to be values_iter + (rows_end - rows_iter)
         */
        template <typename ROW_ITER, typename VAL_ITER>
        void update(ROW_ITER rows_iter, const ROW_ITER& rows_end, VAL_ITER values_iter, const typename SR::map_type_r& b_val) {
            while (rows_iter != rows_end) {
                update(*rows_iter, semiring.multiply(*values_iter, b_val));

                ++rows_iter;
                ++values_iter;
            }
        }

        /**
         * Copy the contents of the SpA to a row index vector and a values vector. Contents are copied using emplace_back().
         * Copy is ordered by row.
         *
         * @tparam ROW_VEC
         * @tparam VAL_VEC
         * @param row_ind vector to write row indices
         * @param values vector to write values
         */
        template <typename ROW_VEC, typename VAL_VEC>
        void emplace_back_result(ROW_VEC& row_ind, VAL_VEC& values) {
            for (auto it : m) {
                row_ind.emplace_back(it.first);
                values.emplace_back(it.second);
            }
        }

        /**
         * Clear the SpA for reuse.
         */
        void clear() {
            m.clear();
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
        void update(const IT key, const typename SR::reduce_type& value) {
            auto it = m.find(key);
            if (it == m.end()) {
                // insert
                m.emplace_hint(it, key, value);
            } else {
                // replace
                it->second = semiring.add(it->second, value);
            }
        }

        const SR& semiring;
        std::map<IT, typename SR::reduce_type, std::less<IT>, typename CONFIG::template TEMP_ALLOC<std::pair<const IT, typename SR::reduce_type>>> m;
    };
}

#endif //QUADMAT_SPA_H
