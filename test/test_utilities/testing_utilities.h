// Copyright (C) 2019-2020 Adam Lugowski
// All Rights Reserved.

#ifndef QUADMAT_TESTING_UTILITIES_H
#define QUADMAT_TESTING_UTILITIES_H

#include <iostream>

#include "quadmat/quadmat.h"
#include "tuple_dumper.h"

#include "problems.h"

using namespace quadmat;

/**
 * A config with a very small split threshold of 4. Useful for testing subdivision.
 */
struct ConfigSplit4 : public DefaultConfig {
    static constexpr BlockNnn LeafSplitThreshold = 4;
};

/**
 * A config that forces use of a CSC index
 */
struct ConfigUseCscIndex : public DefaultConfig {
    static bool ShouldUseDcscBoolMask(Index ncols, std::size_t num_nn_cols) {
        return false;
    }

    static bool ShouldUseCscIndex(Index ncols, std::size_t num_nn_cols) {
        return true;
    }
};

/**
 * A config that forces use of a boolean mask index
 */
struct ConfigUseBoolMaskIndex : public DefaultConfig {
    static bool ShouldUseDcscBoolMask(Index ncols, std::size_t num_nn_cols) {
        return true;
    }

    static bool ShouldUseCscIndex(Index ncols, std::size_t num_nn_cols) {
        return false;
    }
};

/**
 * A config that forbids indexing
 */
struct ConfigNoIndex : public DefaultConfig {
    static bool ShouldUseDcscBoolMask(Index ncols, std::size_t num_nn_cols) {
        return false;
    }

    static bool ShouldUseCscIndex(Index ncols, std::size_t num_nn_cols) {
        return false;
    }
};

/**
 * Utility function to identify whether or not a node is a leaf.
 *
 * @param node
 * @return true if node is a leaf node, false otherwise
 */
template <typename T, typename Config = DefaultConfig>
bool IsLeaf(TreeNode<T, Config> node) {
    return std::visit(overloaded{
            [](const std::monostate& ignored) { return false; },
            [](const std::shared_ptr<FutureBlock<T, Config>>& ignored) { return false; },
            [](const std::shared_ptr<InnerBlock<T, Config>>& ignored) { return false; },
            [](const LeafNode<T, Config>& ignored) { return true; }
    }, node);
}

/**
 * A node visitor that prints matrix structure
 *
 * @tparam T
 */
template <typename T>
class StructurePrinter {
public:
    explicit StructurePrinter(std::ostream &os) : os_(os) {}

    template <typename LeafType>
    void operator()(const std::shared_ptr<LeafType> leaf, const Offset& offsets, const Shape& shape) const {
        // stats
        os_ << "leaf ( " << shape.nrows << " x " << shape.ncols << " ), at ( "
           << offsets.row_offset << ", " << offsets.col_offset << " )";

        // values
        if (shape.ncols < 40 && shape.nrows < 100) {
            DenseStringMatrix smat(shape);
            smat.FillTuples(leaf->tuples());

            os_ << "\n" << smat.ToString() << "\n";
        }
    }
protected:
    std::ostream& os_;
};

/**
 * Utility to print a matrix structure
 *
 * @param node
 * @return human readable string
 */
template <typename T, typename Config = DefaultConfig>
std::string PrintStructure(TreeNode<T, Config> node, const Shape& shape) {
    std::ostringstream ss;
    std::visit(GetLeafVisitor<T>(StructurePrinter<T>(ss), shape), node);
    return ss.str();
}

/**
 * Utility to print a matrix structure
 *
 * @param mat
 * @return human readable string
 */
template <typename T, typename Config = DefaultConfig>
std::string PrintStructure(Matrix<T, Config> mat) {
    return PrintStructure<T, Config>(mat.GetRootBC()->GetChild(0), mat.GetShape());
}

/**
 * Subdivide a node into an inner block and four children.
 */
template <typename T, typename Config = DefaultConfig>
void SubdivideLeaf(std::shared_ptr<BlockContainer<T, Config>> bc, int position, const Shape& bc_shape) {
    // get the node to subdivide
    TreeNode<T, Config> node = bc->GetChild(position);

    // create inner block to hold subdivided children
    auto new_inner = bc->CreateInner(position);
    Shape new_inner_shape = bc->GetChildShape(position, bc_shape);

    // use triples blocks as an intermediate data structure
    std::array<TriplesBlock<T, Index, Config>, kAllInnerPositions.size()> children;

    // route tuples
    std::vector<std::tuple<Index, Index, T>> tuples = DumpTuples(node);
    Offset offsets = new_inner->GetChildOffsets(SE, {0, 0});
    for (auto tuple : tuples) {
        auto [row, col, value] = tuple;

        // route this tuple
        if (row < offsets.row_offset) {
            if (col < offsets.col_offset) {
                children[NW].Add(row, col, value);
            } else {
                children[NE].Add(row, col - offsets.col_offset, value);
            }
        } else {
            if (col < offsets.col_offset) {
                children[SW].Add(row - offsets.row_offset, col, value);
            } else {
                children[SE].Add(row - offsets.row_offset, col - offsets.col_offset, value);
            }
        }
    }

    // convert to dcsc and place in tree
    for (auto new_child_pos : kAllInnerPositions) {
        new_inner->SetChild(
            new_child_pos,
            CreateLeaf<T, Config>(
                new_inner->GetChildShape(new_child_pos, new_inner_shape),
                children[new_child_pos].GetNnn(),
                children[new_child_pos].SortedTuples()));
    }
}

/**
 * Matrix printer
 */
template <typename T, typename Config>
std::ostream& operator<<(std::ostream& os, const Matrix<T, Config>& mat) {
    Shape shape = mat.GetShape();
    // stats
    os << "matrix ( " << shape.nrows << " x " << shape.ncols << " )";

    // values
    if (shape.ncols < 40 && shape.nrows < 100) {
        DenseStringMatrix smat(shape);
        smat.FillTuples(DumpTuples(mat));

        os << "\n" << smat.ToString();
    }
    return os;
}

/**
 * Catch2 matrix printer
 */
namespace Catch {
    template <typename T, typename Config>
    struct StringMaker<Matrix<T, Config>> {
        static std::string convert(Matrix<T, Config> const& value ) {
            std::ostringstream ss;
            ::operator<<(ss, value);
            return ss.str();
        }
    };
}

/**
 * problem printer
 */
template <typename T, typename IT>
std::ostream& operator<<(std::ostream& os, const CannedMatrix<T, IT> mat) {
    Shape shape = mat.shape;
    // stats
    os << "matrix ( " << shape.nrows << " x " << shape.ncols << " )";

    // values
    if (shape.ncols < 40 && shape.nrows < 100) {
        DenseStringMatrix smat(shape);
        smat.FillTuples(mat.sorted_tuples);

        os << "\n" << smat.ToString();
    }
    return os;
}

/**
 * Catch2 matrix matcher
 */
template <typename T, typename Config = DefaultConfig>
class MatrixEquals : public Catch::MatcherBase<Matrix<T, Config>> {
public:
    explicit MatrixEquals(const Matrix<T, Config> &mat) : shape_(mat.GetShape()), my_tuples_(DumpTuples(mat)) {
        std::sort(my_tuples_.begin(), my_tuples_.end());
    }

    explicit MatrixEquals(const CannedMatrix<T, Index> &mat, const Config& = Config()) : shape_(mat.shape), my_tuples_(mat.sorted_tuples) {
        std::sort(my_tuples_.begin(), my_tuples_.end());
    }

    template <typename Gen>
    explicit MatrixEquals(const Shape& shape, Gen tuple_gen, const Config& = Config()) : shape_(shape) {
        for (auto tup : tuple_gen) {
            const Index row = std::get<0>(tup);
            const Index col = std::get<1>(tup);
            const T& value = std::get<2>(tup);

            my_tuples_.emplace_back(row, col, value);
        }

        std::sort(my_tuples_.begin(), my_tuples_.end());
    }

    /**
     * Test against a matrix
     */
    [[nodiscard]] bool match(const Matrix<T, Config> &test_value) const override {
        // test shape
        if (shape_.nrows != test_value.GetShape().nrows ||
            shape_.ncols != test_value.GetShape().ncols) {
            return false;
        }

        // test tuples
        auto rhs_tuples = DumpTuples(test_value);

        if (my_tuples_.size() != rhs_tuples.size()) {
            return false;
        }

        std::sort(rhs_tuples.begin(), rhs_tuples.end());

        return std::equal(my_tuples_.begin(), my_tuples_.end(), rhs_tuples.begin());
    }

    /**
     * Produces a string describing what this matcher does. It should
     * include any provided data (the begin/ end in this case) and
     * be written as if it were stating a fact (in the output it will be
     * preceded by the value under test).
     */
    [[nodiscard]] std::string describe() const override {
        std::ostringstream ss;
        ss << "\nMatrixEquals \n" << CannedMatrix<T, Index>{
                .shape = shape_,
                .sorted_tuples = my_tuples_,
        };
        return ss.str();
    }

private:
    const Shape shape_;
    std::vector<std::tuple<Index, Index, T>> my_tuples_;
};

/**
 * Argument grouping for the sanity check visitor
 */
struct SanityCheckInfo {
    Offset offsets;
    Shape shape;
    Index expected_discriminating_bit;
    bool are_futures_ok = false;
    bool check_tuples = false;
};

/**
 * Visitor for sanity checking the quad tree
 */
template <typename T, typename Config>
class SanityCheckVisitor {
public:
    explicit SanityCheckVisitor(SanityCheckInfo& info) : info_(info) {}

    std::string operator()(const std::monostate& ignored) {
        return "";
    }

    std::string operator()(const std::shared_ptr<FutureBlock<T, Config>>& fb) {
        return info_.are_futures_ok ? "" : "future_block present";
    }

    std::string operator()(const std::shared_ptr<InnerBlock<T, Config>> inner) {
        // make sure the discriminating bit makes sense
        if (inner->GetDiscriminatingBit() != info_.expected_discriminating_bit) {
            return std::to_string(inner->GetDiscriminatingBit()) +
                   " is not the expected discriminating bit " +
                   std::to_string(info_.expected_discriminating_bit);
        }

        // verify child shapes
        Shape nw_shape = inner->GetChildShape(NW, info_.shape);
        Shape ne_shape = inner->GetChildShape(NE, info_.shape);
        Shape sw_shape = inner->GetChildShape(SW, info_.shape);
        Shape se_shape = inner->GetChildShape(SE, info_.shape);
        if (nw_shape.nrows + sw_shape.nrows != info_.shape.nrows ||
            ne_shape.nrows + se_shape.nrows != info_.shape.nrows ||
            nw_shape.ncols + ne_shape.ncols != info_.shape.ncols ||
            sw_shape.ncols + se_shape.ncols != info_.shape.ncols) {
            return "child dimensions don't match inner block";
        }

        // ensure there are no blocks outside dimensions
        if ((ne_shape.ncols <= 0 && !std::holds_alternative<std::monostate>(inner->GetChild(NE))) ||
            (se_shape.ncols <= 0 && !std::holds_alternative<std::monostate>(inner->GetChild(SE))) ||
            (sw_shape.nrows <= 0 && !std::holds_alternative<std::monostate>(inner->GetChild(SW))) ||
            (se_shape.nrows <= 0 && !std::holds_alternative<std::monostate>(inner->GetChild(SE)))) {
            return "child outside dimensions";
        }

        // recurse on children
        for (auto pos : kAllInnerPositions) {
            auto child = inner->GetChild(pos);

            SanityCheckInfo child_info(info_);

            child_info.offsets = inner->GetChildOffsets(pos, info_.offsets);
            child_info.shape = inner->GetChildShape(pos, info_.shape);
            child_info.expected_discriminating_bit >>= 1;  // NOLINT(hicpp-signed-bitwise)

            if (child_info.shape.nrows > info_.expected_discriminating_bit ||
                child_info.shape.ncols > info_.expected_discriminating_bit) {
                return Join::ToString("child dimensions ", child_info.shape.nrows, ", ", child_info.shape.ncols,
                       " > than inner block's discriminating_bit ", info_.expected_discriminating_bit);
            }

            std::string ret = std::visit(SanityCheckVisitor<T, Config>(child_info), child);
            if (!ret.empty()) {
                return ret;
            }
        }
        return "";
    }

    std::string operator()(const LeafNode<T, Config>& leaf) {
        return std::visit(*this, leaf);
    }

    std::string operator()(const LeafCategory<T, int64_t, Config>& leaf) {
        return std::visit(*this, leaf);
    }

    std::string operator()(const LeafCategory<T, int32_t, Config>& leaf) {
        return std::visit(*this, leaf);
    }

    std::string operator()(const LeafCategory<T, int16_t, Config>& leaf) {
        return std::visit(*this, leaf);
    }

    template <typename LeafType>
    std::string operator()(const std::shared_ptr<LeafType>& leaf) {
        if (info_.shape.nrows <= 0 ||
            info_.shape.ncols <= 0) {
            return "leaf dimensions <= 0";
        }

        if (!info_.check_tuples) {
            // done
            return "";
        }

        for (auto tup : leaf->Tuples()) {
            auto [row, col, value] = tup;

            // make sure tuple is within shape
            if (row >= info_.shape.nrows || col >= info_.shape.ncols) {
                return Join::ToString("tuple <", row, ", ", col, ", ", value, "> outside of leaf shape ", info_.shape.ToString());
            }
        }
        return "";
    }

protected:
    SanityCheckInfo info_;
};

/**
 * Make sure the matrix structure makes sense.
 */
template <typename T, typename Config = DefaultConfig>
std::string SanityCheck(Matrix<T, Config>& mat, bool slow= true) {
    SanityCheckInfo info {
        .shape = mat.GetShape(),
        .expected_discriminating_bit = single_block_container<T>(mat.GetShape()).GetDiscriminatingBit() >> 1,
        .check_tuples = slow,
    };
    return std::visit(SanityCheckVisitor<T, Config>(info), mat.GetRootBC()->GetChild(0));
}

/**
 * Construct a matrix from tuples. The matrix consists of a single leaf.
 *
 * @tparam Gen tuple generator. Must have a begin() and end() that return tuple<IT, IT, T> for some integer type IT
 * @param shape shape of leaf
 * @param nnn estimated number of non-nulls, i.e. col_ordered_gen.size().
 * @param col_ordered_gen tuple generator
 */
template <typename T, typename Config = DefaultConfig, typename Gen>
Matrix<T, Config> SingleLeafMatrixFromTuples(const Shape shape, const BlockNnn nnn, const Gen& col_ordered_gen) {
    Matrix<T, Config> ret{shape};

    TreeNode<T, Config> node = CreateLeaf<T, Config>(shape, nnn, col_ordered_gen);
    ret.GetRootBC()->SetChild(0, node);
    return ret;
}

#endif //QUADMAT_TESTING_UTILITIES_H
