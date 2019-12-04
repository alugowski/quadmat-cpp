// Copyright (C) 2019 Adam Lugowski
// All Rights Reserved.

#ifndef QUADMAT_TRIPLES_BLOCK_H
#define QUADMAT_TRIPLES_BLOCK_H

#include <numeric>
#include <utility>
#include <vector>

using std::vector;

#include "quadmat/quadtree/block.h"
#include "quadmat/util/base_iterators.h"

namespace quadmat {

    /**
     * A simple block composed of <row, column, value> triples
     *
     * @tparam T value type, eg. double
     */
    template<typename T, typename IT, typename CONFIG = default_config>
    class triples_block: public block<T> {
    public:
        using value_type = T;
        using index_type = IT;
        using config_type = CONFIG;
        using permutation_vector_type = vector<size_t, typename CONFIG::template TEMP_ALLOC<size_t>>;

        triples_block() = default;

        block_size_info size() override {
            return block_size_info{
                rows.size() * sizeof(IT) + cols.size() * sizeof(IT),
                values.size() * sizeof(T),
                sizeof(triples_block<T, IT, CONFIG>),
                values.size()
            };
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
         * Iterator type for iterating over this block's tuples
         */
        class tuple_iterator: public base_indexed_random_access_iterator<size_t, tuple_iterator> {
        public:
            using iterator_category = std::random_access_iterator_tag;
            using value_type = std::tuple<IT, IT, T>;
            using pointer = value_type*;
            using reference = value_type&;
            using difference_type = std::ptrdiff_t;

            tuple_iterator(const triples_block<T, IT, CONFIG>* block, size_t i) : base_indexed_random_access_iterator<size_t, tuple_iterator>(i), block(block) {}
            tuple_iterator(const tuple_iterator& rhs) : base_indexed_random_access_iterator<size_t, tuple_iterator>(rhs.i), block(rhs.block) {}

            value_type operator*() const {
                return std::tuple<IT, IT, T>(block->rows[this->i], block->cols[this->i], block->values[this->i]);
            }

        private:
            const triples_block<T, IT, CONFIG>* block;
        };

        /**
         * Input Iterator type for iterating over this block's tuples in permuted order
         */
        class permuted_iterator: public base_indexed_random_access_iterator<size_t, permuted_iterator> {
        public:
            using iterator_category = std::input_iterator_tag;
            using value_type = std::tuple<IT, IT, T>;
            using pointer = value_type*;
            using reference = value_type&;
            using difference_type = std::ptrdiff_t;

            explicit permuted_iterator(
                    const triples_block<T, IT, CONFIG>* block,
                    shared_ptr<permutation_vector_type> permutation,
                    size_t i)
                    : base_indexed_random_access_iterator<size_t, permuted_iterator>(i),
                            permutation(std::move(permutation)),
                            block(block) {}
            permuted_iterator(
                    const permuted_iterator& rhs)
                    : base_indexed_random_access_iterator<size_t, permuted_iterator>(rhs.i),
                            permutation(rhs.permutation),
                            block(rhs.block) {}

            value_type operator*() const {
                return std::tuple<IT, IT, T>(
                        block->rows[(*permutation)[this->i]],
                        block->cols[(*permutation)[this->i]],
                        block->values[(*permutation)[this->i]]);
            }

        private:
            shared_ptr<permutation_vector_type> permutation;
            const triples_block<T, IT, CONFIG>* block;
        };

        /**
         * @return a (begin, end) pair of tuple iterators that return tuples in order sorted by column, row.
         */
        range_t<tuple_iterator> original_tuples() const {
            return range_t<tuple_iterator>{
                tuple_iterator(this, 0),
                tuple_iterator(this, rows.size())
            };
        }

        /**
         * @return a (begin, end) pair of tuple iterators that return tuples in order sorted by column, row.
         */
        range_t<permuted_iterator> sorted_tuples() const {
            auto permutation = get_sort_permutation();
            return range_t<permuted_iterator>{
                permuted_iterator(this, permutation, 0),
                permuted_iterator(this, permutation, rows.size())
            };
        }

        /**
         *
         * @param permutation a permutation vector
         * @param p_begin start of permutation range
         * @param p_end end of permutation range
         * @return a (begin, end) pair of tuple iterators that return tuples in the order specified by permutation.
         */
        range_t<permuted_iterator> permuted_tuples(shared_ptr<permutation_vector_type> permutation,
                                                   typename permutation_vector_type::iterator p_begin,
                                                   typename permutation_vector_type::iterator p_end) const {
            return range_t<permuted_iterator>{
                permuted_iterator(this, permutation, p_begin - permutation->begin()),
                permuted_iterator(this, permutation, p_end - permutation->begin())
            };
        }

        /**
         * @return number of tuples in this block.
         */
        [[nodiscard]] blocknnn_t nnn() const {
            return values.size();
        }

        /**
         * @param i
         * @return the row index of tuple i
         */
        [[nodiscard]] IT get_row(size_t i) const {
            return rows[i];
        }

        /**
         * @param i
         * @return the column index of tuple i
         */
        [[nodiscard]] IT get_col(size_t i) const {
            return cols[i];
        }

    protected:
        vector<IT, typename CONFIG::template ALLOC<IT>> rows;
        vector<IT, typename CONFIG::template ALLOC<IT>> cols;
        vector<T, typename CONFIG::template ALLOC<T>> values;

        /**
         * Get a permutation that would order the triples by column then row.
         */
        shared_ptr<permutation_vector_type> get_sort_permutation() const
        {
            auto p = std::make_shared<permutation_vector_type>(rows.size());
            std::iota(p->begin(), p->end(), 0);
            std::sort(p->begin(), p->end(),
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
