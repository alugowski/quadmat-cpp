// Copyright (C) 2019-2020 Adam Lugowski
// All Rights Reserved.

#ifndef QUADMAT_TYPES_H
#define QUADMAT_TYPES_H

#include <numeric>
#include <utility>
#include <string>
#include <exception>

namespace quadmat {

    typedef int32_t BlockNnn;
    typedef int64_t Index;

    /**
     * Utility type to be able to return iterator ranges usable in a foreach.
     * @tparam ITER
     */
    template <typename ITER>
    struct Range {
        Range(ITER begin, ITER end) : _begin(begin), _end(end) {}

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
    struct Shape {
        Index nrows = 0;
        Index ncols = 0;

        bool operator==(const Shape& rhs) const {
            return nrows == rhs.nrows && ncols == rhs.ncols;
        }

        Shape& operator=(const Shape& rhs) = default;

        [[nodiscard]] std::string ToString() const {
            return std::string("{") + std::to_string(nrows) + ", " + std::to_string(ncols) + "}";
        }
    };

    /**
     * Utility type to bundle passing offsets around. This is used often as leaf blocks' indices are relative
     * to those blocks' position in the quad tree. To get the actual row, column index we must know what part
     * of the matrix each block represents.
     */
    struct Offset {
        Index row_offset = 0;
        Index col_offset = 0;

        bool operator==(const Offset& rhs) const {
            return row_offset == rhs.row_offset && col_offset == rhs.col_offset;
        }

        Offset operator+(const Offset& rhs) const {
            return Offset{
                    .row_offset = row_offset + rhs.row_offset,
                    .col_offset = col_offset + rhs.col_offset,
            };
        }
    };

    /**
     * Utility structure for getting block statistics
     */
    struct BlockSizeInfo {
        size_t index_bytes = 0;
        size_t value_bytes = 0;
        size_t overhead_bytes = 0;
        size_t nnn = 0;

        [[nodiscard]] size_t GetTotalBytes() const {
            return index_bytes + value_bytes + overhead_bytes;
        }

        BlockSizeInfo operator+(const BlockSizeInfo& rhs) const {
            return BlockSizeInfo {
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
    struct PlusTimesSemiring {
        using MapTypeA = T;
        using MapTypeB = T;
        using ReduceType = T;

        ReduceType Multiply(const MapTypeA& lhs, const MapTypeB& rhs) const {
            return lhs * rhs;
        }

        ReduceType Add(const ReduceType& lhs, const ReduceType& rhs) const {
            return lhs + rhs;
        }
    };

    /**
     * Utility to construct strings out of a list of arguments
     */
    struct Join {
        static std::string ToString(const std::string& e) {
            return e;
        }

        template <typename... Args>
        static std::string ToString(const char* arg, Args... args) {
            return ToString(std::string(arg), args...);
        }

        template <typename... Args>
        static std::string ToString(const std::string& arg1, const std::string& arg2, Args... args) {
            return ToString(arg1 + arg2, args...);
        }

        template <typename... Args>
        static std::string ToString(const std::string& arg1, const char* arg2, Args... args) {
            return ToString(arg1 + arg2, args...);
        }

        template <typename ARG, typename... Args>
        static std::string ToString(const std::string& arg, const ARG& arg2, Args... args) {
            return ToString(arg + std::to_string(arg2), args...);
        }

        template <typename ARG, typename... Args>
        static std::string ToString(const ARG& arg, Args... args) {
            return ToString(std::to_string(arg), args...);
        }
    };

    /**
     * A simple implementation of a class that consumes errors by immediately throwing them.
     *
     * @tparam EX type of exception to throw. Must have a constructor that accepts a std::string
     */
    template <typename EX = std::invalid_argument>
    struct ThrowingErrorConsumer {
        explicit ThrowingErrorConsumer(std::string prefix = std::string()) : prefix(std::move(prefix)) {}

        void SetPrefix(const std::string &new_prefix) {
            prefix = new_prefix;
        }

        template <typename... Args>
        void Error(Args... args) {
            throw EX(prefix + Join::ToString(args...));
        }

        template <typename... Args>
        void Warning(Args... args) {
            Error(args...);
        }

        std::string prefix;
    };

    /**
     * A simple error consumer that ignores everything.
     */
    struct IgnoringErrorConsumer {
        explicit IgnoringErrorConsumer(const std::string& = std::string()) {}

        void SetPrefix(const std::string &) {
        }

        template <typename... Args>
        void Error(Args... args) {
        }

        template <typename... Args>
        void Warning(Args... args) {
            Error(args...);
        }
    };

    /**
     * Temporary exception that is thrown when control reaches areas that are not yet implemented.
     *
     * All instances of this will be removed in the future.
     */
    class NotImplemented : public std::invalid_argument {
    public:
        explicit NotImplemented(const std::string &s) : invalid_argument(std::string("not implemented: ") + s) {}
    };

    /**
     * Thrown when incompatible nodes are pair-computed. For example, two leaf blocks with different sized indices.
     */
    class NodeTypeMismatch : public std::invalid_argument {
    public:
        NodeTypeMismatch() : invalid_argument("node mismatch") {}
        explicit NodeTypeMismatch(const std::string &s): invalid_argument("node mismatch: " + s) {}
    };
}

#endif //QUADMAT_TYPES_H
