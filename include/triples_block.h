// Copyright (C) 2019 Adam Lugowski
// All Rights Reserved.

#ifndef QUADMAT_TRIPLES_BLOCK_H
#define QUADMAT_TRIPLES_BLOCK_H

#include <numeric>
#include <utility>
#include <vector>

using std::vector;

#include "block.h"
#include "generators/base_tuple_iterators.h"

namespace quadmat {

    /**
     * A simple block composed of <row, column, value> triples
     *
     * @tparam T value type, eg. double
     */
    template<typename T, typename IT, typename CONFIG = basic_settings>
    class triples_block: public block<T> {
    public:
        triples_block(const index_t nrows, const index_t ncols) : block<T>(nrows, ncols) {}

        block_size_info size() override {
            return block_size_info{
                rows.size() * sizeof(IT) + cols.size() * sizeof(IT),
                values.size() * sizeof(T),
                2 * sizeof(std::vector<IT>) + sizeof(std::vector<T>)
            } + block<T>::size();
        }

        /**
         * Add a triple.
         *
         * @param row row index
         * @param col column index
         * @param value value
         */
        void add(IT row, IT col, T value) {
            rows.emplace_back(row);
            cols.emplace_back(col);
            values.emplace_back(value);
        }

        /**
         * Add many tuples
         *
         * @tparam GEN tuple generator template type
         * @param gen tuple generator
         */
        template <typename GEN>
        void add(const GEN gen) {
            for (auto tup : gen) {
                rows.emplace_back(std::get<0>(tup));
                cols.emplace_back(std::get<1>(tup));
                values.emplace_back(std::get<2>(tup));
            }
        }

        /**
         * Input Iterator type for iterating over this block's tuples
         */
        class iterator: public base_indexed_input_iterator<iterator> {
        public:
            using iterator_category = std::input_iterator_tag;
            using value_type = std::tuple<IT, IT, T>;
            using pointer = value_type*;
            using reference = value_type&;
            using difference_type = std::ptrdiff_t;

            explicit iterator(
                    const triples_block<T, IT, CONFIG>& block,
                    size_t i) : base_indexed_input_iterator<iterator>(i), block(block) {}
            iterator(const iterator& rhs) : base_indexed_input_iterator<iterator>(rhs.i), block(rhs.block) {}

            value_type operator*() {
                return std::tuple<IT, IT, T>(block.rows[this->i], block.cols[this->i], block.values[this->i]);
            }

        private:
            const triples_block<T, IT, CONFIG>& block;
        };

        /**
         * Input Iterator type for iterating over this block's tuples in permuted order
         */
        class permuted_iterator: public base_indexed_input_iterator<permuted_iterator> {
        public:
            using iterator_category = std::input_iterator_tag;
            using value_type = std::tuple<IT, IT, T>;
            using pointer = value_type*;
            using reference = value_type&;
            using difference_type = std::ptrdiff_t;

            explicit permuted_iterator(
                    const triples_block<T, IT, CONFIG>& block,
                    shared_ptr<vector<size_t>> permutation,
                    size_t i)
                    : base_indexed_input_iterator<permuted_iterator>(i),
                            permutation(std::move(permutation)),
                            block(block) {}
            permuted_iterator(
                    const permuted_iterator& rhs)
                    : base_indexed_input_iterator<permuted_iterator>(rhs.i),
                            permutation(rhs.permutation),
                            block(rhs.block) {}

            value_type operator*() {
                return std::tuple<IT, IT, T>(
                        block.rows[(*permutation)[this->i]],
                        block.cols[(*permutation)[this->i]],
                        block.values[(*permutation)[this->i]]);
            }

        private:
            shared_ptr<vector<size_t>> permutation;
            const triples_block<T, IT, CONFIG>& block;
        };

        /**
         * @return a tuple iterator pointing at the first tuple
         */
        iterator begin() const {
            return iterator(*this, 0);
        }

        /**
         * @return a one-past-the-end tuple iterator
         */
        iterator end() const {
            return iterator(*this, rows.size());
        }

        /**
         * @return a (begin, end) pair of tuple iterators that return tuples in order sorted by column, row.
         */
        range<permuted_iterator> sorted_range() const {
            shared_ptr<vector<size_t, typename CONFIG::template TEMP_ALLOC<size_t>>> permutation = std::make_shared<vector<size_t, typename CONFIG::template TEMP_ALLOC<size_t>>>(get_sort_permutation());
            return range<permuted_iterator>{permuted_iterator(*this, permutation, 0), permuted_iterator(*this, permutation, rows.size())};
        }

    protected:
        vector<IT, typename CONFIG::template ALLOC<IT>> rows;
        vector<IT, typename CONFIG::template ALLOC<IT>> cols;
        vector<T, typename CONFIG::template ALLOC<T>> values;

        /**
         * Get a permutation that would order the triples by column then row.
         */
        vector<size_t, typename CONFIG::template TEMP_ALLOC<size_t>> get_sort_permutation() const
        {
            std::vector<std::size_t, typename CONFIG::template TEMP_ALLOC<size_t>> p(rows.size());
            std::iota(p.begin(), p.end(), 0);
            std::sort(p.begin(), p.end(),
                      [&](size_t i, size_t j) {
                if (cols[i] != cols[j]) {
                    return cols[i] < cols[j];
                } else if (rows[i] != rows[j]) {
                    return rows[i] < rows[j];
                } else {
                    return i < j;
                }
            });
            return p;
        }
    };
}

#endif //QUADMAT_TRIPLES_BLOCK_H
