// Copyright (C) 2019-2020 Adam Lugowski
// All Rights Reserved.

#include <benchmark/benchmark.h>

#include <vector>

#include "quadmat/quadmat.h"
using namespace quadmat;

#include "benchmark_utilities.h"

// ClangTidy doesn't identify template uses
#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection"

template <typename IT>
struct SearchProblem {
    std::vector<IT> haystack;
    std::vector<IT> point_needles;
    std::vector<std::pair<IT, IT>> range_needles;
};

/**
 * Generate a col_ind and a lookup pattern.
 */
template <typename IT>
SearchProblem<IT> GenerateSearchProblem(std::size_t length, std::size_t num_lookups = 1000, IT index_min = 0, IT index_max = 1u << 15u) {
    SearchProblem<IT> ret {
        .haystack = GenerateUniform<IT>(length, index_min, index_max, length, true),
    };

    // quadmat range lookups are relatively small range
    const std::size_t kRangeMax = 50;
    std::size_t range_length = 0;

    for (auto point_idx : GenerateUniform<std::size_t>(num_lookups, 0, length - 1, length, false)) {
        // for needle use either a value found or not found
        IT needle = ret.haystack[point_idx];
        if (point_idx % 2) {
            needle -= 1; // a point that might not be found
        }
        ret.point_needles.emplace_back(needle);

        // generate a second point for range searches
        IT needle2 = ret.haystack[std::min(point_idx + range_length, ret.haystack.size() - 1)];
        if (point_idx % 3) {
            needle2 += 1; // a point that might not be found
        }
        ret.range_needles.emplace_back(needle, needle2);

        range_length = (range_length + 1) % kRangeMax;
    }

    return ret;
}

#pragma clang diagnostic push
#pragma ide diagnostic ignored "cert-err58-cpp"

/**
 * Template for array search benchmarks.
 */
template <typename Impl>
static void BM_Search(benchmark::State& state) {
    std::size_t num_lookups = 0;
    const auto problem = GenerateSearchProblem<int32_t>(state.range(0));

    for (auto _ : state) {
        for (const auto& needle : problem.point_needles) {
            auto result = Impl::call(problem.haystack, needle);
            benchmark::DoNotOptimize(result);
        }

        num_lookups += problem.point_needles.size();
    }

    state.counters["Lookups"] = benchmark::Counter(num_lookups, benchmark::Counter::kIsRate);
}

/**
 * Array search: std::lower_bound
 */
struct Search_lower_bound {
    template <typename T>
    static auto call(const std::vector<T>& vec, const T& needle) {
        return std::lower_bound(std::begin(vec), std::end(vec), needle);
    }
};

/**
 * Array search: std::bsearch
 */
struct Search_bsearch {
    static int bsearch_compare(const void *lhs_ptr, const void *rhs_ptr)
    {
        return *static_cast<const int32_t*>(lhs_ptr) - *static_cast<const int32_t*>(rhs_ptr);
    }

    template <typename T>
    static auto call(const std::vector<T>& vec, const T& needle) {
        return std::bsearch(&needle, vec.data(), vec.size(), sizeof(needle), bsearch_compare);
    }
};

/**
 * Branchless binary search based on:
 * https://www.pvk.ca/Blog/2012/07/03/binary-search-star-eliminates-star-branch-mispredictions/
 *
 * THIS VERSION ASSUMES vec size is a power of 2
 */
struct SearchBranchless {
    /* log_2 ceiling */
    static unsigned lb (unsigned long x)
    {
        if (x <= 1) return 0;
        return (8*sizeof(unsigned long))-__builtin_clzl(x-1);
    }

    template <typename T>
    static auto call(const std::vector<T>& vec, const T& needle) {
        int size = vec.size();
        const T* low = vec.data();

        for (unsigned i = lb(size); i != 0; i--) {
            size /= 2;
            unsigned mid = low[size];
            if (mid <= needle)
                low += size;
        }

        return (*low > needle)? -1: low - vec.data();
    }
};

/**
 * Array search: linear, using counting
 */
struct SearchLinearCounting {
    template <typename T>
    static auto call(const std::vector<T>& vec, const T& needle) {
        int smaller = 0;

        for (const auto& val : vec) {
            smaller += (val < needle);
        }

        return smaller;
    }
};

/**
 * Array search: linear, using std::find
 */
struct SearchLinear_find {
    template <typename T>
    static auto call(const std::vector<T>& vec, const T& needle) {
        return std::find(std::begin(vec), std::end(vec), needle);
    }
};

// -------------------------------------------------------------------------

/**
 * Template for tighten bounds benchmarks.
 *
 * Range narrowing starts with a first and last iterators then adjusts them to better fit low/high values.
 */
template <typename Impl>
static void BM_TightenBounds(benchmark::State& state) {
    std::size_t num_lookups = 0;

    const auto problem = GenerateSearchProblem<int32_t>(state.range(0));
    std::size_t start_offset_max = state.range(1);

    // generate starting points for range narrowing
    std::vector<std::pair<std::size_t, std::size_t>> range_starts;
    for (auto [needle, needle2] : problem.range_needles) {
        auto first_idx = std::lower_bound(std::begin(problem.haystack), std::end(problem.haystack), needle)
            - std::begin(problem.haystack);
        auto last_idx = std::upper_bound(std::begin(problem.haystack), std::end(problem.haystack), needle2)
            - std::begin(problem.haystack);

        auto offset = start_offset_max == 0 ? 0 : needle % start_offset_max;
        range_starts.emplace_back(offset > first_idx ? first_idx : first_idx - offset,
                                  std::min(start_offset_max == 0 ? last_idx : last_idx + needle2 % start_offset_max, problem.haystack.size()));
    }

    for (auto _ : state) {
        for (int i = 0 ; i < problem.range_needles.size(); i++) {
            const auto& needles = problem.range_needles[i];
            const auto& starts = range_starts[i];

            auto first = std::begin(problem.haystack) + starts.first;
            auto last = std::begin(problem.haystack) + starts.second;

            Impl::call(first, last, needles.first, needles.second);

            benchmark::DoNotOptimize(first);
            benchmark::DoNotOptimize(last);
        }

        num_lookups += problem.point_needles.size();
    }

    state.counters["Narrows"] = benchmark::Counter(num_lookups, benchmark::Counter::kIsRate);
}

/**
 * Adjusts iterators to point to low and high.
 *
 * Uses two binary searches via std::lower_bound and std::upper_bound.
 */
struct TightenBoundsBinaryStdLib {
    template <typename Iterator>
    static void call(Iterator &first, Iterator &last, typename Iterator::value_type low, typename Iterator::value_type high) {
        first = std::lower_bound(first, last, low);
        last = std::upper_bound(first, last, high);
    }
};

/**
 * Adjusts iterators to point to low and high.
 *
 * Uses a linear search with branching.
 */
struct TightenBoundsLinearBreaking {
    template <typename Iterator>
    static void call(Iterator &first, Iterator &last, typename Iterator::value_type low, typename Iterator::value_type high) {
        while (first != last && *first < low) {
            ++first;
        }

        Iterator& last_candidate = first;

        while (last_candidate != last && high <= last_candidate[1]) {
            ++last_candidate;
        }

        last = last_candidate;
    }
};

/**
 * Adjusts iterators to point to low and high.
 *
 * Uses a branchless linear search that counts how far to advance
 */
struct TightenBoundsLinearCounting {
    template <typename Iterator>
    static void call(Iterator &first, Iterator &last, typename Iterator::value_type low, typename Iterator::value_type high) {
        int smaller = 0;
        int larger = 0;

        for (Iterator it = first; it != last; ++it) {
            smaller += (*it < low);
            larger += (*it > high);
        }

        last = first + larger;
        first += smaller;
    }
};

/**
 * Adjusts iterators to point to low and high.
 *
 * Combines above approaches
 */
struct TightenBoundsHybrid {
    template <typename Iterator>
    static void call(Iterator &first, Iterator &last, typename Iterator::value_type low, typename Iterator::value_type high) {
        if (last - first < 256) {
            TightenBoundsLinearCounting::call(first, last, low, high);
        } else {
            TightenBoundsBinaryStdLib::call(first, last, low, high);
        }
    }
};


// -------------------------------------------------------------------------

/**
 * Template for column lookup benchmarks.
 */
template <typename Impl>
static void BM_GetColumn(benchmark::State& state) {
    std::size_t num_lookups = 0;
    const auto problem = GenerateSearchProblem<int32_t>(state.range(0));
    auto block_ptr = Impl::Factory(problem);
    auto& block = *block_ptr;

    for (auto _ : state) {
        for (const auto& needle : problem.point_needles) {
            auto result = block.GetColumn(needle);
            benchmark::DoNotOptimize(result);
        }

        num_lookups += problem.point_needles.size();
    }

    state.counters["Lookups"] = benchmark::Counter(num_lookups, benchmark::Counter::kIsRate);
}


/**
 * Benchmark lookups for a quadmat DCSC leaf block.
 */
template <typename BlockType = quadmat::DcscBlock<double, int32_t>>
class DcscBlockConstructor {
public:
    using IndexType = typename BlockType::IndexType;
    using ValueType = typename BlockType::ValueType;

    /**
     * Factory method so that the implementation can be all const.
     */
    static std::shared_ptr<BlockType> Factory(const SearchProblem<IndexType>& problem) {
        // create tuples
        std::vector<std::tuple<IndexType, IndexType, ValueType>> tuples;
        tuples.reserve(problem.haystack.size());

        for (auto col : problem.haystack) {
            tuples.emplace_back(col, col, 1.0);
        }

        // sort
        std::sort(std::begin(tuples), std::end(tuples));

        DcscBlockFactory<ValueType, IndexType> factory(problem.haystack.size(), tuples);
        return factory.Finish();
    }
};

/**
 * Benchmark lookups for a windowed shadow block over a DCSC leaf block.
 */
template <typename ShadowBlockType = WindowShadowBlock<int32_t, quadmat::DcscBlock<double, int32_t>>>
class ShadowedDcscBlockConstructor {
public:
    using IndexType = typename ShadowBlockType::IndexType;
    using ValueType = typename ShadowBlockType::ValueType;

    /**
     * Factory method so that the implementation can be all const.
     */
    static std::shared_ptr<ShadowBlockType> Factory(const SearchProblem<IndexType>& problem) {
        auto shadowed_block = DcscBlockConstructor<typename ShadowBlockType::ShadowedLeafType>::Factory(problem);

        return std::make_shared<ShadowBlockType>(
                shadowed_block,
                shadowed_block->ColumnsBegin(),
                shadowed_block->ColumnsEnd(),
                Offset{0, 0},
                Shape{static_cast<Index>(problem.haystack.size()), static_cast<Index>(problem.haystack.size())}
            );
    }
};

// -------------------------------------------------------------------------

static void SearchArguments(benchmark::internal::Benchmark* b) {
    for (auto haystack_size : {
        8u,
        64u,
        128u,
        256u,
        1u << 9u,
        1u << 10u,
        1u << 11u,
        1u << 12u,
        1u << 15u}) {
        b->Args({haystack_size});
    }
}

// GetColumn benchmarks
BENCHMARK_TEMPLATE(BM_GetColumn, DcscBlockConstructor<quadmat::DcscBlock<double, int32_t>>)->Apply(SearchArguments);
BENCHMARK_TEMPLATE(BM_GetColumn, ShadowedDcscBlockConstructor<WindowShadowBlock<int32_t, quadmat::DcscBlock<double, int32_t>>>)->Apply(SearchArguments);

// search benchmarks
BENCHMARK_TEMPLATE(BM_Search, SearchLinearCounting)->Apply(SearchArguments);
BENCHMARK_TEMPLATE(BM_Search, SearchLinear_find)->Apply(SearchArguments);
BENCHMARK_TEMPLATE(BM_Search, Search_lower_bound)->Apply(SearchArguments);
BENCHMARK_TEMPLATE(BM_Search, Search_bsearch)->Apply(SearchArguments);
BENCHMARK_TEMPLATE(BM_Search, SearchBranchless)->Apply(SearchArguments);

static void NarrowingArguments(benchmark::internal::Benchmark* b) {
    for (int haystack_size = 8; haystack_size <= 1u << 15u; haystack_size *= 8) {
        for (int offset = 64; offset <= 1024; offset *= 2) {
            if (offset < haystack_size) {
                b->Args({haystack_size, offset});
            }
        }
    }
}

// bounds tightening benchmarks
BENCHMARK_TEMPLATE(BM_TightenBounds, TightenBoundsBinaryStdLib)->Apply(NarrowingArguments);
BENCHMARK_TEMPLATE(BM_TightenBounds, TightenBoundsLinearBreaking)->Apply(NarrowingArguments);
BENCHMARK_TEMPLATE(BM_TightenBounds, TightenBoundsLinearCounting)->Apply(NarrowingArguments);
BENCHMARK_TEMPLATE(BM_TightenBounds, TightenBoundsHybrid)->Apply(NarrowingArguments);

#pragma clang diagnostic pop // static
#pragma clang diagnostic pop // bad unused method detection

BENCHMARK_MAIN();
