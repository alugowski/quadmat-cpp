// Copyright (C) 2019 Adam Lugowski
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

using std::size_t;
using std::string;
using std::vector;

namespace quadmat {

    /**
     * Class for pretty-printing a matrix into a grid.
     */
    class dense_string_matrix {
    public:
        explicit dense_string_matrix(const shape_t shape) : strings(shape.nrows), column_widths(shape.ncols) {
            // pre-fill with empty strings
            for (auto& row_v : strings) {
                row_v.resize(shape.ncols);
            }
        }

        /**
         * Populate the matrix with tuples from the generator.
         *
         * @tparam GEN has a begin() and end()
         * @param generator will be iterated once
         */
        template <typename GEN>
        void fill_tuples(const GEN& generator) {
            for (auto tup : generator) {
                auto row = std::get<0>(tup);
                auto col = std::get<1>(tup);
                auto value = std::get<2>(tup);

                std::ostringstream ss;
                ss.precision(5);
                ss << value;

                string sval = ss.str();

                strings[row][col] = sval;
                column_widths[col] = std::max(column_widths[col], sval.size());
            }
        }

        /**
         * Dump to a string.
         *
         * @return
         */
        [[nodiscard]] string to_string(const string& line_delimiter = "\n", const string& col_delimiter = " ") const {
            std::ostringstream ss;
            ss << std::left << std::setfill(' ');

            bool first = true;
            for (auto row_v : strings) {
                // delimit with newline on 2nd and subsequent lines
                if (!first) {
                    ss << line_delimiter;
                } else {
                    first = false;
                }

                // dump values
                for (size_t i = 0; i < row_v.size(); i++) {
                    if (i > 0) {
                        ss << col_delimiter;
                    }

                    ss << std::setw(column_widths[i]) << row_v[i];
                }
            }

            return ss.str();
        }

    private:
        vector<vector<string>> strings;
        vector<size_t> column_widths;
    };

     /**
      * Get a permutation vector
      *
      * @tparam ITER iterator type
      * @tparam Compare comparison function type
      * @param begin beginning of range to sort
      * @param end end of range to sort
      * @param compare comparison function, same as used by std::sort.
      * @return a permutation vector of size (end - begin)
      */
    template <typename ITER, typename Compare, typename ALLOC=std::allocator<size_t>>
    vector<size_t> get_sort_permutation(const ITER begin, const ITER end, const Compare& compare) {
        vector<std::size_t, ALLOC> perm(end - begin);
        std::iota(perm.begin(), perm.end(), 0);
        std::sort(perm.begin(), perm.end(),
                  [&](size_t i, size_t j){ return compare(*(begin + i), *(begin + j)); });
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
    template <typename T, typename ALLOC=std::allocator<T>>
    vector<T> apply_permutation(const vector<T>& vec, const vector<size_t>& perm) {
        vector<T, ALLOC> permuted_vec(vec.size());
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
    void apply_permutation_inplace(vector<T>& vec, vector<size_t>& perm)
    {
        for (size_t i = 0; i < perm.size(); i++) {
            size_t current = i;
            while (i != perm[current]) {
                size_t next = perm[current];
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
    template <typename ITER>
    void swap_at(size_t a, size_t b, ITER dest_begin) {
        std::swap(*(dest_begin + a), *(dest_begin + b));
    }

    /**
     * Swap elements (dest_begin + a, dest_begin + b) and similarly for every iterator in dest_rest.
     */
    template <typename ITER, typename... ITERS>
    void swap_at(size_t a, size_t b, ITER dest_begin, ITERS... dest_rest) {
        std::swap(*(dest_begin + a), *(dest_begin + b));
        swap_at(a, b, dest_rest...);
    }

    /**
     * Apply a permutation to a range inplace.
     *
     * The permutation vector is modified so this function is variadic to support permuting multiple ranges
     * using the same permutation vector.
     *
     * @tparam ITERS writable iterator types
     * @param perm permutation vector. This vector is modified!
     * @param dest_begins start of the range to permute. end is dest_begin + perm.size()
     */
    template <typename... ITERS>
    void apply_permutation_inplace(vector<size_t>& perm, ITERS... dest_begins)
    {
        for (size_t i = 0; i < perm.size(); i++) {
            size_t current = i;
            while (i != perm[current]) {
                size_t next = perm[current];
                // swap the elements indexed by current and next
                swap_at(current, next, dest_begins...);
                perm[current] = current;
                current = next;
            }
            perm[current] = current;
        }
    }

    /**
     * Same as std::shuffle but uses a stable RNG so the results are repeatable. Useful for tests.
     */
    template <typename ITER>
    void stable_shuffle(ITER begin, ITER end, int seed = 0) {
        auto rng = std::mt19937(seed);
        std::shuffle(begin, end, rng);
    }

    /**
     * Slice up a range into n_parts ranges.
     *
     * @tparam ITER random access iterator
     * @param n_parts
     * @param start
     * @param end
     * @return
     */
    template <typename ITER>
    auto slice_ranges(int n_parts, const ITER& start, const ITER& end) {
        auto n_elements = end - start;
        int size_per_slice = std::ceil((double)n_elements / n_parts);

        vector<range_t<ITER>> ret;

        auto cur_start = start;
        while (end - cur_start > size_per_slice) {
            ret.emplace_back(range_t<ITER>{
                cur_start,
                cur_start + size_per_slice
            });
            cur_start += size_per_slice;
        }
        ret.emplace_back(range_t<ITER>{
            cur_start,
            end
        });

        return ret;
    }

    /**
     * Get the number with only the most significant bit set.
     */
    inline index_t clear_all_except_msb(index_t n) {
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
    inline index_t get_discriminating_bit(const shape_t& shape) {
        index_t dim_max = std::max(shape.ncols, shape.nrows);
        if (dim_max < 2) {
            return 1;
        }
        return clear_all_except_msb(dim_max - 1); // NOLINT(hicpp-signed-bitwise),
    }
}

#endif //QUADMAT_UTIL_H
