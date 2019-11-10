// Copyright (C) 2019 Adam Lugowski
// All Rights Reserved.

#ifndef QUADMAT_IDENTITY_TUPLES_GENERATOR_H
#define QUADMAT_IDENTITY_TUPLES_GENERATOR_H

#include <iterator>
#include <tuple>

#include "quadmat/util/base_iterators.h"

using std::tuple;

namespace quadmat {

    /**
     * Generator for an identity matrix, i.e. <i, i, 1> tuples.
     *
     * @tparam T type of the 1
     * @tparam IT type of the indices
     */
    template<typename T, typename IT>
    class identity_tuples_generator {
    public:

        /**
         * Input Iterator type that this generator emits
         */
        class iterator: public base_indexed_random_access_iterator<IT, iterator> {
        public:
            using iterator_category = std::input_iterator_tag;
            using value_type = tuple<IT, IT, T>;
            using pointer = value_type*;
            using reference = value_type&;
            using difference_type = std::ptrdiff_t;

            iterator(IT row, IT col_offset) : base_indexed_random_access_iterator<IT, iterator>(row), col_offset(col_offset) {}
            iterator(const iterator& rhs) : base_indexed_random_access_iterator<IT, iterator>(rhs.i), col_offset(rhs.col_offset) {}

            value_type operator*() {
                return tuple<IT, IT, T>(this->i, this->i + col_offset, 1);
            }

        private:
            const IT col_offset;
        };

        /**
         * Create a generator that will generate tuples (i, i, 1) in the range [0, n)
         * @param n
         */
        explicit identity_tuples_generator(IT n) : begin_iter(0, 0), end_iter(n, 0) {}

        /**
         * Create a generator that will generate tuples (i, i, 1) in the closed range [start, end].
         *
         * @param start
         * @param end
         */
        explicit identity_tuples_generator(IT start, IT end) : begin_iter(start, 0), end_iter(end+1, 0) {
            // note that this constructor depends on the stability of overflow.
        }

        iterator begin() const {
            return iterator(begin_iter);
        }

        iterator end() const {
            return iterator(end_iter);
        }

    private:
        const iterator begin_iter;
        const iterator end_iter;
    };

}

#endif //QUADMAT_IDENTITY_TUPLES_GENERATOR_H
