// Copyright (C) 2019 Adam Lugowski
// All Rights Reserved.

#include <functional>
#include <numeric>
#include <vector>

#include <tbb/parallel_for.h>
#include <tbb/parallel_reduce.h>

#include "../catch.hpp"

TEST_CASE("TBB is present", "TBB Parallel For and Reduce"){
    int size = 10000;
    auto values = std::vector<int>(size);

    // set all values to 1
    tbb::parallel_for( tbb::blocked_range<int>(0, values.size()),
                       [&](tbb::blocked_range<int> r)
                       {
                           for (int i=r.begin(); i<r.end(); ++i)
                           {
                               values[i] = 1;
                           }
                       });

    // sum all values
    int total = tbb::parallel_reduce(
            tbb::blocked_range<std::vector<int>::iterator>(values.begin(), values.end()),
            0,
            [&](const tbb::blocked_range<std::vector<int>::iterator>& r, int value)->int {
                return std::accumulate(r.begin(), r.end(), value);
            },
            std::plus<>()
    );

    REQUIRE(total == size);
}
