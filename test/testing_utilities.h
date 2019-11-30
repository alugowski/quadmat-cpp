// Copyright (C) 2019 Adam Lugowski
// All Rights Reserved.

#ifndef QUADMAT_TESTING_UTILITIES_H
#define QUADMAT_TESTING_UTILITIES_H

#include "quadmat/quadmat.h"

using namespace quadmat;

#include "problem_generator.h"

/**
 * A node visitor that dumps tuples from nodes
 *
 * @tparam T
 */
template <typename T>
class tuple_dumper {
public:
    explicit tuple_dumper(vector<std::tuple<index_t, index_t, T>> &tuples) : tuples(tuples) {}

    template <typename LEAF>
    void operator()(const std::shared_ptr<LEAF> leaf, offset_t offsets, shape_t shape) const {
        for (auto tup : leaf->tuples()) {
            tuples.emplace_back(
                    std::get<0>(tup) + offsets.row_offset,
                    std::get<1>(tup) + offsets.col_offset,
                    std::get<2>(tup)
            );
        }
    }
protected:
    vector<std::tuple<index_t, index_t, T>>& tuples;
};

/**
 * Utility to dump all tuples from a tree node and/or its children.
 * @param node
 * @return a vector of tuples. Tuples are not sorted.
 */
template <typename T, typename CONFIG = default_config>
vector<std::tuple<index_t, index_t, T>> dump_tuples(tree_node_t<T, CONFIG> node) {
    vector<std::tuple<index_t, index_t, T>> v;
    std::visit(leaf_visitor<double>(tuple_dumper<T>(v)), node);
    return v;
}

/**
 * Utility to dump all tuples from a matrix
 * @param mat
 * @return a vector of tuples. Tuples are not sorted.
 */
template <typename T, typename CONFIG = default_config>
vector<std::tuple<index_t, index_t, T>> dump_tuples(matrix<T, CONFIG> mat) {
    return dump_tuples(mat.get_root_bc()->get_child(0));
}

/**
 * Utility function to identify whether or not a node is a leaf.
 *
 * @param node
 * @return true if node is a leaf node, false otherwise
 */
template <typename T, typename CONFIG = default_config>
bool is_leaf(tree_node_t<T, CONFIG> node) {
    return std::visit(overloaded{
            [](const std::monostate& ignored) { return false; },
            [](const std::shared_ptr<future_block<T, CONFIG>>& ignored) { return false; },
            [](const std::shared_ptr<inner_block<T, CONFIG>>& ignored) { return false; },
            [](const leaf_node_t<T, CONFIG>& ignored) { return true; }
    }, node);
}

/**
 * A node visitor that prints matrix structure
 *
 * @tparam T
 */
template <typename T>
class structure_printer {
public:
    explicit structure_printer(std::ostream &os) : os(os) {}

    template <typename LEAF>
    void operator()(const std::shared_ptr<LEAF> leaf, const offset_t& offsets, const shape_t& shape) const {
        // stats
        os << "leaf ( " << shape.nrows << " x " << shape.ncols << " ), at ( "
           << offsets.row_offset << ", " << offsets.col_offset << " )";

        // values
        if (shape.ncols < 40 && shape.nrows < 100) {
            dense_string_matrix smat(shape);
            smat.fill_tuples(leaf->tuples());

            os << "\n" << smat.to_string() << "\n";
        }
    }
protected:
    std::ostream& os;
};

/**
 * Utility to print a matrix structure
 *
 * @param node
 * @return human readable string
 */
template <typename T, typename CONFIG = default_config>
std::string print_structure(tree_node_t<T, CONFIG> node, const shape_t& shape) {
    std::ostringstream ss;
    std::visit(leaf_visitor<T>(structure_printer<T>(ss), shape), node);
    return ss.str();
}

/**
 * Utility to print a matrix structure
 *
 * @param mat
 * @return human readable string
 */
template <typename T, typename CONFIG = default_config>
std::string print_structure(matrix<T, CONFIG> mat) {
    return print_structure<T, CONFIG>(mat.get_root_bc()->get_child(0), mat.get_shape());
}

/**
 * Subdivide a node into an inner block and four children.
 */
template <typename T, typename CONFIG = default_config>
void subdivide_leaf(shared_ptr<block_container<T, CONFIG>> bc, int position, const shape_t& bc_shape) {
    // get the node to subdivide
    tree_node_t<T, CONFIG> node = bc->get_child(position);

    // create inner block to hold subdivided children
    auto new_inner = bc->create_inner(position);
    shape_t new_inner_shape = bc->get_child_shape(position, bc_shape);

    // use triples blocks as an intermediate data structure
    std::array<triples_block<T, index_t, CONFIG>, all_inner_positions.size()> children;

    // route tuples
    vector<std::tuple<index_t, index_t, T>> tuples = dump_tuples(node);
    offset_t offsets = new_inner->get_offsets(SE, {0, 0});
    for (auto tuple : tuples) {
        auto [row, col, value] = tuple;

        // route this tuple
        if (row < offsets.row_offset) {
            if (col < offsets.col_offset) {
                children[NW].add(row, col, value);
            } else {
                children[NE].add(row, col - offsets.col_offset, value);
            }
        } else {
            if (col < offsets.col_offset) {
                children[SW].add(row - offsets.row_offset, col, value);
            } else {
                children[SE].add(row - offsets.row_offset, col - offsets.col_offset, value);
            }
        }
    }

    // convert to dcsc and place in tree
    for (auto new_child_pos : all_inner_positions) {
        new_inner->set_child(
                new_child_pos,
                create_leaf<T, CONFIG>(
                        new_inner->get_child_shape(new_child_pos, new_inner_shape),
                        children[new_child_pos].nnn(),
                        children[new_child_pos].sorted_tuples()));
    }
}

/**
 * Matrix printer
 */
template <typename T, typename CONFIG>
std::ostream& operator<<(std::ostream& os, const matrix<T, CONFIG>& mat) {
    shape_t shape = mat.get_shape();
    // stats
    os << "matrix ( " << shape.nrows << " x " << shape.ncols << " )";

    // values
    if (shape.ncols < 40 && shape.nrows < 100) {
        dense_string_matrix smat(shape);
        smat.fill_tuples(dump_tuples(mat));

        os << "\n" << smat.to_string();
    }
    return os;
}

/**
 * Catch2 matrix printer
 */
namespace Catch {
    template <typename T, typename CONFIG>
    struct StringMaker<matrix<T, CONFIG>> {
        static std::string convert( matrix<T, CONFIG> const& value ) {
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
std::ostream& operator<<(std::ostream& os, const canned_matrix<T, IT> mat) {
    shape_t shape = mat.shape;
    // stats
    os << "matrix ( " << shape.nrows << " x " << shape.ncols << " )";

    // values
    if (shape.ncols < 40 && shape.nrows < 100) {
        dense_string_matrix smat(shape);
        smat.fill_tuples(mat.sorted_tuples);

        os << "\n" << smat.to_string();
    }
    return os;
}

/**
 * Catch2 matrix matcher
 */
template <typename T, typename CONFIG = default_config>
class MatrixEquals : public Catch::MatcherBase<matrix<T, CONFIG>> {
    const shape_t shape;
    vector<tuple<index_t, index_t, T>> my_tuples;
public:
    explicit MatrixEquals(const matrix<T, CONFIG> &mat) : shape(mat.get_shape()), my_tuples(dump_tuples(mat)) {
        std::sort(my_tuples.begin(), my_tuples.end());
    }

    explicit MatrixEquals(const canned_matrix<T, index_t> &mat) : shape(mat.shape), my_tuples(mat.sorted_tuples) {
        std::sort(my_tuples.begin(), my_tuples.end());
    }

    /**
     * Test against a matrix
     */
    [[nodiscard]] bool match(const matrix<T, CONFIG> &test_value) const override {
        // test shape
        if (shape.nrows != test_value.get_shape().nrows ||
            shape.ncols != test_value.get_shape().ncols) {
            return false;
        }

        // test tuples
        auto rhs_tuples = dump_tuples(test_value);

        if (my_tuples.size() != rhs_tuples.size()) {
            return false;
        }

        std::sort(rhs_tuples.begin(), rhs_tuples.end());

        return std::equal(my_tuples.begin(), my_tuples.end(), rhs_tuples.begin());
    }

    /**
     * Produces a string describing what this matcher does. It should
     * include any provided data (the begin/ end in this case) and
     * be written as if it were stating a fact (in the output it will be
     * preceded by the value under test).
     */
    [[nodiscard]] std::string describe() const override {
        std::ostringstream ss;
        ss << "\nMatrixEquals \n" << canned_matrix<T, index_t>{
                .shape = shape,
                .sorted_tuples = my_tuples,
        };
        return ss.str();
    }
};

/**
 * Argument grouping for the sanity check visitor
 */
struct sanity_check_info {
    offset_t offsets;
    shape_t shape;
    index_t expected_discriminating_bit;
    bool are_futures_ok = false;
    bool check_tuples = false;
};

/**
 * Visitor for santiy checking the quad tree
 */
template <typename T, typename CONFIG>
class sanity_check_visitor {
public:
    explicit sanity_check_visitor(sanity_check_info& info) : info(info) {}

    string operator()(const std::monostate& ignored) {
        return "";
    }

    string operator()(const std::shared_ptr<future_block<T, CONFIG>>& fb) {
        return info.are_futures_ok ? "" : "future_block present";
    }

    string operator()(const std::shared_ptr<inner_block<T, CONFIG>> inner) {
        // make sure the discriminating bit makes sense
        if (inner->get_discriminating_bit() != info.expected_discriminating_bit) {
            return std::to_string(inner->get_discriminating_bit()) +
                   " is not the expected discriminating bit " +
                   std::to_string(info.expected_discriminating_bit);
        }

        if (info.shape.nrows < info.expected_discriminating_bit ||
            info.shape.ncols < info.expected_discriminating_bit) {
            return "inner_block dimensions < discriminating_bit";
        }

        // verify child shapes
        shape_t nw_shape = inner->get_child_shape(NW, info.shape);
        shape_t se_shape = inner->get_child_shape(SE, info.shape);
        if (nw_shape.nrows + se_shape.nrows != info.shape.nrows ||
            nw_shape.ncols + se_shape.ncols != info.shape.ncols) {
            return "child dimensions don't match inner block";
        }

        // recurse on children
        for (auto pos : all_inner_positions) {
            auto child = inner->get_child(pos);

            sanity_check_info child_info(info);

            child_info.offsets = inner->get_offsets(pos, info.offsets);
            child_info.shape = inner->get_child_shape(pos, info.shape);
            child_info.expected_discriminating_bit >>= 1;  // NOLINT(hicpp-signed-bitwise)

            if (child_info.shape.nrows > info.expected_discriminating_bit ||
                child_info.shape.ncols > info.expected_discriminating_bit) {
                return "child dimensions " + std::to_string(child_info.shape.nrows) + ", " + std::to_string(child_info.shape.ncols) +
                       " > than inner block's discriminating_bit " + std::to_string(info.expected_discriminating_bit);
            }

            string ret = std::visit(sanity_check_visitor<T, CONFIG>(child_info), child);
            if (!ret.empty()) {
                return ret;
            }
        }
        return "";
    }

    string operator()(const leaf_category_t<T, int64_t, CONFIG>& leaf) {
        return std::visit(*this, leaf);
    }

    string operator()(const leaf_category_t<T, int32_t, CONFIG>& leaf) {
        return std::visit(*this, leaf);
    }

    string operator()(const leaf_category_t<T, int16_t, CONFIG>& leaf) {
        return std::visit(*this, leaf);
    }

    template <typename IT>
    string operator()(const std::shared_ptr<dcsc_block<T, IT, CONFIG>>& leaf) {
        if (info.shape.nrows <= 0 ||
            info.shape.ncols <= 0) {
            return "leaf dimensions <= 0";
        }

        if (!info.check_tuples) {
            // done
            return "";
        }

        for (auto tup : leaf->tuples()) {
            auto [row, col, value] = tup;

            // make sure tuple is within shape
            if (row >= info.shape.nrows || col >= info.shape.ncols) {
                return "tuple outside of leaf shape";
            }
        }
        return "";
    }

protected:
    sanity_check_info info;
};

/**
 * Make sure the matrix structure makes sense.
 */
template <typename T, typename CONFIG = default_config>
string sanity_check(matrix<T, CONFIG>& mat, bool slow=true) {
    sanity_check_info info {
        .shape = mat.get_shape(),
        .expected_discriminating_bit = single_block_container<T>(mat.get_shape()).get_discriminating_bit() >> 1,
        .check_tuples = slow,
    };
    return std::visit(sanity_check_visitor<T, CONFIG>(info), mat.get_root_bc()->get_child(0));
}

/**
 * Construct a matrix from tuples. The matrix consists of a single leaf.
 *
 * @tparam GEN tuple generator. Must have a begin() and end() that return tuple<IT, IT, T> for some integer type IT
 * @param shape shape of leaf
 * @param nnn estimated number of non-nulls, i.e. col_ordered_gen.size().
 * @param col_ordered_gen tuple generator
 */
template <typename T, typename CONFIG = default_config, typename GEN>
matrix<T, CONFIG> single_leaf_matrix_from_tuples(const shape_t shape, const blocknnn_t nnn, const GEN& col_ordered_gen) {
    matrix<T, CONFIG> ret{shape};

    tree_node_t<T, CONFIG> node = create_leaf<T, CONFIG>(shape, nnn, col_ordered_gen);
    ret.get_root_bc()->set_child(0, node);
    return ret;
}

#endif //QUADMAT_TESTING_UTILITIES_H
