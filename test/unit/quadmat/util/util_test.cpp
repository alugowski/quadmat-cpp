// Copyright (C) 2019-2020 Adam Lugowski
// All Rights Reserved.

#include <functional>

#include "../../../test_dependencies/catch.hpp"

#include "quadmat/quadmat.h"

using namespace quadmat;

using Catch::Matchers::Equals;

TEST_CASE("Dense String Matrix") {
    SECTION("empty matrix") {
        int size = 4;

        DenseStringMatrix smat({size, size});

        smat.FillTuples(SimpleTuplesGenerator<double, int>::GetEmptyTuples());

        std::string str = smat.ToString();

        std::string expected = "   \n"
                               "   \n"
                               "   \n"
                               "   ";

        REQUIRE(str == expected);
    }
    SECTION("small identity matrix") {
        int size = 4;
        DenseStringMatrix smat({size, size});
        IdentityTuplesGenerator<double, int> gen(size);

        smat.FillTuples(gen);

        std::string str = smat.ToString();

        std::string expected = "1      \n"
                               "  1    \n"
                               "    1  \n"
                               "      1";

        REQUIRE(str == expected);
    }
    SECTION("Kepner-Gilbert graph") {
        Shape shape = SimpleTuplesGenerator<double, int>::GetKepnerGilbertGraphShape();
        DenseStringMatrix smat(shape);

        smat.FillTuples(SimpleTuplesGenerator<double, int>::GetKepnerGilbertGraphTuples());

        std::string str = smat.ToString();

        std::string expected =
                "      1      \n"
                "1            \n"
                "      1   1 1\n"
                "1           1\n"
                "  1         1\n"
                "    1   1    \n"
                "  1          ";

        REQUIRE(str == expected);
    }
}

TEST_CASE("Vector Sorting and Permutation") {
    std::vector<int> unsorted = {8, 2, 5, 3, 5, 6, 1};
    std::vector<int> sorted(unsorted);
    std::sort(sorted.begin(), sorted.end(), std::less<>());

    // permute
    std::vector<size_t> permutation = GetSortPermutation(unsorted.begin(), unsorted.end(), std::less<>());

    SECTION("out of place permutation") {
        std::vector<int> permute_sorted = ApplyPermutation(unsorted, permutation);
        REQUIRE_THAT(permute_sorted, Equals(sorted));
    }

    SECTION("in-place permutation") {
        SECTION("vector") {
            std::vector<int> vec(unsorted);
            ApplyPermutationInplace(vec, permutation);
            REQUIRE_THAT(vec, Equals(sorted));
        }
        SECTION("range single") {
            std::vector<int> vec(unsorted);
            ApplyPermutationInplace(permutation, vec.begin());
            REQUIRE_THAT(vec, Equals(sorted));
        }
        SECTION("range multi") {
            std::vector<int> vec(unsorted);
            std::vector<int> vec2(unsorted);
            ApplyPermutationInplace(permutation, vec.begin(), vec2.begin());
            REQUIRE_THAT(vec, Equals(sorted));
            REQUIRE_THAT(vec2, Equals(sorted));
        }
    }

    SECTION("shuffle") {
        std::vector<int> vec(sorted);
        REQUIRE_THAT(vec, Equals(sorted));

        StableShuffle(std::begin(vec), std::end(vec));
        REQUIRE_THAT(vec, !Equals(sorted));
        std::vector<int> vec_shuffled(vec);

        // re-sort to make sure nothing went missing
        std::sort(std::begin(vec), std::end(vec), std::less<>());
        REQUIRE_THAT(vec, Equals(sorted));

        // re-shuffle to test stability
        StableShuffle(std::begin(vec), std::end(vec));
        REQUIRE_THAT(vec, Equals(vec_shuffled));
    }

    SECTION("TightenBounds") {
        static const std::initializer_list<int> haystack_init = {0, 0, 1, 4, 4, 4, 6, 7, 9};
        static const std::initializer_list<int> extra_needles = {-2, -1, 8, 10, 11};

        std::string haystack_size = GENERATE(values({"small", "large"}));

        SECTION(haystack_size) {
            std::vector<int> haystack = haystack_init;

            if (haystack_size == "large") {
                for (int i = 0; i < 256; ++i) {
                    haystack.push_back(4);
                }
                std::sort(std::begin(haystack), std::end(haystack));
            }

            // for every pair of needles in the haystack
            auto low = GENERATE(values(haystack_init), values(extra_needles));
            auto high = GENERATE(values(haystack_init), values(extra_needles));

            if (low > high) {
                std::swap(low, high);
            }

            // find the needles
            auto expected_first = std::lower_bound(haystack.begin(), haystack.end(), low);
            auto expected_last = std::upper_bound(haystack.begin(), haystack.end(), high);

            // try every possible offset of the first and last iterators
            for (auto first_offset = 0; first_offset <= std::min<int>(20, expected_first - haystack.begin()); first_offset++) {
                for (auto last_offset = 0; last_offset <= std::min<int>(20, (haystack.end() - expected_last)); last_offset++) {

                    // test all implementations
                    // if we were testing a class we could use TEMPLATE_TEST_CASE, but that doesn't work here
                    for (auto version = 0; version < 3; version++) {

                        auto test_first = haystack.begin() + first_offset;
                        auto test_last = haystack.end() - last_offset;

                        switch (version) {
                            case 0:quadmat::TightenBoundsStdLib(test_first, test_last, low, high);
                                break;
                            case 1:quadmat::TightenBoundsCounting(test_first, test_last, low, high);
                                break;
                            case 2:quadmat::TightenBounds(test_first, test_last, low, high);
                                break;
                            default:REQUIRE(false);
                        }

                        if (expected_first == expected_last) {
                            REQUIRE(test_first == test_last);
                        } else {
                            REQUIRE(test_first == expected_first);
                            REQUIRE(test_last == expected_last);
                        }
                    }
                }
            }
        }
    }
}

TEST_CASE("Slicing") {
    int size = GENERATE(0, 7, 10);
    SECTION(std::string("vector size ") + std::to_string(size)) {

        std::vector<int> original(size);
        std::iota(original.begin(), original.end(), 0);

        int num_parts = GENERATE(1, 2, 3, 20);
        SECTION(std::to_string(num_parts) + " splits") {
            auto ranges = SliceRanges(num_parts, begin(original), end(original));

            // make sure we have the right number of ranges
            int expected_num_parts = (size == 0 ? 1 : std::min(size, num_parts));
            REQUIRE(ranges.size() == expected_num_parts);

            // make sure sizes are right
            int total_range_size = std::accumulate(begin(ranges), end(ranges), 0,
                                                   [](int sum, auto r) { return sum + r.size(); });
            REQUIRE(total_range_size == size);

            // make sure the ranges cover the right elements
            std::vector<int> copy;
            for (auto range : ranges) {
                REQUIRE(range.size() >= size / (expected_num_parts + 1));
                REQUIRE(range.size() * (expected_num_parts - 1) <= size);

                copy.insert(copy.end(), begin(range), end(range));
            }

            REQUIRE_THAT(copy, Equals(original));
        }
    }
}

TEST_CASE("Numeric Utilities") {
    SECTION("ClearAllExceptMsb") {
        REQUIRE(ClearAllExceptMsb(0) == 0);
        REQUIRE(ClearAllExceptMsb(1) == 1);
        REQUIRE(ClearAllExceptMsb(2) == 2);
        REQUIRE(ClearAllExceptMsb(3) == 2);
        REQUIRE(ClearAllExceptMsb(0b100111000) == 0b100000000);
        REQUIRE(ClearAllExceptMsb(0b111111000) == 0b100000000);
        REQUIRE(ClearAllExceptMsb(0b111111111) == 0b100000000);
        REQUIRE(ClearAllExceptMsb((1ul << 63ul) - 1ul) == (1ul << 62ul));
        REQUIRE(ClearAllExceptMsb(std::numeric_limits<Index>::max()) == (1ul << 62ul));
    }
    SECTION("discriminating bit") {
        REQUIRE(GetDiscriminatingBit({0, 0}) == 1);
        REQUIRE(GetDiscriminatingBit({1, 1}) == 1);
        REQUIRE(GetDiscriminatingBit({7, 7}) == 4);
        REQUIRE(GetDiscriminatingBit({8, 8}) == 4);
        REQUIRE(GetDiscriminatingBit({9, 9}) == 8);

        REQUIRE(GetChildDiscriminatingBit(0) == 1);
        REQUIRE(GetChildDiscriminatingBit(1) == 1);
        REQUIRE(GetChildDiscriminatingBit(2) == 1);
        REQUIRE(GetChildDiscriminatingBit(4) == 2);
        REQUIRE(GetChildDiscriminatingBit(1ul << 62ul) == 1ul << 61ul);
    }
}