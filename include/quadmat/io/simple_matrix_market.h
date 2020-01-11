// Copyright (C) 2019-2020 Adam Lugowski
// All Rights Reserved.

#ifndef QUADMAT_SIMPLE_MATRIX_MARKET_H
#define QUADMAT_SIMPLE_MATRIX_MARKET_H

#include <fstream>
#include <mutex>
#include <sstream>
#include <vector>

#include "quadmat/config.h"
#include "quadmat/matrix.h"
#include "quadmat/quadtree/tree_visitors.h"
#include "quadmat/util/base_iterators.h"
#include "quadmat/util/types.h"

namespace quadmat {

    struct MatrixMarketHeader {
        // some format definitions
        const std::string MatrixMarketBanner = "%MatrixMarket";
        const std::string MatrixMarketBanner2 = "%%MatrixMarket";

        static bool IsComment(const std::string& line) {
            return !line.empty() && line[0] == '%';
        }

        template <typename ErrorConsumer>
        bool ReadHeader(std::istream& instream, ErrorConsumer& ec) {
            std::string line;
            std::string delimiter = " ";

            // read banner
            std::getline(instream, line);
            lines_read++;

            if (line.rfind(MatrixMarketBanner, 0) != 0
                && line.rfind(MatrixMarketBanner2, 0) != 0) {
                // not a matrix market file because the banner is missing
                ec.Error("Not a Matrix Market file. Missing banner.");
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
                    ec.Error("Unknown object type");
                    return false;
                }

                // parse format
                if (f_format == "array") {
                    format = array;
                } else if (f_format == "coordinate") {
                    format = coordinate;
                } else {
                    ec.Error("Unknown format type");
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
                    ec.Error("Unknown field type");
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
                    ec.Error("Unknown symmetry type");
                    return false;
                }
            }

            // read the dimension line, which may be preceded by comments
            do {
                std::getline(instream, line);
                lines_read++;

                if (!instream) {
                    ec.Error("Premature EOF");
                    return false;
                }
            } while (IsComment(line));

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

        template <typename ErrorConsumer>
        bool WriteHeader(std::ostream& os, ErrorConsumer& ec) {
            std::string delim = " ";

            // write the banner
            os << MatrixMarketBanner2 << delim;
            os << "matrix" << delim;
            os << "coordinate" << delim;
            os << "real" << delim;
            os << "general" << "\n";

            // no comment

            // write dimension line
            os << nrows << delim << ncols << delim << nnz << "\n";
            return true;
        }

        enum {matrix, vector} object = matrix;
        enum {array, coordinate} format = coordinate;
        enum {real, double_, complex, integer, pattern} field = real;
        enum {general, symmetric, skew_symmetric, hermitian} symmetry = general;

        int64_t nrows = 0;
        int64_t ncols = 0;
        int64_t nnz = 0;

        size_t lines_read = 0;
    };

    /**
     * A very simple matrix-market loader that stores everything in memory.
     *
     * Matrix Market format declared here: http://networkrepository.com/mtx-matrix-market-format.html
     */
    template <typename ErrorConsumer = ThrowingErrorConsumer<>, typename T = double, typename IT = int64_t>
    class SimpleMatrixMarketLoader {
    public:

        explicit SimpleMatrixMarketLoader(ErrorConsumer ec = ErrorConsumer()) : ec_(ec) {};

        /**
         * @return A begin,end pair of iterators over the tuples that have been loaded in the constructor
         */
        [[nodiscard]] auto tuples() const {
            return Range{loaded_tuples_.cbegin(), loaded_tuples_.cend()};
        }

        /**
         * Load from a stream object.
         *
         * @tparam ErrorConsumer
         * @param instream stream to load from
         * @param ec error consumer to use
         * @param pattern_value value to insert if the matrix market file uses a pattern field
         * @return loaded matrix
         */
        template <typename Config = DefaultConfig>
        Matrix<T, Config> Load(std::istream& instream, const T& pattern_value = 1) {
            load_successful_ = LoadImpl(instream, pattern_value);

            return MatrixFromTuples<T, Config>(shape_, loaded_tuples_.size(), loaded_tuples_);
        }

        /**
         * Load from a filename.
         *
         * @tparam Config
         * @param filename filename to load from
         * @param pattern_value value to insert if the matrix market file uses a pattern field
         * @return loaded matrix
         */
        template <typename Config = DefaultConfig>
        Matrix<T, Config> Load(const std::string& filename, const T& pattern_value = 1) {
            ec_.SetPrefix(filename + ": ");

            if (std::ifstream infile{filename}) {
                return Load(infile, pattern_value);
            } else {
                ec_.Error("Cannot open file");
                return Matrix<T, Config>{Shape{-1, -1}};
            }
        }

        /**
         * @return true if the constructor managed to load a file with no errors. Warnings ok.
         */
        [[nodiscard]] bool IsLoadSuccessful() const {
            return load_successful_;
        }

    protected:
        bool LoadImpl(std::istream& instream, const T& pattern_value) {
            bool has_warnings = false;

            // read the header
            MatrixMarketHeader header;
            if (!header.ReadHeader(instream, ec_)) {
                return false;
            }

            shape_ = {header.nrows, header.ncols};

            // make sure it's a format we support
            if (header.object != MatrixMarketHeader::matrix) {
                ec_.Error("Only matrix objects are supported");
                return false;
            }

            if (header.format != MatrixMarketHeader::coordinate) {
                ec_.Error("Only coordinate matrix market files supported at this time");
                return false;
            }

            if (header.field != MatrixMarketHeader::real &&
                    header.field != MatrixMarketHeader::double_ &&
                    header.field != MatrixMarketHeader::integer &&
                    header.field != MatrixMarketHeader::pattern) {
                ec_.Error("Only fields convertible to double supported at this time");
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

                Index row;
                Index col;

                iss >> row >> col;

                if (row < 1 || row > shape_.nrows) {
                    ec_.Warning("line ", line_num, ": row index out of range");
                    has_warnings = true;
                    continue;
                }
                if (col < 1 || col > shape_.ncols) {
                    ec_.Warning("line ", line_num, ": column index out of range");
                    has_warnings = true;
                    continue;
                }

                if (header.field == MatrixMarketHeader::pattern) {
                    loaded_tuples_.emplace_back(row - 1, col - 1, pattern_value);
                } else {
                    T value;
                    iss >> value;
                    loaded_tuples_.emplace_back(row - 1, col - 1, value);
                }
            }

            // sanity check length of what we read
            if (loaded_tuples_.size() != header.nnz) {
                ec_.Warning("File is truncated. Expected ", header.nnz, " nonzeros but loaded ", loaded_tuples_.size());
            }

            // if the file used a symmetry then duplicate tuples accordingly
            ExpandSymmetry(header);

            return !has_warnings;
        }

        void ExpandSymmetry(const MatrixMarketHeader& header) {
            switch (header.symmetry) {
                case MatrixMarketHeader::general:
                    // no symmetry
                    break;

                case MatrixMarketHeader::hermitian:
                    // Hermitian not supported due to no complex value support
                case MatrixMarketHeader::symmetric: {
                    // only the entries on or below the main diagonal are listed
                    // duplicate entries below the diagonal, transposing row/column

                    size_t end = loaded_tuples_.size();
                    for (size_t i = 0; i < end; i++) {
                        const auto &tup = loaded_tuples_[i];

                        if (std::get<1>(tup) != std::get<0>(tup)) {
                            loaded_tuples_.emplace_back(std::get<1>(tup), std::get<0>(tup), std::get<2>(tup));
                        }
                    }
                    break;
                }

                case MatrixMarketHeader::skew_symmetric: {
                    // only the entries strictly below the main diagonal are to be listed
                    // duplicate all entries, transposing row/column and negating values

                    size_t end = loaded_tuples_.size();
                    for (size_t i = 0; i < end; i++) {
                        const auto &tup = loaded_tuples_[i];
                        loaded_tuples_.emplace_back(std::get<1>(tup), std::get<0>(tup), -std::get<2>(tup));
                    }
                    break;
                }
            }
        }

        /**
         * In-memory storage of loaded tuples
         */
        std::vector<std::tuple<IT, IT, T>> loaded_tuples_;

        Shape shape_;

        bool load_successful_ = false;

        ErrorConsumer ec_;
    };

    /**
     * Users should use this alias
     */
    template <typename T = double, typename IT = int64_t>
    using MatrixMarketLoader = SimpleMatrixMarketLoader<T, IT>;

    /**
     * Matrix Market I/O
     */
    class MatrixMarket {
    public:

        /**
         * Load a Matrix Market file into a matrix.
         *
         * @tparam T
         * @tparam Config
         * @tparam ErrorConsumer
         * @param input
         * @param ec
         * @return a matrix
         */
        template <typename T = double, typename Config = DefaultConfig, typename ErrorConsumer = ThrowingErrorConsumer<>>
        static Matrix<T, Config> Load(std::istream& input, ErrorConsumer ec = ErrorConsumer(), const T& pattern_value = 1) {
            SimpleMatrixMarketLoader loader;
            return loader.Load<Config>(input);
        }

        /**
         * Save a matrix to a Matrix Market file.
         *
         * @tparam T
         * @tparam Config
         * @tparam ErrorConsumer
         * @param mat matrix to write
         * @param ec error consumer
         * @return `true` on success
         */
        template <typename T, typename Config, typename ErrorConsumer = ThrowingErrorConsumer<>>
        static bool Save(Matrix<T, Config> mat, std::ostream& output, ErrorConsumer ec = ErrorConsumer()) {
            // write the header
            MatrixMarketHeader header;
            header.object = MatrixMarketHeader::matrix;
            header.format = MatrixMarketHeader::coordinate;
            header.field = MatrixMarketHeader::real;
            header.symmetry = MatrixMarketHeader::general;

            header.nrows = mat.GetShape().nrows;
            header.ncols = mat.GetShape().ncols;
            header.nnz = mat.GetNnn();

            header.WriteHeader(output, ec);

            // write the tuples
            std::mutex output_mutex;
            std::visit(GetLeafVisitor<T, Config>([&](auto leaf, const Offset &offsets, Shape) {
              // format the string
              std::ostringstream oss;
              for (auto tup : leaf->Tuples()) {
                  oss << 1 + offsets.row_offset + std::get<0>(tup) << " "
                      << 1 + offsets.col_offset + std::get<1>(tup) << " "
                      << std::get<2>(tup) << "\n";
              }

              // dump the formatted chunk
              {
                  std::lock_guard lock(output_mutex);
                  output << oss.str();
              }
            }), mat.GetRootBC()->GetChild(0));

            return true;
        }
    };
}

#endif //QUADMAT_SIMPLE_MATRIX_MARKET_H
