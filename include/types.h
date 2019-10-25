// Copyright (C) 2019 Adam Lugowski
// All Rights Reserved.

#ifndef QUADMAT_TYPES_H
#define QUADMAT_TYPES_H

#include <numeric>
#include <utility>
#include <string>

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

        shape_t& operator=(const shape_t& rhs) {
            nrows = rhs.nrows;
            ncols = rhs.ncols;
            return *this;
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
        void error(const std::string& arg, ARGS... args) {
            error(arg, args...);
        }

        template <typename ARG, typename... ARGS>
        void error(const ARG arg, ARGS... args) {
            error(std::to_string(arg), args...);
        }

        template <typename... ARGS>
        void warning(ARGS... args) {
            error(args...);
        }

        std::string prefix;
    };
}

#endif //QUADMAT_TYPES_H
