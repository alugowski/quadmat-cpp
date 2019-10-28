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
    template <typename T, typename IT, typename ADDER = std::plus<T>, typename CONFIG=basic_settings>
    class sparse_spa {
    public:
        /**
         * @param size ignored here. Used in dense SpA.
         */
        explicit sparse_spa(size_t size, const ADDER& adder = ADDER()) : adder(adder) {}

        /**
         * Update the SpA with an entire row.
         *
         * @tparam ROW_ITER
         * @tparam VAL_ITER
         * @param rows_iter input iterator. start of the rows range
         * @param rows_end input iterator. end of the rows range
         * @param values_iter input iterator. Start of the values range. End assumed to be values_iter + (rows_end - rows_iter)
         */
        template <typename ROW_ITER, typename VAL_ITER>
        void update(ROW_ITER rows_iter, ROW_ITER rows_end, VAL_ITER values_iter) {
            while (rows_iter != rows_end) {
                update(*rows_iter, *values_iter);

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
        void update(const IT key, const T& value) {
            auto it = m.find(key);
            if (it == m.end()) {
                // insert
                m.emplace_hint(it, key, value);
            } else {
                // replace
                it->second = adder(it->second, value);
            }
        }

        const ADDER& adder;
        std::map<IT, T, std::less<IT>, typename CONFIG::template TEMP_ALLOC<std::pair<const IT, T>>> m;
    };
}

#endif //QUADMAT_SPA_H
