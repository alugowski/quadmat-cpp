// Copyright (C) 2019 Adam Lugowski
// All Rights Reserved.

#ifndef QUADMAT_FULL_TUPLES_GENERATOR_H
#define QUADMAT_FULL_TUPLES_GENERATOR_H

#include <iterator>
#include <tuple>

#include "quadmat/types.h"
#include "quadmat/base_iterators.h"

using std::tuple;

namespace quadmat {

    /**
     * Generator for a full matrix, i.e. <i, j, 1> tuples.
     *
     * @tparam T type of the 1
     * @tparam IT type of the indices
     */
    template<typename T, typename IT>
    class full_tuples_generator {
    public:

        /**
         * Input Iterator type that this generator emits
         */
        class iterator: public base_tuple_input_iterator<iterator> {
        public:
            using iterator_category = std::input_iterator_tag;
            using value_type = tuple<IT, IT, T>;
            using pointer = value_type*;
            using reference = value_type&;
            using difference_type = std::ptrdiff_t;

            iterator(const shape_t &shape, const T &value, IT row, IT col) : shape(shape), value(value), row(row),
                                                                             col(col) {}

            iterator(const iterator& rhs) : shape(rhs.shape), value(rhs.value), row(rhs.row),
                                            col(rhs.col) {}

            value_type operator*() {
                return tuple<IT, IT, T>(row, col, value);
            }

            iterator& operator++() {
                if (row == shape.nrows -1) {
                    row = 0;
                    col++;
                } else {
                    row++;
                }
                return *this;
            }

            bool operator==(iterator rhs) const {
                return row == rhs.row && col == rhs.col;
            }

        private:
            const shape_t shape;
            const T& value;
            IT row;
            IT col;
        };

        /**
         * Create a generator that will generate tuples (i, i, 1) in the range [0, n)
         * @param n
         */
        explicit full_tuples_generator(shape_t shape, const T& value) :
                begin_iter(shape, value, 0, 0),
                end_iter(shape, value, 0, shape.ncols)
                {}


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

#endif //QUADMAT_FULL_TUPLES_GENERATOR_H
