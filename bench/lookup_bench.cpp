// Copyright (C) 2019-2020 Adam Lugowski
// All Rights Reserved.

#include <benchmark/benchmark.h>

#include <utility>
#include <map>
#include <unordered_map>

#include "quadmat/quadmat.h"
#include "benchmark_utilities.h"

using namespace quadmat;

// ClangTidy doesn't identify template uses
#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection"

template <typename IT>
struct Problem {
    std::size_t num_columns;
    std::vector<IT> nn_cols;
    std::vector<IT> keys;
};

/**
 * Generate a column non-null pattern and a lookup pattern.
 */
template <typename IT>
Problem<IT> GenerateLookupProblem(std::size_t num_columns, double fill_fraction, double lookup_fraction) {
    return Problem<IT> {
        .num_columns = num_columns,
        .nn_cols = GenerateUniformIndices<IT>(num_columns, fill_fraction, 0, true),
        .keys = GenerateUniformIndices<IT>(num_columns, lookup_fraction, 1, false)
    };
}

/**
 * A bitmask that can only perform hit testing.
 */
template <typename IT>
class VectorBoolIndex {
public:
    using IndexType = IT;

    explicit VectorBoolIndex(std::vector<bool>&& mask) : mask_(mask) {}

    [[nodiscard]] std::size_t GetBytesUsed() const {
        // assume 1 bit per bool
        return sizeof(mask_) + mask_.size() / 8; // NOLINT(bugprone-sizeof-container)
    }

    [[nodiscard]] bool IsColumnEmpty(const IT col) const {
        return !mask_[col];
    }

    /**
     * Factory method so that the implementation can be all const.
     */
    static VectorBoolIndex<IT> Factory(const Problem<IT>& problem) {
        std::vector<bool> mask(problem.num_columns);

        // mark columns that exist
        for (auto col : problem.nn_cols) {
            mask[col] = true;
        }

        return VectorBoolIndex<IT>(std::move(mask));
    }

private:
    const std::vector<bool> mask_;
};

/**
 * A CSC index. Only indptr is tested, the row and value arrays are ignored.
 *
 * O(n) space.
 */
template <typename IT>
class CscIndex {
public:
    using IndexType = IT;

    explicit CscIndex(std::vector<IT>&& indptr) : indptr_(indptr) {}

    [[nodiscard]] std::size_t GetBytesUsed() const {
        return sizeof(indptr_) + sizeof(IT) * indptr_.size(); // NOLINT(bugprone-sizeof-container)
    }

    [[nodiscard]] bool IsColumnEmpty(const IT col) const {
        return indptr_[col] == indptr_[col + 1];
    }

    /**
     * Factory method so that the implementation can be all const.
     */
    static CscIndex<IT> Factory(const Problem<IT>& problem) {
        std::vector<IT> indptr(problem.num_columns + 1);

        // compute column counts
        for (auto col : problem.nn_cols) {
            indptr[col]++;
        }

        // convert to column pointers
        IT cumsum = 0;
        for (std::size_t i = 0; i < indptr.size(); ++i) {
            IT add = indptr[i];
            indptr[i] += cumsum;
            cumsum += add;
        }

        return CscIndex<IT>(std::move(indptr));
    }

private:
    const std::vector<IT> indptr_;
};

/**
 * A DCSC index.
 *
 * O(k) space.
 */
template <typename IT>
class DcscIndex {
public:
    using IndexType = IT;

    explicit DcscIndex(std::vector<IT>&& col_ind, std::vector<BlockNnn>&& col_ptr) :
        col_ind_(col_ind),
        col_ptr_(col_ptr)
        {}

    [[nodiscard]] std::size_t GetBytesUsed() const {
        return sizeof(col_ind_) + sizeof(IT) * col_ind_.size() +
            sizeof(col_ptr_) + sizeof(BlockNnn) * col_ptr_.size(); // NOLINT(bugprone-sizeof-container)
    }

    [[nodiscard]] bool IsColumnEmpty(const IT col) const {
        auto pos = std::lower_bound(begin(col_ind_), end(col_ind_), col);
        return pos == end(col_ind_) || *pos != col;
    }

    /**
     * Factory method so that the implementation can be all const.
     */
    static DcscIndex<IT> Factory(const Problem<IT>& problem) {
        std::vector<IT> col_ind;
        std::vector<BlockNnn> col_ptr;

        IT prev_col = -1;
        BlockNnn i = 0;
        // problem.nn_cols is sorted
        for (auto col : problem.nn_cols) {
            if (prev_col != col) {
                // new column
                col_ind.emplace_back(col);
                col_ptr.emplace_back(i);
            }
            prev_col = col;
            i++;
        }
        col_ptr.emplace_back(i);

        return DcscIndex<IT>(std::move(col_ind), std::move(col_ptr));
    }

private:
    const std::vector<IT> col_ind_;
    const std::vector<BlockNnn> col_ptr_;
};

/**
 * Value type used by map indices. Simulates indices into row and value arrays.
 */
struct MapValue {
    BlockNnn start;
    BlockNnn end;
};

/**
 * An index using a map.
 *
 * O(k) space.
 */
template <typename MapType>
class MapIndex {
public:
    using IndexType = typename MapType::key_type;

    explicit MapIndex(MapType&& map) : map_(map) {}

    [[nodiscard]] std::size_t GetBytesUsed() const {
        // large underestimation
        return sizeof(map_) + sizeof(IndexType) * map_.size() + sizeof(MapValue) * map_.size(); // NOLINT(bugprone-sizeof-container)
    }

    [[nodiscard]] bool IsColumnEmpty(const IndexType col) const {
        return map_.find(col) == map_.end();
    }

    /**
     * Factory method so that the implementation can be all const.
     */
    static MapIndex<MapType> Factory(const Problem<IndexType>& problem) {
        MapType map;
        std::vector<BlockNnn> col_ptr;

        IndexType prev_col = -1;
        BlockNnn i = 0;
        // problem.nn_cols is sorted
        for (auto col : problem.nn_cols) {
            if (prev_col != col) {
                // new column
                map[prev_col] = MapValue{i, i};
            }
            prev_col = col;
            i++;
        }

        if (prev_col != -1) {
            map[prev_col] = MapValue{i, i};
        }

        return MapIndex<MapType>(std::move(map));
    }

private:
    const MapType map_;
};

/**
 * An index that uses a full quadmat DCSC leaf block.
 */
template <typename BlockType>
class DcscBlockIndex {
public:
    using IndexType = typename BlockType::IndexType;
    using ValueType = typename BlockType::ValueType;

    explicit DcscBlockIndex(std::shared_ptr<BlockType> block_ptr)
        : block_ptr_(std::move(block_ptr)), block_(*(block_ptr_.get())) {
        column_ref_ = block_.ConstructReusableColumnRef();
    }

    [[nodiscard]] std::size_t GetBytesUsed() const {
        auto size_info = block_.GetSize();
        return size_info.index_bytes - sizeof(IndexType) * block_.GetNnn();
    }

    [[nodiscard]] bool IsColumnEmpty(const IndexType col) {
        return !block_.GetColumn(col, column_ref_);
    }

    /**
     * Factory method so that the implementation can be all const.
     */
    static DcscBlockIndex<BlockType> Factory(const Problem<IndexType>& problem) {
        // create tuples
        std::vector<std::tuple<IndexType, IndexType, ValueType>> tuples;
        tuples.reserve(problem.nn_cols.size());

        for (auto col : problem.nn_cols) {
            tuples.emplace_back(col, col, 1.0);
        }

        // sort
        std::sort(std::begin(tuples), std::end(tuples));

        DcscBlockFactory<ValueType, IndexType> factory(problem.nn_cols.size(), tuples);
        return DcscBlockIndex<BlockType>(factory.Finish());
    }

    [[nodiscard]] const std::shared_ptr<BlockType>& getBlockPtr() const {
        return block_ptr_;
    }

private:
    std::shared_ptr<BlockType> block_ptr_;
    const BlockType& block_;
    typename BlockType::ColumnRef column_ref_;
};

/**
 * An index that uses a shadowed DCSC leaf block.
 */
template <typename BlockType>
class ShadowedDcscBlockIndex {
public:
    using IndexType = typename BlockType::IndexType;
    using ValueType = typename BlockType::ValueType;
    using DcscType = typename BlockType::ShadowedLeafType;

    explicit ShadowedDcscBlockIndex(std::shared_ptr<BlockType> block_ptr)
        : block_ptr_(std::move(block_ptr)), block_(*(block_ptr_.get())) {
        column_ref_ = block_.ConstructReusableColumnRef();
    }

    [[nodiscard]] std::size_t GetBytesUsed() const {
        auto size_info = block_.GetSize();
        return size_info.index_bytes;
    }

    [[nodiscard]] bool IsColumnEmpty(const IndexType col) {
        return !block_.GetColumn(col, column_ref_);
    }

    /**
     * Factory method so that the implementation can be all const.
     */
    static ShadowedDcscBlockIndex<BlockType> Factory(const Problem<IndexType>& problem) {
        auto shadowed_block = DcscBlockIndex<DcscType>::Factory(problem).getBlockPtr();

        return ShadowedDcscBlockIndex<BlockType>(
            std::make_shared<BlockType>(
                shadowed_block,
                shadowed_block->ColumnsBegin(),
                shadowed_block->ColumnsEnd(),
                Offset{0, 0},
                Shape{static_cast<Index>(problem.num_columns), static_cast<Index>(problem.num_columns)}
                )
            );
    }

private:
    std::shared_ptr<BlockType> block_ptr_;
    const BlockType& block_;
    typename BlockType::ColumnRef column_ref_;
};

/**
 * Test whether column is empty or not
 */
template <typename Impl>
static void BM_LookupHit(benchmark::State& state) {
    const std::size_t num_columns = state.range(0);
    const std::size_t col_nnn = state.range(1);
    double fill_fraction = static_cast<double>(col_nnn) / num_columns;
    double lookup_fraction = state.range(2) / 1000.0;

    using IndexType = typename Impl::IndexType;

    const auto problem = GenerateLookupProblem<IndexType>(num_columns, fill_fraction, lookup_fraction);

    // construct implementation
    Impl impl = Impl::Factory(problem);

    std::size_t num_lookups = 0;
    std::size_t num_hits = 0;

    for (auto _ : state) {
        for (auto key : problem.keys) {
            if (!impl.IsColumnEmpty(key)) {
                num_hits++;
            }
        }
        num_lookups += problem.keys.size();
    }

    state.counters["IndexBits"] = benchmark::Counter(8u * sizeof(IndexType));
    state.counters["IndexSizeBytes"] = benchmark::Counter(impl.GetBytesUsed());
    state.counters["Fill%"] = benchmark::Counter(fill_fraction * 100.);
    state.counters["Lookup%"] = benchmark::Counter(lookup_fraction * 100.);
    state.counters["Lookups"] = benchmark::Counter(num_lookups, benchmark::Counter::kIsRate);
    state.counters["Hit%"] = benchmark::Counter(100. * num_hits / num_lookups);
}


#pragma clang diagnostic push
#pragma ide diagnostic ignored "cert-err58-cpp"

// Common configurations of all the parameters.
const std::pair<int64_t, int64_t> kNumColumns32Small = {1u << 16u, 1u << 22u};
const std::pair<int64_t, int64_t> kNumColumns32Large = {1u << 22u, 1u << 26u};

const std::pair<int64_t, int64_t> kColNNNs = {1u << 8u, 1u << 14u};

const std::pair<int64_t, int64_t> kLookupFractionNumeratorRangeSmall = {8, 1000}; // 0.8% to 100%
const std::pair<int64_t, int64_t> kLookupFractionNumeratorRangeLarge = {1, 8}; // 0.1% to 0.8%

const std::vector<std::pair<int64_t, int64_t>> k32bitSmall = {kNumColumns32Small, kColNNNs, kLookupFractionNumeratorRangeSmall};
const std::vector<std::pair<int64_t, int64_t>> k32bitLarge = {kNumColumns32Large, kColNNNs, kLookupFractionNumeratorRangeLarge};

// bitmask hit testing
BENCHMARK_TEMPLATE(BM_LookupHit, VectorBoolIndex<int32_t>)->Ranges(k32bitSmall);
BENCHMARK_TEMPLATE(BM_LookupHit, VectorBoolIndex<int32_t>)->Ranges(k32bitLarge);

// CSC-style indexing
BENCHMARK_TEMPLATE(BM_LookupHit, CscIndex<int32_t>)->Ranges(k32bitSmall);
BENCHMARK_TEMPLATE(BM_LookupHit, CscIndex<int32_t>)->Ranges(k32bitLarge);

// DCSC-style indexing
BENCHMARK_TEMPLATE(BM_LookupHit, DcscIndex<int32_t>)->Ranges(k32bitSmall);
BENCHMARK_TEMPLATE(BM_LookupHit, DcscIndex<int32_t>)->Ranges(k32bitLarge);

// ordered map indexing
BENCHMARK_TEMPLATE(BM_LookupHit, MapIndex<std::map<int32_t, MapValue>>)->Ranges(k32bitSmall);
BENCHMARK_TEMPLATE(BM_LookupHit, MapIndex<std::map<int32_t, MapValue>>)->Ranges(k32bitLarge);

// unordered map indexing
BENCHMARK_TEMPLATE(BM_LookupHit, MapIndex<std::unordered_map<int32_t, MapValue>>)->Ranges(k32bitSmall);
BENCHMARK_TEMPLATE(BM_LookupHit, MapIndex<std::unordered_map<int32_t, MapValue>>)->Ranges(k32bitLarge);

// benchmark quadmat dcsc blocks
BENCHMARK_TEMPLATE(BM_LookupHit, DcscBlockIndex<DcscBlock<double, int32_t>>)->Ranges(k32bitSmall);
BENCHMARK_TEMPLATE(BM_LookupHit, DcscBlockIndex<DcscBlock<double, int32_t>>)->Ranges(k32bitLarge);

// benchmark quadmat window shadow block on top of dcsc block
BENCHMARK_TEMPLATE(BM_LookupHit, ShadowedDcscBlockIndex<WindowShadowBlock<int32_t, DcscBlock<double, int32_t>>>)->Ranges(k32bitSmall);
BENCHMARK_TEMPLATE(BM_LookupHit, ShadowedDcscBlockIndex<WindowShadowBlock<int32_t, DcscBlock<double, int32_t>>>)->Ranges(k32bitLarge);

#pragma clang diagnostic pop // static
#pragma clang diagnostic pop // bad unused method detection

BENCHMARK_MAIN();
