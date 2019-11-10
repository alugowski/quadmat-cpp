// Copyright (C) 2019 Adam Lugowski
// All Rights Reserved.

#ifndef QUADMAT_BASE_ITERATORS_H
#define QUADMAT_BASE_ITERATORS_H

#include <iterator>

namespace quadmat {

    /**
     * Boilerplate for an input iterator.
     *
     * The implementing class needs to provide:
     *   - using iterator_category = std::input_iterator_tag
     *   - typedefs for value_type, pointer, reference, difference_type
     *   - copy constructor
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

        bool operator!=(const derived& rhs) const {
            return !static_cast<const derived*>(this)->operator==(rhs);
        }
    };

    /**
     * Boilerplate for a random access iterator that is based on an index.
     *
     * The implementing class needs to provide:
     *   - using iterator_category = std::random_access_iterator_tag
     *   - typedefs for value_type, pointer, reference, difference_type
     *   - constructor, copy constructor
     *   - operator*()
     *   - operator[](difference_type)
     *
     * If the implementing class uses the index as a base for more operations, likely override these
     *   - operator++()
     *   - operator+=(ptrdiff)
     *
     * The other arithmetic operators are built on the base of these.
     *
     * @tparam derived implementation class
     */
    template<typename IT, typename derived>
    class base_indexed_random_access_iterator {
    public:
        // - default-constructible
        base_indexed_random_access_iterator() : i(0) {}

        // - copy-constructible, copy-assignable and destructible
        explicit base_indexed_random_access_iterator(IT i) : i(i) {}

        derived& operator=(const derived& rhs) const { // NOLINT(cppcoreguidelines-c-copy-assignment-signature,misc-unconventional-assign-operator)
            i = rhs.i;
            return *static_cast<derived*>(this);
        }

        // - Can be incremented

        derived& operator++() {
            ++i;
            return *static_cast<derived*>(this);
        }

        derived operator++(int) { // NOLINT(cert-dcl21-cpp)
            derived temp(*this);
            operator++();
            return temp;
        }

        // - Supports equality/inequality comparisons

        bool operator==(const derived& rhs) const {
            return i == rhs.i;
        }

        bool operator!=(const derived& rhs) const {
            return !operator==(rhs);
        }

        // - Can be dereferenced as an rvalue (Input iterators)
        // - Can be dereferenced as an lvalue (Output iterators)
        // left to derived

        // - Can be decremented

        derived& operator--() {
            *static_cast<derived*>(this) -= 1;
            return *static_cast<derived*>(this);
        }

        derived operator--(int) { // NOLINT(cert-dcl21-cpp)
            derived temp(*this);
            temp -= 1;
            return temp;
        }

        // - Supports arithmetic operators + and -

        derived operator+(std::ptrdiff_t n) const {
            derived temp(*this);
            temp += n;
            return temp;
        }

        friend derived operator+(std::ptrdiff_t lhs, const derived& rhs) {
            derived temp(rhs);
            temp += lhs;
            return temp;
        }

        derived operator-(std::ptrdiff_t n) const {
            derived temp(*this);
            temp += -n;
            return temp;
        }

        std::ptrdiff_t operator-(const derived& rhs) const {
            return i - rhs.i;
        }

        // - Supports inequality comparisons (<, >, <= and >=) between iterators

        bool operator<(const derived& rhs) const {
            return i < rhs.i;
        }

        bool operator>(const derived& rhs) const {
            return i > rhs.i;
        }

        bool operator<=(const derived& rhs) const {
            return i <= rhs.i;
        }

        bool operator>=(const derived& rhs) const {
            return i >= rhs.i;
        }

        // - Supports compound assignment operations += and -=

        derived& operator+=(std::ptrdiff_t n) {
            i += n;
            return *static_cast<derived*>(this);
        }

        derived& operator-=(std::ptrdiff_t n) {
            *static_cast<derived*>(this) += -n;
            return *static_cast<derived*>(this);
        }

        // - Supports offset dereference operator ([])
        // left to derived

    protected:
        IT i;
    };

}
#endif //QUADMAT_BASE_ITERATORS_H
