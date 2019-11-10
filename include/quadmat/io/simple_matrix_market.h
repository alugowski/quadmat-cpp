// Copyright (C) 2019 Adam Lugowski
// All Rights Reserved.

#ifndef QUADMAT_SIMPLE_MATRIX_MARKET_H
#define QUADMAT_SIMPLE_MATRIX_MARKET_H

#include <fstream>
#include <sstream>
#include <vector>

#include "quadmat/base_iterators.h"
#include "quadmat/types.h"

namespace quadmat {

    struct matrix_market_header {
        // some format definitions
        const std::string MatrixMarketBanner = "%MatrixMarket";
        const std::string MatrixMarketBanner2 = "%%MatrixMarket";

        static bool is_comment(const std::string& line) {
            return !line.empty() && line[0] == '%';
        }

        template <typename EC>
        bool read_header(std::istream& instream, EC& ec) {
            std::string line;
            std::string delimiter = " ";

            // read banner
            std::getline(instream, line);
            lines_read++;

            if (line.rfind(MatrixMarketBanner, 0) != 0
                && line.rfind(MatrixMarketBanner2, 0) != 0) {
                // not a matrix market file because the banner is missing
                ec.error("Not a Matrix Market file. Missing banner.");
                return false;
            }

            // parse banner
            {
                std::istringstream iss(line);
                std::string banner, f_object, f_format, f_field, f_symmetry;
                iss >> banner >> f_object >> f_format >> f_field >> f_symmetry;

                // parse object
                if (f_object == "matrix") {
                    object = matrix;
                } else if (f_object == "vector") {
                    object = vector;
                } else {
                    ec.error("Unknown object type");
                    return false;
                }

                // parse format
                if (f_format == "array") {
                    format = array;
                } else if (f_format == "coordinate") {
                    format = coordinate;
                } else {
                    ec.error("Unknown format type");
                    return false;
                }

                // parse field
                if (f_field == "real") {
                    field = real;
                } else if (f_field == "double") {
                    field = double_;
                } else if (f_field == "complex") {
                    field = complex;
                } else if (f_field == "integer") {
                    field = integer;
                } else if (f_field == "pattern") {
                    field = pattern;
                } else {
                    ec.error("Unknown field type");
                    return false;
                }

                // parse symmetry
                if (f_symmetry == "general") {
                    symmetry = general;
                } else if (f_symmetry == "symmetric") {
                    symmetry = symmetric;
                } else if (f_symmetry == "skew-symmetric") {
                    symmetry = skew_symmetric;
                } else if (f_symmetry == "hermitian") {
                    symmetry = hermitian;
                } else {
                    ec.error("Unknown symmetry type");
                    return false;
                }
            }

            // read the dimension line, which may be preceeded by comments
            do {
                std::getline(instream, line);
                lines_read++;

                if (!instream) {
                    ec.error("Premature EOF");
                    return false;
                }
            } while (is_comment(line));

            // parse the dimension line
            {
                std::istringstream iss(line);

                iss >> nrows >> ncols;
                if (format == coordinate) {
                    iss >> nnz;
                }
            }

            return true;
        }

        enum {matrix, vector} object = matrix;
        enum {array, coordinate} format = coordinate;
        enum {real, double_, complex, integer, pattern} field = real;
        enum {general, symmetric, skew_symmetric, hermitian} symmetry = general;

        long long nrows = 0;
        long long ncols = 0;
        long long nnz = 0;

        size_t lines_read = 0;
    };

    /**
     * A very simple matrix-market loader that stores everything in memory.
     *
     * Matrix Market format declared here: http://networkrepository.com/mtx-matrix-market-format.html
     */
    template <typename T = double, typename IT = long long>
    class simple_matrix_market_loader {
    public:

        template <typename EC = throwing_error_consumer<>>
        explicit simple_matrix_market_loader(std::istream& instream, EC ec = EC(), const T& pattern_value = 1) {
            load_successful = load(instream, ec, pattern_value);
        }

        template <typename EC = throwing_error_consumer<>>
        explicit simple_matrix_market_loader(const std::string& filename, EC ec = EC(), const T& pattern_value = 1) {
            ec.set_prefix(filename + ": ");

            std::ifstream infile;
            infile.open(filename);

            if (!infile) {
                ec.error("Cannot open file");
            } else {
                load_successful = load(infile, ec, pattern_value);
            }

            infile.close();
        }

        /**
         * @return A begin,end pair of iterators over the tuples that have been loaded in the constructor
         */
        auto tuples() const {
            return range_t{loaded_tuples.cbegin(), loaded_tuples.cend()};
        }

        /**
         * @return number of rows and columns as declared in the file
         */
        [[nodiscard]] const shape_t &get_shape() const {
            return shape;
        }

        /**
         * @return true if the constructor managed to load a file with no errors. Warnings ok.
         */
        [[nodiscard]] bool is_load_successful() const {
            return load_successful;
        }

    protected:
        template <typename EC>
        bool load(std::istream& instream, EC& ec, const T& pattern_value) {

            // read the header
            matrix_market_header header;
            if (!header.read_header(instream, ec)) {
                return false;
            }

            shape = {header.nrows, header.ncols};

            // make sure it's a format we support
            if (header.object != matrix_market_header::matrix) {
                ec.error("Only matrix objects are supported");
                return false;
            }

            if (header.format != matrix_market_header::coordinate) {
                ec.error("Only coordinate matrix market files supported at this time");
                return false;
            }

            if (header.field != matrix_market_header::real &&
                    header.field != matrix_market_header::double_ &&
                    header.field != matrix_market_header::integer &&
                    header.field != matrix_market_header::pattern) {
                ec.error("Only fields convertible to double supported at this time");
                return false;
            }

            // read the tuples
            size_t line_num = header.lines_read;
            while (!instream.eof()) {
                std::string line;
                std::getline(instream, line);
                line_num++;

                if (line.empty()) {
                    break;
                }

                std::istringstream iss(line);

                index_t row;
                index_t col;

                iss >> row >> col;

                if (row < 1 || row > shape.nrows) {
                    ec.warning("line ", line_num, ": row index out of range");
                    continue;
                }
                if (col < 1 || col > shape.ncols) {
                    ec.warning("line ", line_num, ": column index out of range");
                    continue;
                }

                if (header.field == matrix_market_header::pattern) {
                    loaded_tuples.emplace_back(row - 1, col - 1, pattern_value);
                } else {
                    T value;
                    iss >> value;
                    loaded_tuples.emplace_back(row - 1, col - 1, value);
                }
            }

            // sanity check length of what we read
            if (loaded_tuples.size() != header.nnz) {
                ec.warning("File is truncated. Expected ", header.nnz, " nonzeros but loaded ", loaded_tuples.size());
            }

            // if the file used a symmetry then duplicate tuples accordingly
            expand_symmetry(header);

            return true;
        }

        void expand_symmetry(const matrix_market_header& header) {
            switch (header.symmetry) {
                case matrix_market_header::general:
                    // no symmetry
                    break;

                case matrix_market_header::symmetric:
                case matrix_market_header::hermitian: {
                    // only the entries on or below the main diagonal are listed
                    // duplicate entries below the diagonal
                    size_t end = loaded_tuples.size();
                    for (size_t i = 0; i < end; i++) {
                        const auto &tup = loaded_tuples[i];

                        if (std::get<1>(tup) != std::get<0>(tup)) {
                            loaded_tuples.emplace_back(std::get<1>(tup), std::get<0>(tup), std::get<2>(tup));
                        }
                    }
                    break;
                }

                case matrix_market_header::skew_symmetric: {
                    // only the entries strictly below the main diagonal are to be listed
                    // duplicate all entries

                    size_t end = loaded_tuples.size();
                    for (size_t i = 0; i < end; i++) {
                        const auto &tup = loaded_tuples[i];
                        loaded_tuples.emplace_back(std::get<1>(tup), std::get<0>(tup), std::get<2>(tup));
                    }
                    break;
                }
            }
        }

        /**
         * In-memory storage of loaded tuples
         */
        std::vector<std::tuple<IT, IT, T>> loaded_tuples;

        shape_t shape;

        bool load_successful = false;
    };

    /**
     * Users should use this alias
     */
    template <typename T = double, typename IT = long long>
    using matrix_market_loader = simple_matrix_market_loader<T, IT>;
}

#endif //QUADMAT_SIMPLE_MATRIX_MARKET_H
