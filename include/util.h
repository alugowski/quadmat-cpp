// Copyright (C) 2019 Adam Lugowski
// All Rights Reserved.

#ifndef QUADMAT_UTIL_H
#define QUADMAT_UTIL_H

#include <algorithm>
#include <ios>
#include <iomanip>
#include <numeric>
#include <sstream>
#include <tuple>
#include <vector>

using std::size_t;
using std::string;
using std::vector;

namespace quadmat {

    /**
     * Class for pretty-printing a matrix into a grid.
     */
    class dense_string_matrix {
    public:
        dense_string_matrix(int nrows, int ncols) : strings(nrows), column_widths(ncols) {
            // pre-fill with empty strings
            for (auto& row_v : strings) {
                row_v.resize(ncols);
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
     * Get a permutation vector.
     *
     * @param vec vector to sort
     * @param compare comparison function
     * @return a permutation vector for vec of length `vec.size()`
     */
    template <typename T, typename Compare>
    vector<size_t> get_sort_permutation(const vector<T>& vec, const Compare& compare) {
        vector<std::size_t> perm(vec.size());
        std::iota(perm.begin(), perm.end(), 0);
        std::sort(perm.begin(), perm.end(),
                  [&](size_t i, size_t j){ return compare(vec[i], vec[j]); });
        return perm;
    }

    template <typename T>
    vector<T> permute(const vector<T>& vec, const vector<size_t>& perm) {
        vector<T> permuted_vec(vec.size());
        std::transform(perm.begin(), perm.end(), permuted_vec.begin(),
                       [&](std::size_t i){ return vec[i]; });
        return permuted_vec;
    }
}

#endif //QUADMAT_UTIL_H
