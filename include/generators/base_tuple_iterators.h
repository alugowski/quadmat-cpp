// Copyright (C) 2019 Adam Lugowski
// All Rights Reserved.

#ifndef QUADMAT_BASE_TUPLE_ITERATORS_H
#define QUADMAT_BASE_TUPLE_ITERATORS_H

#include <iterator>

namespace quadmat {

    /**
     * Boilerplate for an input iterator.
     *
     * The implementing class needs to provide:
     *   - using iterator_category = std::input_iterator_tag
     *   - typedefs for value_type, pointer_type, reference_type
     *   - constructor and copy constructor
     *   - operator*()
     *   - operator++()
     *   - operator+=()
     *   - operator==()
     *
     * @tparam derived implementation class
     */
    template<typename derived>
    class base_tuple_input_iterator {
    public:
        derived operator++(int) { // NOLINT(cert-dcl21-cpp)
            derived temp(*this);
            derived::operator++();
            return temp;
        }

        derived operator+(int n) const {
            derived temp(*this);
            temp += n;
            return temp;
        }

        bool operator!=(derived rhs) const {
            return !static_cast<const derived*>(this)->operator==(rhs);
        }
    };

    /**
     * Boilerplate for an input iterator that mainly works using an index.
     *
     * The implementing class needs to provide:
     *   - using iterator_category = std::input_iterator_tag
     *   - typedefs for value_type, pointer_type, reference_type
     *   - constructor and copy constructor
     *   - operator*()
     *
     * @tparam derived implementation class
     */
    template<typename derived>
    class base_indexed_input_iterator {
    public:
        explicit base_indexed_input_iterator(size_t i) : i(i) {}

        derived& operator++() {
            ++i;
            return *static_cast<derived*>(this);
        }

        derived& operator+=(int n) {
            i += n;
            return *static_cast<derived*>(this);
        }

        bool operator==(derived rhs) const {
            return i == rhs.i;
        }

        derived operator++(int) { // NOLINT(cert-dcl21-cpp)
            derived temp(*this);
            ++i;
            return temp;
        }

        derived operator+(int n) const {
            derived temp(*this);
            temp += n;
            return temp;
        }

        bool operator!=(derived rhs) const {
            return !operator==(rhs);
        }

    protected:
        size_t i;
    };

}
#endif //QUADMAT_BASE_TUPLE_ITERATORS_H
