// Copyright (C) 2019-2020 Adam Lugowski
// All Rights Reserved.

#ifndef QUADMAT_DENSE_SPA_H
#define QUADMAT_DENSE_SPA_H

#include "quadmat/config.h"

namespace quadmat {

    /**
     * A dense SpA is an array.
     */
    template <typename IT, typename SR, typename CONFIG=default_config>
    class dense_spa {
    public:
        /**
         * @param size ignored here. Used in dense SpA.
         */
        explicit dense_spa(size_t size, const SR& semiring = SR()) : semiring(semiring), x(size), mark(size) {
            w.reserve(1024);
        }

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
        void scatter(ROW_ITER rows_iter, const ROW_ITER& rows_end, VAL_ITER values_iter) {
            while (rows_iter != rows_end) {
                const IT row = *rows_iter;

                x[row] = semiring.add(x[row], *values_iter);

                if (!mark[row]) {
                    mark[row] = true;
                    w.emplace_back(row);
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
         * @tparam ROW_ITER
         * @tparam VAL_ITER
         * @param rows_iter input iterator. start of the rows range
         * @param rows_end input iterator. end of the rows range
         * @param values_iter input iterator over SR::map_type_l. Start of the values range, with end assumed to be values_iter + (rows_end - rows_iter)
         */
        template <typename ROW_ITER, typename VAL_ITER>
        void scatter(ROW_ITER rows_iter, const ROW_ITER& rows_end, VAL_ITER values_iter, const typename SR::map_type_r& b_val) {
            while (rows_iter != rows_end) {
                const IT row = *rows_iter;

                x[row] = semiring.add(x[row], semiring.multiply(*values_iter, b_val));

                if (!mark[row]) {
                    mark[row] = true;
                    w.emplace_back(row);
                }

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
            std::sort(w.begin(), w.end());

            for (auto i : w) {
                row_ind.emplace_back(i);
                values.emplace_back(x[i]);
            }
        }

        /**
         * @return true if the SpA has no elements set.
         */
        [[nodiscard]] bool empty() const {
            return w.empty();
        }

        /**
         * Clear the SpA for reuse.
         */
        void clear() {
            for (auto i : w) {
                x[i] = typename SR::reduce_type();
                mark[i] = false;
            }
            w.clear();
        }

    protected:

        const SR& semiring;

        /**
         * Dense array that holds the accumulated values.
         *
         * Size is spa size, i.e. number of rows in output matrix.
         */
        std::vector<typename SR::reduce_type, typename CONFIG::template TEMP_ALLOC<typename SR::reduce_type>> x;


        /**
         * Dense array of flags. x[i] is nonempty iff w[i] is true.
         *
         * Size is spa size, i.e. number of rows in output matrix.
         */
        std::vector<bool, typename CONFIG::template TEMP_ALLOC<bool>> mark;

        /**
         * List of i where mark[i] is true.
         */
        std::vector<IT, typename CONFIG::template TEMP_ALLOC<IT>> w;
    };
}

#endif //QUADMAT_DENSE_SPA_H
