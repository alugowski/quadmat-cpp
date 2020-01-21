// Copyright (C) 2019-2020 Adam Lugowski
// All Rights Reserved.

#include <benchmark/benchmark.h>

#include <array>
#include <cstdio>
#include <cstdlib>
#include <cerrno>
#include <sstream>

#if defined(__cplusplus) && __cplusplus >= 201703L
#include <charconv>
#endif

#include "quadmat/util/stream_chunker.h"

#pragma clang diagnostic push
#pragma ide diagnostic ignored "cert-err58-cpp"

static const std::array<std::string, 3> kLines = {
    "123456 234567 333.323",
    "1 234567 333.323",
    "1 2 3",
};

static const std::array<std::string, 2> kIntStrings = {
    "123456",
    "1",
};

static const std::array<std::string, 3> kDoubleStrings = {
    "123456",
    "1",
    "333.323",
};

/**
 * Constructs a large string block composed of repeated lines from kLines.
 *
 * @param byte_target size in bytes of the result
 */
static std::string ConstructManyLines(std::size_t byte_target) {
    std::vector<char> chunk;
    for (const auto& line : kLines) {
        std::copy(std::begin(line), std::end(line), std::back_inserter(chunk));
        chunk.emplace_back('\n');
    }

    std::vector<char> result;
    result.reserve(byte_target + chunk.size() + 1);

    while (result.size() < byte_target) {
        std::copy(std::begin(chunk), std::end(chunk), std::back_inserter(result));
    }
    result.emplace_back(0);

    return std::string(result.data());
}

/**
 * Large string with many lines.
 */
static const std::string kLineBlock = ConstructManyLines(50u << 20u);


/**
 * Find the roofline scanning speed.
 *
 * Add up every byte. Simulates touching every byte once.
 */
static void BM_ScanSpeed(benchmark::State& state) {
    std::size_t num_bytes = 0;

    for (auto _ : state) {
        auto sum = kLineBlock[0];
        for (const char& c : kLineBlock) {
            sum += c;
        }

        benchmark::DoNotOptimize(sum);
        num_bytes += kLineBlock.size();
    }

    state.SetBytesProcessed(num_bytes);
}

BENCHMARK(BM_ScanSpeed);

/**
 * Find the roofline scanning speed when reading input using StreamChunker.
 *
 * Add up every byte. Simulates touching every byte once.
 */
static void BM_ChunkedScanSpeed(benchmark::State& state) {
    std::size_t num_bytes = 0;

    for (auto _ : state) {
        std::istringstream iss{kLineBlock};
        quadmat::StreamChunker chunker(iss, 1u << 20u);

        auto sum = kLineBlock[0];
        for (const auto& chunk : chunker) {
            for (const char &c : chunk) {
                sum += c;
            }
        }

        benchmark::DoNotOptimize(sum);
        num_bytes += kLineBlock.size();
    }

    state.SetBytesProcessed(num_bytes);
}

BENCHMARK(BM_ChunkedScanSpeed);

/**
 * Find line breaks using strchr
 */
static void BM_SplitLines_strchr(benchmark::State& state) {
    std::size_t num_bytes = 0;

    for (auto _ : state) {
        const char* pos = kLineBlock.c_str();
        do {
            pos = std::strchr(pos + 1, '\n');
            benchmark::DoNotOptimize(pos);
        } while (pos != nullptr);

        num_bytes += kLineBlock.size();
    }

    state.SetBytesProcessed(num_bytes);
}

BENCHMARK(BM_SplitLines_strchr);

/**
 * Find line breaks using string::find_first_of (needle is single character variant)
 */
static void BM_SplitLines_find_first_of_char(benchmark::State& state) {
    std::size_t num_bytes = 0;

    for (auto _ : state) {
        size_t pos = 0;
        do {
            pos = kLineBlock.find_first_of('\n', pos + 1);
            benchmark::DoNotOptimize(pos);
        } while (pos != std::string::npos);

        num_bytes += kLineBlock.size();
    }

    state.SetBytesProcessed(num_bytes);
}

BENCHMARK(BM_SplitLines_find_first_of_char);

/**
 * Find line breaks using string::find_first_of (needle is string variant)
 */
static void BM_SplitLines_find_first_of_str(benchmark::State& state) {
    std::size_t num_bytes = 0;

    for (auto _ : state) {
        size_t pos = 0;
        do {
            pos = kLineBlock.find_first_of("\r\n", pos + 1);
            benchmark::DoNotOptimize(pos);
        } while (pos != std::string::npos);

        num_bytes += kLineBlock.size();
    }

    state.SetBytesProcessed(num_bytes);
}

BENCHMARK(BM_SplitLines_find_first_of_str);

/**
 * Find lines using std::getline()
 */
static void BM_SplitLines_getline(benchmark::State& state) {
    std::size_t num_bytes = 0;

    for (auto _ : state) {
        std::istringstream instream{kLineBlock};
        while (!instream.eof()) {
            std::string line;
            std::getline(instream, line);

            benchmark::DoNotOptimize(line);
        }

        num_bytes += kLineBlock.size();
    }

    state.SetBytesProcessed(num_bytes);
}

BENCHMARK(BM_SplitLines_getline);

/**
 * Tokenize line using strpbrk and strspn.
 */
static void BM_LineTokenize_strpbrk(benchmark::State& state, const char* sep) {
    std::size_t num_bytes = 0;
    std::size_t num_lines = 0;

    const char *row_start, *row_end;
    const char *col_start, *col_end;
    const char *value_start, *value_end;

    for (auto _ : state) {
        for (const auto& line : kLines) {
            row_start = line.c_str();
            row_end = std::strpbrk(row_start, sep);
            if (!row_end) {
                break; // error testing
            }

            col_start = row_end + std::strspn(row_end, sep); // skip separator
            col_end = std::strpbrk(col_start, sep);
            if (!col_end) {
                break; // error testing
            }

            value_start = col_end + std::strspn(col_end, sep); // skip separator
            value_end = std::strpbrk(value_start, sep);
            if (value_end) {
                // value_end is always NULL in this benchmark
                break; // error testing
            }

            benchmark::DoNotOptimize(row_end);
            benchmark::DoNotOptimize(col_end);
            benchmark::DoNotOptimize(value_end);

            num_bytes += line.size();
        }
        num_lines += kLines.size();
    }

    state.SetBytesProcessed(num_bytes);
    state.counters["lines_tokenized_per_second"] = benchmark::Counter(num_lines, benchmark::Counter::kIsRate);
}

BENCHMARK_CAPTURE(BM_LineTokenize_strpbrk, space_only, " ");
BENCHMARK_CAPTURE(BM_LineTokenize_strpbrk, space_tab, " \t");

/**
 * Tokenize line using strtok
 *
 * This benchmark serves to only provide a performance context because strtok is unusable:
 *  - strtok() is not thread safe.
 *  - strtok_r() is thread safe, but only available on POSIX systems. It is _not_ in the std namespace.
 *  - strtok_s() is standard C11, but Windows has an older incompatible version.
 */
static void BM_LineTokenize_strtok(benchmark::State& state, const char* sep) {
    std::size_t num_bytes = 0;
    std::size_t num_lines = 0;

    // reserve space for a copy of the line because strtok makes modifications.
    std::size_t max_length = 0;
    for (const auto& line : kLines) {
        max_length = std::max(max_length, line.size());
    }
    std::vector<char> line_buffer(max_length + 1);
    char* line_copy = &line_buffer[0];

    for (auto _ : state) {
        for (const auto& line : kLines) {
            // Copy the original string so that strtok can modify it
            // This copy is extra work that makes the method appear slightly slower but it's unavoidable.
            // This also highlights another drawback of strtok(): you can't use it on const arrays.
            std::strcpy(line_copy, line.c_str());

            char *row_s = std::strtok(line_copy, sep);
            if (!row_s) {
                break; // error testing
            }

            char *col_s = std::strtok(nullptr, sep);
            if (!col_s) {
                break; // error testing
            }

            char *val_s = std::strtok(nullptr, sep);
            if (!val_s) {
                break; // error testing
            }

            benchmark::DoNotOptimize(row_s);
            benchmark::DoNotOptimize(col_s);
            benchmark::DoNotOptimize(val_s);

            num_bytes += line.size();
        }

        num_lines += kLines.size();
    }

    state.SetBytesProcessed(num_bytes);
    state.counters["lines_tokenized_per_second"] = benchmark::Counter(num_lines, benchmark::Counter::kIsRate);
}

BENCHMARK_CAPTURE(BM_LineTokenize_strtok, space_only, " ");
BENCHMARK_CAPTURE(BM_LineTokenize_strtok, space_tab, " \t");

#if defined(__cplusplus) && __cplusplus >= 201703L
/**
 * Convert a single field from string to int.
 */
static void BM_IntFieldParse_from_chars(benchmark::State& state) {
    std::size_t num_bytes = 0;
    std::size_t num_fields = 0;

    for (auto _ : state) {
        for (const auto& field : kIntStrings) {
            int64_t value;
            std::from_chars_result result = std::from_chars(field.data(), field.data() + field.size(), value);
            if (result.ec != std::errc()) {
                break; // error testing
            }
            benchmark::DoNotOptimize(value);
            num_bytes += field.size();
        }
        num_fields += kIntStrings.size();
    }

    state.SetBytesProcessed(num_bytes);
    state.counters["fields_converted_per_second"] = benchmark::Counter(num_fields, benchmark::Counter::kIsRate);
}

BENCHMARK(BM_IntFieldParse_from_chars);
#endif

/**
 * Convert a single field from string to int using atol.
 *
 * atol() should be avoided because it does not report conversion errors. It is only included here for
 * a performance context.
 */
static void BM_IntFieldParse_atol(benchmark::State& state) {
    std::size_t num_bytes = 0;
    std::size_t num_fields = 0;

    for (auto _ : state) {
        for (const auto& field : kIntStrings) {
            long value = std::atol(field.c_str()); // NOLINT(cert-err34-c)
            benchmark::DoNotOptimize(value);
            num_bytes += field.size();
        }
        num_fields += kIntStrings.size();
    }

    state.SetBytesProcessed(num_bytes);
    state.counters["fields_converted_per_second"] = benchmark::Counter(num_fields, benchmark::Counter::kIsRate);
}

BENCHMARK(BM_IntFieldParse_atol);

/**
 * Convert a single field from string to int.
 */
static void BM_IntFieldParse_stoll(benchmark::State& state) {
    std::size_t num_bytes = 0;
    std::size_t num_fields = 0;

    errno = 0;
    for (auto _ : state) {
        for (const auto& field : kIntStrings) {
            int64_t value = std::stoll(field, nullptr, 10);
            if (errno != 0) {
                break; // error checking
            }
            benchmark::DoNotOptimize(value);
            num_bytes += field.size();
        }
        num_fields += kIntStrings.size();
    }

    state.SetBytesProcessed(num_bytes);
    state.counters["fields_converted_per_second"] = benchmark::Counter(num_fields, benchmark::Counter::kIsRate);
}

BENCHMARK(BM_IntFieldParse_stoll);

/**
 * Convert a single field from string to int.
 */
static void BM_IntFieldParse_strtoll(benchmark::State& state) {
    std::size_t num_bytes = 0;
    std::size_t num_fields = 0;

    errno = 0;
    for (auto _ : state) {
        for (const auto& field : kIntStrings) {
            int64_t value = std::strtoll(field.c_str(), nullptr, 10);
            if (errno != 0) {
                break; // error checking
            }
            benchmark::DoNotOptimize(value);
            num_bytes += field.size();
        }
        num_fields += kIntStrings.size();
    }

    state.SetBytesProcessed(num_bytes);
    state.counters["fields_converted_per_second"] = benchmark::Counter(num_fields, benchmark::Counter::kIsRate);
}

BENCHMARK(BM_IntFieldParse_strtoll);

/**
 * Convert a single field from string to int using sscanf.
 *
 * sscanf should be avoided because it does not report all conversion errors.
 */
static void BM_IntFieldParse_sscanf(benchmark::State& state) {
    std::size_t num_bytes = 0;
    std::size_t num_fields = 0;

    for (auto _ : state) {
        for (const auto& field : kIntStrings) {
            int64_t value;
            if (sscanf(field.c_str(), "%lld", &value) != 1) { // NOLINT(cert-err34-c)
                break; // error checking
            }
            benchmark::DoNotOptimize(value);
            num_bytes += field.size();
        }
        num_fields += kIntStrings.size();
    }

    state.SetBytesProcessed(num_bytes);
    state.counters["fields_converted_per_second"] = benchmark::Counter(num_fields, benchmark::Counter::kIsRate);
}

BENCHMARK(BM_IntFieldParse_sscanf);

/**
 * Convert a single field from string to int using istringstream.
 */
static void BM_IntFieldParse_istringstream(benchmark::State& state) {
    std::size_t num_bytes = 0;
    std::size_t num_fields = 0;

    for (auto _ : state) {
        for (const auto& field : kIntStrings) {
            int64_t value;

            std::istringstream iss(field);
            iss >> value;

            benchmark::DoNotOptimize(value);
            num_bytes += field.size();
        }
        num_fields += kIntStrings.size();
    }

    state.SetBytesProcessed(num_bytes);
    state.counters["fields_converted_per_second"] = benchmark::Counter(num_fields, benchmark::Counter::kIsRate);
}

BENCHMARK(BM_IntFieldParse_istringstream);

#if false && defined(__cplusplus) && __cplusplus >= 201703L
/**
 * Convert a single field from string to double.
 *
 * Compiler support for a double version of from_chars appears to be very spotty at this time.
 */
static void BM_DoubleFieldParse_from_chars(benchmark::State& state) {
    std::size_t num_bytes = 0;
    std::size_t num_fields = 0;

    for (auto _ : state) {
        for (const auto& field : kDoubleStrings) {
            double value = 0;
            std::from_chars_result result = std::from_chars(field.data(), field.data() + field.size(), value, std::chars_format::general);
            if (result.ec != std::errc()) {
                break; // error testing
            }
            benchmark::DoNotOptimize(value);
            num_bytes += field.size();
        }
        num_fields += kDoubleStrings.size();
    }

    state.SetBytesProcessed(num_bytes);
    state.counters["fields_converted_per_second"] = benchmark::Counter(num_fields, benchmark::Counter::kIsRate);
}

BENCHMARK(BM_DoubleFieldParse_from_chars);
#endif

/**
 * Convert a single field from string to double.
 */
static void BM_DoubleFieldParse_strtod(benchmark::State& state) {
    std::size_t num_bytes = 0;
    std::size_t num_fields = 0;

    errno = 0;
    for (auto _ : state) {
        for (const auto& field : kDoubleStrings) {
            double value = std::strtod(field.c_str(), nullptr);
            if (errno != 0) {
                break; // error checking
            }
            benchmark::DoNotOptimize(value);
            num_bytes += field.size();
        }
        num_fields += kDoubleStrings.size();
    }

    state.SetBytesProcessed(num_bytes);
    state.counters["fields_converted_per_second"] = benchmark::Counter(num_fields, benchmark::Counter::kIsRate);
}

BENCHMARK(BM_DoubleFieldParse_strtod);

/**
 * Convert a single field from string to double using sscanf.
 *
 * sscanf should be avoided because it does not report all conversion errors.
 */
static void BM_DoubleFieldParse_sscanf(benchmark::State& state) {
    std::size_t num_bytes = 0;
    std::size_t num_fields = 0;

    for (auto _ : state) {
        for (const auto& field : kDoubleStrings) {
            double value;
            if (sscanf(field.c_str(), "%lf", &value) != 1) { // NOLINT(cert-err34-c)
                break; // error checking
            }
            benchmark::DoNotOptimize(value);
            num_bytes += field.size();
        }
        num_fields += kDoubleStrings.size();
    }

    state.SetBytesProcessed(num_bytes);
    state.counters["fields_converted_per_second"] = benchmark::Counter(num_fields, benchmark::Counter::kIsRate);
}

BENCHMARK(BM_DoubleFieldParse_sscanf);

/**
 * Convert a single field from string to double using istringstream.
 */
static void BM_DoubleFieldParse_istringstream(benchmark::State& state) {
    std::size_t num_bytes = 0;
    std::size_t num_fields = 0;

    for (auto _ : state) {
        for (const auto& field : kDoubleStrings) {
            double value;

            std::istringstream iss(field);
            iss >> value;

            benchmark::DoNotOptimize(value);
            num_bytes += field.size();
        }
        num_fields += kDoubleStrings.size();
    }

    state.SetBytesProcessed(num_bytes);
    state.counters["fields_converted_per_second"] = benchmark::Counter(num_fields, benchmark::Counter::kIsRate);
}

BENCHMARK(BM_DoubleFieldParse_istringstream);

/**
 * Parse a line using istringstream
 */
static void BM_LineParse_istringstream(benchmark::State& state) {
    std::size_t num_bytes = 0;
    std::size_t num_lines = 0;

    int64_t row, col;
    double value;

    for (auto _ : state) {
        for (const auto& line : kLines) {
            std::istringstream iss(line);
            iss >> row >> col >> value;

            benchmark::DoNotOptimize(row);
            benchmark::DoNotOptimize(col);
            benchmark::DoNotOptimize(value);

            num_bytes += line.size();
        }
        num_lines += kLines.size();
    }

    state.SetBytesProcessed(num_bytes);
    state.counters["lines_parsed_per_second"] = benchmark::Counter(num_lines, benchmark::Counter::kIsRate);
}

BENCHMARK(BM_LineParse_istringstream);

/**
 * Parse a line using sscanf
 */
static void BM_LineParse_sscanf(benchmark::State& state) {
    std::size_t num_bytes = 0;
    std::size_t num_lines = 0;

    int64_t row, col;
    double value;

    for (auto _ : state) {
        for (const auto& line : kLines) {
            std::sscanf(line.c_str(), "%lld %lld %lf", &row, &col, &value); // NOLINT(cert-err34-c)

            benchmark::DoNotOptimize(row);
            benchmark::DoNotOptimize(col);
            benchmark::DoNotOptimize(value);

            num_bytes += line.size();
        }
        num_lines += kLines.size();
    }

    state.SetBytesProcessed(num_bytes);
    state.counters["lines_parsed_per_second"] = benchmark::Counter(num_lines, benchmark::Counter::kIsRate);
}

BENCHMARK(BM_LineParse_sscanf);

/**
 * Parse a line using strtoll and strtod.
 *
 * On error strtoll returns 0 or causes undefined behavior.
 */
static void BM_LineParse_strtoll(benchmark::State& state) {
    std::size_t num_bytes = 0;
    std::size_t num_lines = 0;

    int64_t row, col;
    double value;

    errno = 0;
    for (auto _ : state) {
        for (const auto& line : kLines) {
            char *end;

            row = std::strtoll(line.c_str(), &end, 10);
            col = std::strtoll(end, &end, 10);
            value = std::strtod(end, nullptr);

            if (errno != 0) {
                break; // error checking
            }

            benchmark::DoNotOptimize(row);
            benchmark::DoNotOptimize(col);
            benchmark::DoNotOptimize(value);

            num_bytes += line.size();
        }
        num_lines += kLines.size();
    }

    state.SetBytesProcessed(num_bytes);
    state.counters["lines_parsed_per_second"] = benchmark::Counter(num_lines, benchmark::Counter::kIsRate);
}

BENCHMARK(BM_LineParse_strtoll);

/**
 * Parse line using from_chars for ints and strtod for doubles.
 */
static void BM_LineParse_from_chars_strtod(benchmark::State& state, const char* sep) {
    std::size_t num_bytes = 0;
    std::size_t num_lines = 0;

    int64_t row, col;
    double value;

    errno = 0;
    for (auto _ : state) {
        for (const auto& line : kLines) {
            const char* line_end = line.data() + line.size();

            std::from_chars_result row_result = std::from_chars(line.data(), line_end, row);
            if (row_result.ec != std::errc()) {
                break; // error testing
            }

            const char* col_start = row_result.ptr + std::strspn(row_result.ptr, sep); // skip separator

            std::from_chars_result col_result = std::from_chars(col_start, line_end, col);
            if (col_result.ec != std::errc()) {
                break; // error testing
            }

            // strtod does its own leading whitespace skipping
            value = std::strtod(col_result.ptr, nullptr);
            if (errno != 0) {
                break; // error checking
            }

            benchmark::DoNotOptimize(row);
            benchmark::DoNotOptimize(col);
            benchmark::DoNotOptimize(value);

            num_bytes += line.size();
        }
        num_lines += kLines.size();
    }

    state.SetBytesProcessed(num_bytes);
    state.counters["lines_parsed_per_second"] = benchmark::Counter(num_lines, benchmark::Counter::kIsRate);
}

BENCHMARK_CAPTURE(BM_LineParse_from_chars_strtod, space_only, " ");
BENCHMARK_CAPTURE(BM_LineParse_from_chars_strtod, space_tab, " \t");

/**
 * Parse a block using istringstream
 *
 * The "default" C++ method.
 */
static void BM_BlockParse_istringstream(benchmark::State& state) {
    std::size_t num_bytes = 0;

    int64_t row, col;
    double value;

    for (auto _ : state) {
        std::istringstream iss(kLineBlock);

        while (!iss.eof()) {
            iss >> row >> col >> value;

            benchmark::DoNotOptimize(row);
            benchmark::DoNotOptimize(col);
            benchmark::DoNotOptimize(value);
        }

        num_bytes += kLineBlock.size();
    }

    state.SetBytesProcessed(num_bytes);
}

BENCHMARK(BM_BlockParse_istringstream);

/**
 * Parse a block using from_chars and strtod
 */
static void BM_BlockParse_from_chars_strtod(benchmark::State& state) {
    std::size_t num_bytes = 0;

    int64_t row, col;
    double value;

    for (auto _ : state) {
        errno = 0;

        const char* pos = kLineBlock.c_str();
        const char* end = pos + kLineBlock.size();

        while (pos != end && pos != nullptr) {

            std::from_chars_result row_result = std::from_chars(pos, end, row);
            if (row_result.ec != std::errc()) {
                break; // error testing
            }

            const char* col_start = row_result.ptr + std::strspn(row_result.ptr, " "); // skip separator

            std::from_chars_result col_result = std::from_chars(col_start, end, col);
            if (col_result.ec != std::errc()) {
                break; // error testing
            }

            // strtod does its own leading whitespace skipping
            char* value_end;
            value = std::strtod(col_result.ptr, &value_end);
            if (errno != 0) {
                break; // error checking
            }

            // find the newline
            pos = std::strchr(col_result.ptr, '\n');

            // bump to start of next line
            if (pos != end) {
                pos++;
            }

            benchmark::DoNotOptimize(row);
            benchmark::DoNotOptimize(col);
            benchmark::DoNotOptimize(value);
        }

        num_bytes += kLineBlock.size();
    }

    state.SetBytesProcessed(num_bytes);
}

BENCHMARK(BM_BlockParse_from_chars_strtod);

#pragma clag diagnostic pop

BENCHMARK_MAIN();
