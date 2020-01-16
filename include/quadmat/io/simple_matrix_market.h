// Copyright (C) 2019-2020 Adam Lugowski
// All Rights Reserved.

#ifndef QUADMAT_SIMPLE_MATRIX_MARKET_H
#define QUADMAT_SIMPLE_MATRIX_MARKET_H

#if defined(__cplusplus) && __cplusplus >= 201703L
#include <charconv>
#endif
#include <fstream>
#include <mutex>
#include <sstream>
#include <vector>

#include "quadmat/config.h"
#include "quadmat/matrix.h"
#include "quadmat/quadtree/tree_visitors.h"
#include "quadmat/util/base_iterators.h"
#include "quadmat/util/stream_chunker.h"
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
            load_successful_ = LoadImpl<Config>(instream, pattern_value);

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

        template <typename Config = DefaultConfig>
        bool LoadImpl(std::istream& instream, const T& pattern_value) {
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
            bool has_warnings = ReadTuples<Config>(header, instream, pattern_value);

            // sanity check length of what we read
            if (loaded_tuples_.size() != header.nnz) {
                ec_.Warning("File is truncated. Expected ", header.nnz, " nonzeros but loaded ", loaded_tuples_.size());
            }

            // if the file used a symmetry then duplicate tuples accordingly
            ExpandSymmetry(header);

            return !has_warnings;
        }

#if !defined(QUADMAT_PARSE_NO_FROMCHARS) && defined(__cplusplus) && __cplusplus >= 201703L
        /**
         * Parse a string into an int.
         *
         * This version uses C++17's from_chars functionality which is faster than strtoll() but less available.
         *
         * @param pos field start
         * @param end field end
         * @param val outparameter where parsed value is written
         * @param left_trim if true then any whitespace is first trimmed
         * @return pointer to the first character after this field, or nullptr if there was a parsing error
         */
        const char* ReadInt(const char* pos, const char* end, Index& val, bool left_trim) const noexcept {
            if (left_trim) {
                pos = pos + std::strspn(pos, " ");
            }

            std::from_chars_result result = std::from_chars(pos, end, val);
            if (result.ec != std::errc()) {
                return nullptr;
            }

            return result.ptr;
        }
#else
        /**
         * Parse a string into an int.
         *
         * This version uses strtoll which is widely available.
         *
         * @param pos field start
         * @param end field end
         * @param val outparameter where parsed value is written
         * @param left_trim if true then any whitespace is first trimmed
         * @return pointer to the first character after this field, or nullptr if there was a parsing error
         */
        const char* ReadInt(const char* pos, const char* end, Index& val, bool left_trim) const noexcept {
            char *ret;
            val = std::strtoll(pos, &ret, 10);

            if (ret == pos) {
                return nullptr;
            }
            return ret;
        }
#endif
        /**
         * Parse a string into a double.
         *
         * This version uses strtod which is widely available.
         *
         * A version of from_chars that parses doubles sill has limited compiler support and is not yet
         * implemented here.
         *
         * @param pos field start
         * @param end field end
         * @param val outparameter where parsed value is written
         * @param left_trim if true then any whitespace is first trimmed
         * @return pointer to the first character after this field, or nullptr if there was a parsing error
         */
        const char* ReadDouble(const char* pos, const char* end, T& val, bool left_trim) const noexcept {
            char *ret;
            val = std::strtod(pos, &ret);

            if (ret == pos) {
                return nullptr;
            }
            return ret;
        }

        /**
         * Find the next line.
         *
         * @param pos position
         * @param end iteration limit
         * @return pos advanced such that it points to one past the next newline, or end if no newline found
         */
        const char* AdvanceToNextLine(const char* pos, const char* end) {
            while (pos != end && *pos != '\n') {
                ++pos;
            }

            if (pos != end) {
                ++pos;
            }
            return pos;
        }

        /**
         * Read tuples. Attempts to take advantage of as many fast parsing practices as possible.
         * See the bench_parse benchmark.
         *
         * The input stream is chunked into relatively large blocks. Line and field separators are found efficiently,
         * fields are parsed using the fastest integer and double parsing methods available.
         *
         * @tparam Config
         * @param header already parsed header
         * @param instream stream to read from. This method is single pass.
         * @param pattern_value value to use if the file is a pattern file (i.e. lines do not specify a value)
         * @return true if any lines have a warning
         */
        template <typename Config = DefaultConfig>
        bool ReadTuples(const MatrixMarketHeader& header, std::istream& instream, const T& pattern_value) {
            bool has_warnings = false;

            size_t line_num = header.lines_read;

            // Chunk the input into 1MB blocks
            // This allows disk I/O to be fast because it reads a large chunk at a time.
            StreamChunker<typename Config::template TempAllocator<char>> chunker(instream, 1u << 20u, '\n');
            for (auto chunk : chunker) {

                const char* pos = chunk.data();
                const char* end = pos + chunk.size();

                while (pos != end && pos != nullptr) {
                    const char* line_start = pos;

                    Index row, col;

                    pos = ReadInt(pos, end, row, false);
                    if (pos == nullptr || row < 1 || row > shape_.nrows) {
                        ec_.Warning("line ", line_num, ": row index out of range");
                        has_warnings = true;
                        pos = AdvanceToNextLine(line_start, end);
                        continue;
                    }

                    pos = ReadInt(pos, end, col, true);
                    if (pos == nullptr || col < 1 || col > shape_.ncols) {
                        ec_.Warning("line ", line_num, ": column index out of range");
                        has_warnings = true;
                        pos = AdvanceToNextLine(line_start, end);
                        continue;
                    }

                    if (header.field == MatrixMarketHeader::pattern) {
                        loaded_tuples_.emplace_back(row - 1, col - 1, pattern_value);
                    } else {
                        T value;
                        pos = ReadDouble(pos, end, value, true);
                        if (pos == nullptr) {
                            ec_.Warning("line ", line_num, ": error parsing value");
                            has_warnings = true;
                            pos = AdvanceToNextLine(line_start, end);
                        }
                        loaded_tuples_.emplace_back(row - 1, col - 1, value);
                    }

                    // ignore trailing whitespace
                    pos = AdvanceToNextLine(pos, end);
                }
            }

            return has_warnings;
        }

        /**
         * If the loaded file has been specified as symmetric than duplicate tuples accordingly.
         */
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
