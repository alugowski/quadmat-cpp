// Copyright (C) 2019 Adam Lugowski
// All Rights Reserved.

#ifndef QUADMAT_CSC_BLOCK_H
#define QUADMAT_CSC_BLOCK_H

#include <algorithm>
#include <numeric>
#include <vector>

using std::vector;

#include "quadmat/quadtree/block.h"
#include "quadmat/util.h"

namespace quadmat {

    /**
     * A simple block composed of <row, column, value> triples
     *
     * @tparam T value type, eg. double
     */
    template<typename T, typename IT, typename CONFIG = basic_settings>
    class csc_block: public block<T> {
    public:
        csc_block(const shape_t shape) : block<T>(shape) {}

        /**
         * Create a CSC block from column-ordered tuples. Single pass.
         *
         * @tparam GEN tuple generator template type
         * @param nrows number of rows in this block
         * @param ncols number of columns in this block
         * @param nnn estimated number of nonzeros in this block.
         * @param col_ordered_gen tuple generator. **Must return tuples ordered by column**
         */
        template<typename GEN>
        csc_block(const shape_t shape, const blocknnn_t nnn, const GEN col_ordered_gen) : block<T>(shape), col_ptr(shape.ncols + 1, 0) {
            // reserve memory
            row_ind.reserve(nnn);
            values.reserve(nnn);

            col_ptr[0] = 0;

            IT prev_col = 0;
            blocknnn_t i = 0;
            for (auto tup : col_ordered_gen) {
                IT col = std::get<1>(tup);
                if (prev_col != col) {
                    std::fill(col_ptr.begin() + prev_col + 1, col_ptr.begin() + col + 1, i);
                }
                prev_col = col;

                row_ind.emplace_back(std::get<0>(tup)); // add row
                values.emplace_back(std::get<2>(tup)); // add row
                i++;
            }

            col_ptr[shape.ncols] = i;
            sort_columns();
        }

        /**
         * Create a CSC block from unordered tuples. Requires multiple passes of the tuples. T must be default-constructable.
         *
         * @tparam GEN tuple generator template type
         * @param nrows number of rows in this block
         * @param ncols number of columns in this block
         * @param gen tuple generator. **Must support multiple passes**
         */
        template<typename GEN>
        csc_block(const index_t nrows, const index_t ncols, const GEN gen) : col_ptr(ncols + 1, 0), block<T>(nrows, ncols) {
            // compute number of non-nulls per column
            vector<blocknnn_t, typename CONFIG::template TEMP_ALLOC<blocknnn_t>> col_nnn(ncols, 0);
            for (auto tup : gen) {
                IT col = std::get<1>(tup);
                col_nnn[col]++;
            }

            // cumsum the number of non-nulls per column to get col_ptr
            blocknnn_t cumsum = 0;
            for (blocknnn_t i = 0; i < ncols; i++){
                col_ptr[i] = cumsum;
                cumsum += col_nnn[i];
            }
            blocknnn_t nnn = cumsum;
            col_ptr[ncols] = nnn;

            // allocate memory
            row_ind.resize(nnn);
            values.resize(nnn);

            // write row, value
            std::fill(col_nnn.begin(), col_nnn.end(), 0);

            for (auto tup : gen) {
                IT col = std::get<1>(tup);

                blocknnn_t offset = col_ptr[col] + col_nnn[col]++;

                row_ind[offset] = std::get<0>(tup);
                values[offset] = std::get<2>(tup);
            }

            sort_columns();
        }

        block_size_info size() override {
            return block_size_info{
                    row_ind.size() * sizeof(IT) + col_ptr.size() * sizeof(blocknnn_t),
                    values.size() * sizeof(T),
                    sizeof(csc_block<T, IT, CONFIG>),
                    values.size()
            };
        }
    protected:
        /**
         * Sort columns according to row number.
         */
        void sort_columns() {
            for (index_t i = 0; i < col_ptr.size() - 1; i++) {
                blocknnn_t col_nnn = col_ptr[i+1] - col_ptr[i];
                if (col_nnn > 1) {
                    vector<size_t> perm = get_sort_permutation(
                            row_ind.begin() + col_ptr[i],
                            row_ind.begin() + col_ptr[i+1],
                            std::less<>());

                    apply_permutation_inplace(perm,
                            row_ind.begin() + col_ptr[i],
                            values.begin() + col_ptr[i]);
                }
            }
        }

        vector<IT, typename CONFIG::template ALLOC<IT>> row_ind;
        vector<blocknnn_t, typename CONFIG::template ALLOC<blocknnn_t>> col_ptr;
        vector<T, typename CONFIG::template ALLOC<T>> values;
    };
}

#endif //QUADMAT_CSC_BLOCK_H
