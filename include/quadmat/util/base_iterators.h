// Copyright (C) 2019-2020 Adam Lugowski
// All Rights Reserved.

#ifndef QUADMAT_BASE_ITERATORS_H
#define QUADMAT_BASE_ITERATORS_H

#include <iterator>

#include "quadmat/util/types.h"

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
    template<typename Derived>
    class BaseInputIterator {
    public:
        Derived operator++(int) { // NOLINT(cert-dcl21-cpp)
            Derived temp(*this);
            Derived::operator++();
            return temp;
        }

        bool operator!=(const Derived& rhs) const {
            return !static_cast<const Derived*>(this)->operator==(rhs);
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
     * @tparam Derived implementation class
     */
    template<typename IT, typename Derived>
    class BaseIndexedRandomAccessIterator {
    public:
        // - default-constructible
        BaseIndexedRandomAccessIterator() = default;

        // - copy-constructible, copy-assignable and destructible
        explicit BaseIndexedRandomAccessIterator(IT i) : i_(i) {}

        Derived& operator=(const Derived& rhs) const { // NOLINT(cppcoreguidelines-c-copy-assignment-signature,misc-unconventional-assign-operator)
            i_ = rhs.i;
            return *static_cast<Derived*>(this);
        }

        // - Can be incremented

        Derived& operator++() {
            ++i_;
            return *static_cast<Derived*>(this);
        }

        Derived operator++(int) { // NOLINT(cert-dcl21-cpp)
            Derived temp(*this);
            operator++();
            return temp;
        }

        // - Supports equality/inequality comparisons

        bool operator==(const Derived& rhs) const {
            return i_ == rhs.i_;
        }

        bool operator!=(const Derived& rhs) const {
            return !operator==(rhs);
        }

        // - Can be dereferenced as an rvalue (Input iterators)
        // - Can be dereferenced as an lvalue (Output iterators)
        // left to derived

        // - Can be decremented

        Derived& operator--() {
            *static_cast<Derived*>(this) -= 1;
            return *static_cast<Derived*>(this);
        }

        Derived operator--(int) { // NOLINT(cert-dcl21-cpp)
            Derived temp(*this);
            temp -= 1;
            return temp;
        }

        // - Supports arithmetic operators + and -

        Derived operator+(std::ptrdiff_t n) const {
            Derived temp(*this);
            temp += n;
            return temp;
        }

        friend Derived operator+(std::ptrdiff_t lhs, const Derived& rhs) {
            Derived temp(rhs);
            temp += lhs;
            return temp;
        }

        Derived operator-(std::ptrdiff_t n) const {
            Derived temp(*this);
            temp += -n;
            return temp;
        }

        std::ptrdiff_t operator-(const Derived& rhs) const {
            return i_ - rhs.i_;
        }

        // - Supports inequality comparisons (<, >, <= and >=) between iterators

        bool operator<(const Derived& rhs) const {
            return i_ < rhs.i_;
        }

        bool operator>(const Derived& rhs) const {
            return i_ > rhs.i_;
        }

        bool operator<=(const Derived& rhs) const {
            return i_ <= rhs.i_;
        }

        bool operator>=(const Derived& rhs) const {
            return i_ >= rhs.i_;
        }

        // - Supports compound assignment operations += and -=

        Derived& operator+=(std::ptrdiff_t n) {
            i_ += n;
            return *static_cast<Derived*>(this);
        }

        Derived& operator-=(std::ptrdiff_t n) {
            *static_cast<Derived*>(this) += -n;
            return *static_cast<Derived*>(this);
        }

        // - Supports offset dereference operator ([])
        auto operator[](const std::ptrdiff_t n) const {
            return *(*static_cast<Derived*>(this) + n);
        }

    protected:
        IT i_;
    };

    /**
     * Iterator that wraps a base iterator and modifies all dereferenced values by adding an offset.
     */
    template <typename BaseIter>
    class OffsetIterator: public BaseIndexedRandomAccessIterator<BaseIter, OffsetIterator<BaseIter>> {
    public:
        using iterator_category = typename BaseIter::iterator_category;
        using value_type = typename BaseIter::value_type;
        using pointer = typename BaseIter::pointer;
        using reference = typename BaseIter::reference;
        using difference_type = typename BaseIter::difference_type;

        OffsetIterator() = default;
        explicit OffsetIterator(const BaseIter& iter, const typename BaseIter::value_type& offset) : BaseIndexedRandomAccessIterator<BaseIter, OffsetIterator<BaseIter>>(iter), offset_(offset) {}
        OffsetIterator(const OffsetIterator<BaseIter>& rhs) : BaseIndexedRandomAccessIterator<BaseIter, OffsetIterator<BaseIter>>(rhs.i_), offset_(rhs.offset_) {}

        value_type operator*() const {
            return *(this->i_) + offset_;
        }

        void SetBaseIterator(BaseIter new_value) {
            this->i_ = new_value;
        }

    private:
        typename BaseIter::value_type offset_;
    };

    /**
     * Iterator that wraps a reference to a base iterator and modifies all dereferenced values by subtracting an offset.
     */
    template <typename BaseIter>
    class NegativeOffsetReferenceIterator: public BaseIndexedRandomAccessIterator<BaseIter&, NegativeOffsetReferenceIterator<BaseIter>> {
    public:
        using iterator_category = typename BaseIter::iterator_category;
        using value_type = typename BaseIter::value_type;
        using pointer = typename BaseIter::pointer;
        using reference = typename BaseIter::reference;
        using difference_type = typename BaseIter::difference_type;

        NegativeOffsetReferenceIterator() = default;
        explicit NegativeOffsetReferenceIterator(BaseIter& iter, const Index& offset) : BaseIndexedRandomAccessIterator<BaseIter&, NegativeOffsetReferenceIterator<BaseIter>>(iter), offset_(offset) {}
        NegativeOffsetReferenceIterator(const NegativeOffsetReferenceIterator<BaseIter>& rhs) : BaseIndexedRandomAccessIterator<BaseIter&, NegativeOffsetReferenceIterator<BaseIter>>(rhs.i_), offset_(rhs.offset_) {}

        value_type operator*() const {
            return *(this->i_) - offset_;
        }

    private:
        const Index& offset_;
    };

    /**
     * Iterator that wraps a tuple iterator and returns an offset version of that iterator's tuples
     */
    template <typename BaseIter>
    class OffsetTuplesIterator: public BaseIter {
    public:
        using iterator_category = typename BaseIter::iterator_category;
        using value_type = typename BaseIter::value_type;
        using pointer = typename BaseIter::pointer;
        using reference = typename BaseIter::reference;
        using difference_type = typename BaseIter::difference_type;

        OffsetTuplesIterator() = default;
        explicit OffsetTuplesIterator(const BaseIter& iter, const Offset& offsets) : BaseIter(iter), offsets_(offsets) {}
        OffsetTuplesIterator(const OffsetTuplesIterator<BaseIter>& rhs) : BaseIter(rhs), offsets_(rhs.offsets_) {}

        value_type operator*() const {
            auto base_value = BaseIter::operator*();
            return value_type(
                    std::get<0>(base_value) + offsets_.row_offset,
                    std::get<1>(base_value) + offsets_.col_offset,
                    std::get<2>(base_value));
        }

    private:
        const Offset offsets_;
    };

    /**
     * Utility function to offset a range of tuple iterators.
     *
     * @tparam Iterator tuple iterator
     * @param range begin,end pair of tuple iterators
     * @param offsets offsets to subtract
     * @return a tuple iterator range that returns all tuples from `range` but subtracts `offsets` from each one.
     */
    template <typename Iterator>
    Range<OffsetTuplesIterator<Iterator>> OffsetTuplesNeg(const Range<Iterator>& range, const Offset& offsets) {
        Offset negative_offsets = {-offsets.row_offset, -offsets.col_offset};
        return Range<OffsetTuplesIterator<Iterator>> {
                OffsetTuplesIterator<Iterator>(range.begin(), negative_offsets),
                OffsetTuplesIterator<Iterator>(range.end(), negative_offsets)
        };
    }
}
#endif //QUADMAT_BASE_ITERATORS_H
