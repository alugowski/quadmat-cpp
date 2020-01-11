// Copyright (C) 2019-2020 Adam Lugowski
// All Rights Reserved.

#ifndef QUADMAT_IDENTITY_TUPLES_GENERATOR_H
#define QUADMAT_IDENTITY_TUPLES_GENERATOR_H

#include <iterator>
#include <tuple>

#include "quadmat/util/base_iterators.h"

namespace quadmat {

    /**
     * Generator for an identity matrix, i.e. <i, i, 1> tuples.
     *
     * @tparam T type of the 1
     * @tparam IT type of the indices
     */
    template<typename T, typename IT>
    class IdentityTuplesGenerator {
    public:

        /**
         * Input Iterator type that this generator emits
         */
        class Iterator: public BaseIndexedRandomAccessIterator<IT, Iterator> {
        public:
            using iterator_category = std::input_iterator_tag;
            using value_type = std::tuple<IT, IT, T>;
            using pointer = value_type*;
            using reference = value_type&;
            using difference_type = std::ptrdiff_t;

            Iterator(IT row, IT col_offset) : BaseIndexedRandomAccessIterator<IT, Iterator>(row), col_offset_(col_offset) {}
            Iterator(const Iterator& rhs) : BaseIndexedRandomAccessIterator<IT, Iterator>(rhs.i_), col_offset_(rhs.col_offset_) {}

            value_type operator*() {
                return std::tuple<IT, IT, T>(this->i_, this->i_ + col_offset_, 1);
            }

        private:
            const IT col_offset_;
        };

        /**
         * Create a generator that will generate tuples (i, i, 1) in the range [0, n)
         * @param n
         */
        explicit IdentityTuplesGenerator(IT n) : begin_iter_(0, 0), end_iter_(n, 0) {}

        /**
         * Create a generator that will generate tuples (i, i, 1) in the closed range [start, end].
         *
         * @param start
         * @param end
         */
        explicit IdentityTuplesGenerator(IT start, IT end) : begin_iter_(start, 0), end_iter_(end+1, 0) {
            // note that this constructor depends on the stability of overflow.
        }

        Iterator begin() const {
            return Iterator(begin_iter_);
        }

        Iterator end() const {
            return Iterator(end_iter_);
        }

    private:
        const Iterator begin_iter_;
        const Iterator end_iter_;
    };

}

#endif //QUADMAT_IDENTITY_TUPLES_GENERATOR_H
