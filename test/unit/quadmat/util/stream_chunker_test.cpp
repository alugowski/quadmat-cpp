// Copyright (C) 2019-2020 Adam Lugowski
// All Rights Reserved.

#include "../../../test_dependencies/catch.hpp"

#include "quadmat/quadmat.h"

using namespace quadmat;

using Catch::Matchers::Equals;

static const std::string kSmallBlockString = // NOLINT(cert-err58-cpp)
    "123456 234567 333.323\n"
    "1 234567 333.323\n"
    "1 2 3\n"
    "123456 234567 333.323\n"
    "1 234567 333.323\n"
    "1 2 3\n"
    "123456 234567 333.323\n"
    "1 234567 333.323\n"
    "1 2 3\n"
    ;

TEST_CASE("Stream Chunking") {
    SECTION("reconstruction") {
        int chunk_size = GENERATE(range(1, (int)kSmallBlockString.size()));

        SECTION("chunk size " + std::to_string(chunk_size)) {
            std::istringstream iss{kSmallBlockString};

            StreamChunker<std::allocator<char>> chunker(iss, chunk_size, '\n', 3);
            std::vector<char> reconstruction;

            for (auto chunk : chunker) {
                std::copy(std::begin(chunk), std::end(chunk), std::back_inserter(reconstruction));
            }

            reconstruction.emplace_back(0); // null terminate to make a valid std::string
            REQUIRE_THAT(std::string(reconstruction.data()), Equals(kSmallBlockString));
        }
    }
}