// Copyright (C) 2019 Adam Lugowski
// All Rights Reserved.

#ifndef QUADMAT_UTIL_H
#define QUADMAT_UTIL_H

#include <numeric>
#include <vector>

using std::size_t;
using std::vector;

namespace quadmat {

    /**
     * Get a permutation vector.
     *
     * @param vec vector to sort
     * @param compare comparison function
     * @return a permuation vector for vec of length `vec.size()`
     */
    template <typename T, typename Compare>
    vector<size_t> get_sort_permutation(const vector<T>& vec, Compare& compare)
    {
        vector<std::size_t> perm(vec.size());
        std::iota(perm.begin(), perm.end(), 0);
        std::sort(perm.begin(), perm.end(),
                  [&](size_t i, size_t j){ return compare(vec[i], vec[j]); });
        return perm;
    }

    template <typename T>
    vector<T> permute(const vector<T>& vec, const vector<size_t>& perm)
    {
        vector<T> permuted_vec(vec.size());
        std::transform(perm.begin(), perm.end(), permuted_vec.begin(),
                       [&](std::size_t i){ return vec[i]; });
        return permuted_vec;
    }
}

#endif //QUADMAT_UTIL_H
