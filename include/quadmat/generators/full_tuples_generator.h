// Copyright (C) 2019-2020 Adam Lugowski
// All Rights Reserved.

#ifndef QUADMAT_FULL_TUPLES_GENERATOR_H
#define QUADMAT_FULL_TUPLES_GENERATOR_H

#include <iterator>
#include <tuple>

#include "quadmat/util/types.h"
#include "quadmat/util/base_iterators.h"

namespace quadmat {

    /**
     * Generator for a full matrix, i.e. <i, j, 1> tuples.
     *
     * @tparam T type of the 1
     * @tparam IT type of the indices
     */
    template<typename T, typename IT>
    class FullTuplesGenerator {
    public:

        /**
         * Input Iterator type that this generator emits
         */
        class Iterator: public BaseInputIterator<Iterator> {
        public:
            using iterator_category = std::input_iterator_tag;
            using value_type = std::tuple<IT, IT, T>;
            using pointer = value_type*;
            using reference = value_type&;
            using difference_type = std::ptrdiff_t;

            Iterator(const Shape &shape, const T &value, IT row, IT col) : shape_(shape), value_(value), row_(row),
                                                                           col_(col) {}

            Iterator(const Iterator& rhs) : shape_(rhs.shape_), value_(rhs.value_), row_(rhs.row_),
                                            col_(rhs.col_) {}

            value_type operator*() {
                return std::tuple<IT, IT, T>(row_, col_, value_);
            }

            Iterator& operator++() {
                if (row_ == shape_.nrows -1) {
                  row_ = 0;
                    col_++;
                } else {
                    row_++;
                }
                return *this;
            }

            bool operator==(const Iterator& rhs) const {
                return row_ == rhs.row_ && col_ == rhs.col_;
            }

        private:
            const Shape shape_;
            const T& value_;
            IT row_;
            IT col_;
        };

        /**
         * Create a generator that will generate tuples (i, i, 1) in the range [0, n)
         * @param n
         */
        explicit FullTuplesGenerator(Shape shape, const T& value) :
            begin_iter_(shape, value, 0, 0),
            end_iter_(shape, value, 0, shape.ncols)
                {}


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

#endif //QUADMAT_FULL_TUPLES_GENERATOR_H
