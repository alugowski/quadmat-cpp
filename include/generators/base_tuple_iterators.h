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
     *   - constructor and copy constructor
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

}
#endif //QUADMAT_BASE_TUPLE_ITERATORS_H
