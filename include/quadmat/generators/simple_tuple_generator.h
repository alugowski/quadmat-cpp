// Copyright (C) 2019-2020 Adam Lugowski
// All Rights Reserved.

#ifndef QUADMAT_SIMPLE_TUPLES_GENERATOR_H
#define QUADMAT_SIMPLE_TUPLES_GENERATOR_H

#include <tuple>
#include <vector>

#include "quadmat/util/types.h"

namespace quadmat {

    /**
     * Generator for simple hard-coded matrices.
     *
     * All matrices are column, row sorted.
     *
     * @tparam T data type
     * @tparam IT type of the indices
     */
    template<typename T, typename IT>
    class SimpleTuplesGenerator {
    public:

        static std::vector<std::tuple<IT, IT, T>> GetEmptyTuples() {
            return std::vector<std::tuple<IT, IT, T>>{};
        }

        /**
         * Dimensions of the matrix for the Keptner-Gilbert Graph
         * @return
         */
        static Shape GetKepnerGilbertGraphShape() {
            return {7, 7};
        }

        /**
         * Small 7-node, 12-edge directed graph used on the cover of
         * "Graph Algorithms in the Language of Linear Algebra" edited by Jeremy Kepner and John Gilbert
         */
        static std::vector<std::tuple<IT, IT, T>> GetKepnerGilbertGraphTuples() {
            return std::vector<std::tuple<IT, IT, T>>{
                    {1, 0, 1.0},
                    {3, 0, 1.0},
                    {4, 1, 1.0},
                    {6, 1, 1.0},
                    {5, 2, 1.0},
                    {0, 3, 1.0},
                    {2, 3, 1.0},
                    {5, 4, 1.0},
                    {2, 5, 1.0},
                    {2, 6, 1.0},
                    {3, 6, 1.0},
                    {4, 6, 1.0},
            };
        }
    };

}

#endif //QUADMAT_SIMPLE_TUPLES_GENERATOR_H
