// Copyright (C) 2019 Adam Lugowski
// All Rights Reserved.

#ifndef QUADMAT_TYPES_H
#define QUADMAT_TYPES_H

#include <numeric>
#include <utility>
#include <string>
#include <exception>

namespace quadmat {

    typedef int32_t blocknnn_t;
    typedef int64_t index_t;

    /**
     * Utility type to be able to return iterator ranges usable in a foreach.
     * @tparam ITER
     */
    template <typename ITER>
    struct range_t {
        range_t(ITER begin, ITER end) : _begin(begin), _end(end) {}

        ITER _begin;
        ITER _end;

        ITER begin() const { return _begin; }
        ITER end() const { return _end; }

        auto size() const {
            return _end - _begin;
        }
    };

    /**
     * Utility type that describes the shape of a block or matrix. I.e. number of rows and columns.
     */
    struct shape_t {
        index_t nrows = 0;
        index_t ncols = 0;

        bool operator==(const shape_t& rhs) const {
            return nrows == rhs.nrows && ncols == rhs.ncols;
        }

        shape_t& operator=(const shape_t& rhs) = default;

        [[nodiscard]] std::string to_string() const {
            return std::string("{") + std::to_string(nrows) + ", " + std::to_string(ncols) + "}";
        }
    };

    /**
     * Utility type to bundle passing offsets around. This is used often as leaf blocks' indices are relative
     * to those blocks' position in the quad tree. To get the actual row, column index we must know what part
     * of the matrix each block represents.
     */
    struct offset_t {
        index_t row_offset = 0;
        index_t col_offset = 0;

        bool operator==(const offset_t& rhs) const {
            return row_offset == rhs.row_offset && col_offset == rhs.col_offset;
        }

        offset_t operator+(const offset_t& rhs) const {
            return offset_t{
                    .row_offset = row_offset + rhs.row_offset,
                    .col_offset = col_offset + rhs.col_offset,
            };
        }
    };

    /**
     * Utility structure for getting block statistics
     */
    struct block_size_info {
        size_t index_bytes = 0;
        size_t value_bytes = 0;
        size_t overhead_bytes = 0;
        size_t nnn = 0;

        [[nodiscard]] size_t total_bytes() const {
            return index_bytes + value_bytes + overhead_bytes;
        }

        block_size_info operator+(const block_size_info& rhs) const {
            return block_size_info {
                    index_bytes + rhs.index_bytes,
                    value_bytes + rhs.value_bytes,
                    overhead_bytes + rhs.overhead_bytes,
                    nnn + rhs.nnn};
        }
    };

    /**
     * Standard mathematical plus-times semiring. The additive and multiplicative identities are never needed.
     *
     * @tparam T numerical type, such as double
     */
    template <typename T>
    struct plus_times_semiring {
        typedef T map_type_l;
        typedef T map_type_r;
        typedef T reduce_type;

        reduce_type multiply(const map_type_l& lhs, const map_type_r& rhs) const {
            return lhs * rhs;
        }

        reduce_type add(const reduce_type& lhs, const reduce_type& rhs) const {
            return lhs + rhs;
        }
    };

    /**
     * A simple implementation of a class that consumes errors by immediately throwing them.
     *
     * @tparam EX type of exception to throw. Must have a constructor that accepts a std::string
     */
    template <typename EX = std::invalid_argument>
    struct throwing_error_consumer {
        explicit throwing_error_consumer(std::string prefix = std::string()) : prefix(std::move(prefix)) {}

        void set_prefix(const std::string &new_prefix) {
            prefix = new_prefix;
        }

        void error(const std::string& e) {
            throw EX(prefix + e);
        }

        template <typename... ARGS>
        void error(const char* arg, ARGS... args) {
            error(std::string(arg), args...);
        }

        template <typename... ARGS>
        void error(const std::string& arg1, const std::string& arg2, ARGS... args) {
            error(arg1 + arg2, args...);
        }

        template <typename... ARGS>
        void error(const std::string& arg1, const char* arg2, ARGS... args) {
            error(arg1 + arg2, args...);
        }

        template <typename ARG, typename... ARGS>
        void error(const std::string& arg, const ARG& arg2, ARGS... args) {
            error(arg + std::to_string(arg2), args...);
        }

        template <typename ARG, typename... ARGS>
        void error(const ARG& arg, ARGS... args) {
            error(std::to_string(arg), args...);
        }

        template <typename... ARGS>
        void warning(ARGS... args) {
            error(args...);
        }

        std::string prefix;
    };

    /**
     * A simple error consumer that ignores everything.
     */
    struct ignoring_error_consumer {
        explicit ignoring_error_consumer(const std::string& = std::string()) {}

        void set_prefix(const std::string &) {
        }

        template <typename... ARGS>
        void error(ARGS... args) {
        }

        template <typename... ARGS>
        void warning(ARGS... args) {
            error(args...);
        }
    };

    /**
     * Temporary exception that is thrown when control reaches areas that are not yet implemented.
     *
     * All instances of this will be removed in the future.
     */
    class not_implemented : public std::invalid_argument {
    public:
        explicit not_implemented(const std::string &s) : invalid_argument(std::string("not implemented: ") + s) {}
    };

    /**
     * Thrown when incompatible nodes are pair-computed. For example, two leaf blocks with different sized indices.
     */
    class node_type_mismatch : public std::invalid_argument {
    public:
        node_type_mismatch() : invalid_argument("node mismatch") {}
        explicit node_type_mismatch(const std::string &s): invalid_argument("node mismatch: " + s) {}
    };
}

#endif //QUADMAT_TYPES_H
