// Copyright (C) 2019-2020 Adam Lugowski
// All Rights Reserved.

#ifndef QUADMAT_INCLUDE_QUADMAT_UTIL_STREAM_CHUNKER_H_
#define QUADMAT_INCLUDE_QUADMAT_UTIL_STREAM_CHUNKER_H_

#include <mutex>
#include <vector>

namespace quadmat {

/**
 * A utility class that returns relatively large chunks from a stream.
 *
 * @tparam ByteStream input stream type
 */
template <typename CharAllocator, typename ByteStream=std::istream>
class StreamChunker {
public:
    using Chunk = std::vector<char, CharAllocator>;

    StreamChunker(ByteStream &instream, std::size_t target_chunk_size,
                    const char delimiter = '\n', const std::size_t leftover_read_size = 512) :
        instream_(instream),
        target_chunk_size_(target_chunk_size),
        leftovers_buffer_(leftover_read_size),
        delimiter_(delimiter),
        leftover_read_size_(leftover_read_size) {
        leftovers_begin_ = leftovers_end_ = std::cend(leftovers_buffer_);
    }

    Chunk NextChunk() {
        std::lock_guard lock(instream_mutex_);

        std::vector<char, CharAllocator> chunk;

        // copy over any leftovers
        std::copy(leftovers_begin_, leftovers_end_, std::back_inserter(chunk));
        leftovers_begin_ = leftovers_end_;

        // check if any bytes left in the stream
        if (!instream_) {
            return chunk;
        }

        // copy from stream
        if (chunk.size() < target_chunk_size_) {
            std::size_t write_start_index = chunk.size();
            chunk.resize(target_chunk_size_);

            std::size_t requested_bytes = target_chunk_size_ - write_start_index;
            instream_.read(&chunk[write_start_index], requested_bytes);
            std::size_t read_bytes = instream_.gcount();

            chunk.resize(write_start_index + read_bytes);
        }

        // see if chunk happens to end on a delimiter
        if (!chunk.empty() && chunk.back() == delimiter_) {
            return chunk;
        }

        // read ahead and find delimiter
        do {
            if (!instream_) {
                // EOF
                return chunk;
            }

            instream_.read(&leftovers_buffer_[0], leftover_read_size_);
            leftovers_begin_ = std::cbegin(leftovers_buffer_);
            leftovers_end_ = leftovers_begin_ + instream_.gcount();

            while (leftovers_begin_ != leftovers_end_) {
                const char c = *leftovers_begin_++;
                chunk.emplace_back(c);

                if (c == delimiter_) {
                    return chunk;
                }
            }
        } while (leftovers_begin_ == leftovers_end_);

        return chunk;
    }

    /**
     * Input Iterator type emits chunks.
     */
    class Iterator {
    public:
        using iterator_category = std::input_iterator_tag;
        using value_type = Chunk;
        using pointer = value_type*;
        using reference = value_type&;
        using difference_type = std::ptrdiff_t;

        Iterator(StreamChunker<CharAllocator, ByteStream> *chunker, bool at_end) : chunker_(chunker), at_end_(at_end) {}

        /**
         * Don't call this unless at the ends of iteration. Horribly inefficient otherwise.
         */
        Iterator(const Iterator& rhs)
            : chunker_(rhs.chunker_), chunk_(rhs.chunk_), at_end_(rhs.at_end_) {}

        /**
         * Copy constructor that moves rhs' chunk.
         */
        Iterator(const Iterator& rhs, bool)
            : chunker_(rhs.chunker_), chunk_(std::move(rhs.chunk_)), at_end_(rhs.at_end_) {}

        reference operator*() {
            return chunk_;
        }

        Iterator& operator++() {
            chunk_ = chunker_->NextChunk();
            at_end_ = chunk_.empty();
            return *this;
        }

        Iterator operator++(int) {  // NOLINT(cert-dcl21-cpp)
            Iterator temp(*this, true);
            operator++();
            return temp;
        }

        bool operator==(const Iterator& rhs) const {
            return at_end_ == rhs.at_end_;
        }

        bool operator!=(const Iterator& rhs) const {
            return at_end_ != rhs.at_end_;
        }

    private:
        StreamChunker<CharAllocator, ByteStream>* chunker_;
        Chunk chunk_{};
        bool at_end_;
    };

    Iterator begin() {
        return Iterator(this, false);
    }

    Iterator end() {
        return Iterator(this, true);
    }

private:

    using Position = typename std::vector<char, CharAllocator>::const_iterator;

    /**
     * Stream we're reading from
     */
    ByteStream& instream_;

    /**
     * Mutex for stream accesses
     */
    std::mutex instream_mutex_;

    /**
     * Target chunk size. Actual size may vary to match delimiter boundaries.
     */
    std::size_t target_chunk_size_;

    /**
     * Chunk delimiter. A chunk will end with one of these or EOF.
     */
    const char delimiter_;

    /**
     * Left overs
     */
    std::vector<char, CharAllocator> leftovers_buffer_{};

    Position leftovers_begin_;
    Position leftovers_end_;

    /**
     * When a chunk doesn't end in a delimiter, read this many bytes at a time to look for it.
     */
    const std::size_t leftover_read_size_;
};

}

#endif //QUADMAT_INCLUDE_QUADMAT_UTIL_STREAM_CHUNKER_H_
