// Copyright (C) 2019-2020 Adam Lugowski
// All Rights Reserved.

#ifndef QUADMAT_UTIL_H
#define QUADMAT_UTIL_H

#include <algorithm>
#include <ios>
#include <iomanip>
#include <random>
#include <sstream>
#include <tuple>
#include <vector>

#include "quadmat/util/types.h"


namespace quadmat {

     /**
      * Get a permutation vector
      *
      * @tparam Iterator iterator type
      * @tparam Compare comparison function type
      * @param begin beginning of range to sort
      * @param end end of range to sort
      * @param compare comparison function, same as used by std::sort.
      * @return a permutation vector of size (end - begin)
      */
    template <typename Iterator, typename Compare, typename Allocator=std::allocator<std::size_t>>
    std::vector<std::size_t> GetSortPermutation(const Iterator begin, const Iterator end, const Compare& compare) {
        std::vector<std::size_t, Allocator> perm(end - begin);
        std::iota(perm.begin(), perm.end(), 0);
        std::sort(perm.begin(), perm.end(),
                  [&](std::size_t i, std::size_t j){ return compare(*(begin + i), *(begin + j)); });
        return perm;
    }

    /**
     * Apply a permutation to a vector.
     *
     * @tparam T
     * @param vec vector to permute
     * @param perm permutation vector
     * @return a new vector with items in `vec` permuted by `perm`.
     */
    template <typename T, typename Allocator=std::allocator<T>>
    std::vector<T> ApplyPermutation(const std::vector<T>& vec, const std::vector<std::size_t>& perm) {
        std::vector<T, Allocator> permuted_vec(vec.size());
        std::transform(perm.begin(), perm.end(), permuted_vec.begin(),
                       [&](std::size_t i){ return vec[i]; });
        return permuted_vec;
    }

    /**
     * Apply a permutation to a vector inplace.
     *
     * @tparam T
     * @param vec vector to permute
     * @param perm permutation vector. This vector is modified!
     */
    template <typename T>
    void ApplyPermutationInplace(std::vector<T>& vec, std::vector<std::size_t>& perm)
    {
        for (std::size_t i = 0; i < perm.size(); i++) {
            std::size_t current = i;
            while (i != perm[current]) {
                std::size_t next = perm[current];
                std::swap(vec[current], vec[next]);
                perm[current] = current;
                current = next;
            }
            perm[current] = current;
        }
    }

    /**
     * Swap elements (dest_begin + a, dest_begin + b).
     *
     * Serves as base case for variadic version below.
     */
    template <typename Iterator>
    void SwapAt(std::size_t a, std::size_t b, Iterator dest_begin) {
        std::swap(*(dest_begin + a), *(dest_begin + b));
    }

    /**
     * Swap elements (dest_begin + a, dest_begin + b) and similarly for every iterator in dest_rest.
     */
    template <typename Iterator, typename... Iterators>
    void SwapAt(std::size_t a, std::size_t b, Iterator dest_begin, Iterators... dest_rest) {
        std::swap(*(dest_begin + a), *(dest_begin + b));
        SwapAt(a, b, dest_rest...);
    }

    /**
     * Apply a permutation to a range inplace.
     *
     * The permutation vector is modified so this function is variadic to support permuting multiple ranges
     * using the same permutation vector.
     *
     * @tparam Iterators writable iterator types
     * @param perm permutation vector. This vector is modified!
     * @param dest_begins start of the range to permute. end is dest_begin + perm.size()
     */
    template <typename... Iterators>
    void ApplyPermutationInplace(std::vector<std::size_t>& perm, Iterators... dest_begins)
    {
        for (std::size_t i = 0; i < perm.size(); i++) {
            std::size_t current = i;
            while (i != perm[current]) {
                std::size_t next = perm[current];
                // swap the elements indexed by current and next
                SwapAt(current, next, dest_begins...);
                perm[current] = current;
                current = next;
            }
            perm[current] = current;
        }
    }

    /**
     * Same as std::shuffle but uses a stable RNG so the results are repeatable. Useful for tests.
     */
    template <typename Iterator>
    void StableShuffle(Iterator begin, Iterator end, int seed = 0) {
        auto rng = std::mt19937(seed);
        std::shuffle(begin, end, rng);
    }

    /**
     * Slice up a range into n_parts ranges.
     *
     * @tparam Iterator random access iterator
     * @param n_parts
     * @param start
     * @param end
     * @return
     */
    template <typename Iterator>
    auto SliceRanges(int n_parts, const Iterator& start, const Iterator& end) {
        auto n_elements = end - start;
        int size_per_slice = std::ceil((double)n_elements / n_parts);

        std::vector<Range<Iterator>> ret;

        auto cur_start = start;
        while (end - cur_start > size_per_slice) {
            ret.emplace_back(Range<Iterator>{
                cur_start,
                cur_start + size_per_slice
            });
            cur_start += size_per_slice;
        }
        ret.emplace_back(Range<Iterator>{
            cur_start,
            end
        });

        return ret;
    }

    /**
     * Get the number with only the most significant bit set.
     */
    inline Index ClearAllExceptMsb(Index n) {
        if (n <= 0) {
            return 0;
        }

        unsigned msb = 0;
        while (n != 1) {
            n = n / 2;
            msb++;
        }

        return (1ul << msb);
    }

    /**
     * Find the discriminating bit to be used if a particular shape is to be subdivided.
     * @param shape
     * @return
     */
    inline Index GetDiscriminatingBit(const Shape& shape) {
        Index dim_max = std::max(shape.ncols, shape.nrows);
        if (dim_max < 2) {
            return 1;
        }
        return ClearAllExceptMsb(dim_max - 1); // NOLINT(hicpp-signed-bitwise),
    }

    /**
     * Find the discriminating bit of a child inner node given the parent inner node's bit.
     */
    inline Index GetChildDiscriminatingBit(const Index parent_discriminating_bit) {
        return parent_discriminating_bit > 1 ? parent_discriminating_bit >> 1 : 1; // NOLINT(hicpp-signed-bitwise)
    }

    /**
     * Deleter for std::unique_ptr and std::shared_ptr
     */
    template<typename Allocator>
    struct Deleter {
        explicit Deleter(const Allocator& alloc) : alloc_(alloc) { }

        void operator()(typename std::allocator_traits<Allocator>::pointer p) const {
            Allocator aa(alloc_);
            std::allocator_traits<Allocator>::destroy(aa, std::addressof(*p));
            std::allocator_traits<Allocator>::deallocate(aa, p, 1);
        }

    private:
        Allocator alloc_;
    };

    /**
     * Allocate a unique_ptr using the default allocator.
     *
     * This method exists because there is no std::allocate_unique.
     */
    template<typename Config, typename T, typename... Args>
    std::unique_ptr<T, Deleter<typename Config::template Allocator<T>>> allocate_unique(Args &&... args) {
        auto alloc = typename Config::template Allocator<T>();
        using AllocTraits = std::allocator_traits<decltype(alloc)>;

        auto ptr = AllocTraits::allocate(alloc, 1);
        try {
            AllocTraits::construct(alloc, std::addressof(*ptr), std::forward<Args>(args)...);
            using D = Deleter<typename AllocTraits::allocator_type>;
            return std::unique_ptr<T, D>(ptr, D(alloc));
        }
        catch (...) {
            AllocTraits::deallocate(alloc, ptr, 1);
            throw;
        }
    }

    /**
     * Convert a unique_ptr to a shared_ptr. Custom Deleter is maintained.
     */
    template<typename T, typename D>
    std::shared_ptr<T> ToShared(std::unique_ptr<T, D>&& ptr)
    {
        return std::shared_ptr<T>(std::move(ptr));
    }

    /**
     * Allocate a shared_ptr using the default allocator.
     *
     * This method only exists because std::allocate_shared requires repeating long template types.
     */
    template <typename Config, typename T, typename... Args>
    std::shared_ptr<T> allocate_shared(Args&&... args) {
        return std::allocate_shared<T>(typename Config::template Allocator<T>(), args...);
    }

    /**
     * Allocate a shared_ptr using the temporary allocator.
     *
     * This method only exists because std::allocate_shared requires repeating long template types.
     */
    template <typename Config, typename T, typename... Args>
    std::shared_ptr<T> allocate_shared_temp(Args&&... args) {
        return std::allocate_shared<T>(typename Config::template TempAllocator<T>(), args...);
    }

    /**
     * @see TightenBounds()
     *
     * Implemented with std::lower_bound and std::upper_bound binary searches.
     */
    template<typename Iterator>
    typename Iterator::difference_type TightenBoundsStdLib(Iterator &first, Iterator &last,
                             const typename Iterator::value_type &low, const typename Iterator::value_type &high) {
        Iterator tight_first = std::lower_bound(first, last, low);
        last = std::upper_bound(tight_first, last, high);

        auto offset = tight_first - first;
        first = tight_first;

        return offset;
    }

    /**
     * @see TightenBounds()
     *
     * Implemented with a counting linear search. Good for when (last - first) is small.
     */
    template<typename Iterator>
    typename Iterator::difference_type TightenBoundsCounting(Iterator &first, Iterator &last,
                               const typename Iterator::value_type &low, const typename Iterator::value_type &high) {
        int smaller = 0;
        int larger = 0;

        for (Iterator it = first; it != last; ++it) {
            smaller += (*it < low);
            larger += (*it <= high);
        }

        last = first + larger;
        first += smaller;

        return smaller;
    }

    /**
     * Advance `first` to point at the first instance of `low`, or if not present,
     * the smallest element > low.
     *
     * Backtrack `last` to point at the last instance of `high`, or if not present,
     * the largest element < high.
     *
     * If there are no elements between low and high inclusive then first == last.
     *
     * @tparam Iterator RandomAccessIterator
     * @param first iterator on a sorted container. points at <= low, such as begin()
     * @param last iterator on a sorted container. points at >= high, such as end()
     * @param low
     * @param high
     */
    template<typename Iterator>
    typename Iterator::difference_type TightenBounds(Iterator &first, Iterator &last,
                       const typename Iterator::value_type &low, const typename Iterator::value_type &high) {
        if (last - first < 256) {
            return TightenBoundsCounting(first, last, low, high);
        } else {
            return TightenBoundsStdLib(first, last, low, high);
        }
    }
}

#endif //QUADMAT_UTIL_H
