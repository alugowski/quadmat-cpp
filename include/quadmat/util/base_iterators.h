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
    class base_input_iterator {
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
        base_indexed_random_access_iterator() = default;

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
        auto operator[](const std::ptrdiff_t n) const {
            return *(*static_cast<derived*>(this) + n);
        }

    protected:
        IT i;
    };

    /**
     * Iterator that wraps another iterator and returns an offset version of the base iterator's values
     */
    template <typename BASE_ITER>
    class offset_iterator: public base_indexed_random_access_iterator<BASE_ITER, offset_iterator<BASE_ITER>> {
    public:
        using iterator_category = typename BASE_ITER::iterator_category;
        using value_type = typename BASE_ITER::value_type;
        using pointer = typename BASE_ITER::pointer;
        using reference = typename BASE_ITER::reference;
        using difference_type = typename BASE_ITER::difference_type;

        offset_iterator() = default;
        explicit offset_iterator(const BASE_ITER& iter, const typename BASE_ITER::value_type& offset) : base_indexed_random_access_iterator<BASE_ITER, offset_iterator<BASE_ITER>>(iter), offset(offset) {}
        offset_iterator(const offset_iterator<BASE_ITER>& rhs) : base_indexed_random_access_iterator<BASE_ITER, offset_iterator<BASE_ITER>>(rhs.i), offset(rhs.offset) {}

        value_type operator*() const {
            return *(this->i) + offset;
        }

    private:
        typename BASE_ITER::value_type offset;
    };

    /**
     * Iterator that wraps a tuple iterator and returns an offset version of that iterator's tuples
     */
    template <typename BASE_ITER>
    class offset_tuples_iterator: public BASE_ITER {
    public:
        using iterator_category = typename BASE_ITER::iterator_category;
        using value_type = typename BASE_ITER::value_type;
        using pointer = typename BASE_ITER::pointer;
        using reference = typename BASE_ITER::reference;
        using difference_type = typename BASE_ITER::difference_type;

        offset_tuples_iterator() = default;
        explicit offset_tuples_iterator(const BASE_ITER& iter, const offset_t& offsets) : BASE_ITER(iter), offsets(offsets) {}
        offset_tuples_iterator(const offset_tuples_iterator<BASE_ITER>& rhs) : BASE_ITER(rhs), offsets(rhs.offsets) {}

        value_type operator*() const {
            auto base_value = BASE_ITER::operator*();
            return value_type(
                    std::get<0>(base_value) + offsets.row_offset,
                    std::get<1>(base_value) + offsets.col_offset,
                    std::get<2>(base_value));
        }

    private:
        const offset_t offsets;
    };

    /**
     * Utility function to offset a range of tuple iterators.
     *
     * @tparam ITER tuple iterator
     * @param range begin,end pair of tuple iterators
     * @param offsets offsets to subtract
     * @return a tuple iterator range that returns all tuples from `range` but subtracts `offsets` from each one.
     */
    template <typename ITER>
    range_t<offset_tuples_iterator<ITER>> offset_tuples_neg(const range_t<ITER>& range, const offset_t& offsets) {
        offset_t negative_offsets = {-offsets.row_offset, -offsets.col_offset};
        return range_t<offset_tuples_iterator<ITER>> {
            offset_tuples_iterator<ITER>(range.begin(), negative_offsets),
            offset_tuples_iterator<ITER>(range.end(), negative_offsets)
        };
    }
}
#endif //QUADMAT_BASE_ITERATORS_H
