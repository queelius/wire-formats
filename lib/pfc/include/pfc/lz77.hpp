// lz77.hpp - LZ77 compression algorithm for PFC library
#pragma once

#include "core.hpp"
#include "error_handling.hpp"
#include <vector>
#include <deque>
#include <cstring>
#include <algorithm>

namespace pfc {
namespace compression {

// ============================================================
// LZ77 Configuration
// ============================================================

struct LZ77Config {
    size_t window_size = 32768;    // Sliding window size (32KB default)
    size_t lookahead_size = 258;   // Lookahead buffer size
    size_t min_match_length = 3;   // Minimum match length to encode
    size_t max_match_length = 258; // Maximum match length

    static LZ77Config default_config() { return {}; }
    static LZ77Config fast() { return {4096, 64, 3, 64}; }
    static LZ77Config balanced() { return {16384, 128, 3, 128}; }
    static LZ77Config best() { return {32768, 258, 3, 258}; }
};

// ============================================================
// LZ77 Match Structure
// ============================================================

struct LZ77Match {
    uint16_t distance;  // Distance back in window (0 = literal)
    uint16_t length;    // Match length (0 for literals)
    uint8_t literal;    // Literal byte (when distance == 0)

    bool is_literal() const { return distance == 0; }

    // Encode the match to bits
    template<typename BitSink>
    result<void> encode(BitSink& sink) const {
        if (is_literal()) {
            // Encode as literal: 0 bit + 8 bits for byte
            sink.write(false);
            for (int i = 0; i < 8; ++i) {
                sink.write((literal >> i) & 1);
            }
        } else {
            // Encode as match: 1 bit + distance + length
            sink.write(true);
            // Encode distance (16 bits)
            for (int i = 0; i < 16; ++i) {
                sink.write((distance >> i) & 1);
            }
            // Encode length (8 bits)
            for (int i = 0; i < 8; ++i) {
                sink.write((length >> i) & 1);
            }
        }
        return {};
    }

    // Decode match from bits
    template<typename BitSource>
    static result<LZ77Match> decode(BitSource& source) {
        if (!source.peek()) {
            return make_error_code(pfc_error::eof_reached);
        }

        LZ77Match match{};
        bool is_match = source.read();

        if (!is_match) {
            // Decode literal
            match.distance = 0;
            match.literal = 0;
            for (int i = 0; i < 8; ++i) {
                if (!source.peek()) {
                    return make_error_code(pfc_error::incomplete_data);
                }
                match.literal |= source.read() << i;
            }
        } else {
            // Decode match
            match.distance = 0;
            for (int i = 0; i < 16; ++i) {
                if (!source.peek()) {
                    return make_error_code(pfc_error::incomplete_data);
                }
                match.distance |= source.read() << i;
            }

            match.length = 0;
            for (int i = 0; i < 8; ++i) {
                if (!source.peek()) {
                    return make_error_code(pfc_error::incomplete_data);
                }
                match.length |= source.read() << i;
            }
        }

        return match;
    }
};

// ============================================================
// LZ77 Compressor
// ============================================================

template<typename Allocator = std::allocator<uint8_t>>
class LZ77Compressor {
public:
    using allocator_type = Allocator;

    explicit LZ77Compressor(const LZ77Config& config = LZ77Config::default_config(),
                           const Allocator& alloc = Allocator())
        : config_(config), window_(alloc), lookahead_(alloc) {
        // Note: deque doesn't have reserve(), but that's okay
        // It will grow as needed
    }

    // Compress data
    template<typename InputIt, typename OutputIt>
    result<size_t> compress(InputIt first, InputIt last, OutputIt output) {
        size_t match_count = 0;

        // Fill lookahead buffer initially
        for (size_t i = 0; i < config_.lookahead_size && first != last; ++i, ++first) {
            lookahead_.push_back(*first);
        }

        while (!lookahead_.empty()) {
            auto match = find_best_match();

            if (match.is_literal() || match.length < config_.min_match_length) {
                // Output literal
                *output++ = LZ77Match{0, 0, lookahead_[0]};
                advance_window(1);
            } else {
                // Output match
                *output++ = match;
                advance_window(match.length);
            }

            match_count++;

            // Refill lookahead
            while (lookahead_.size() < config_.lookahead_size && first != last) {
                lookahead_.push_back(*first++);
            }
        }

        return match_count;
    }

    // Compress to bit stream
    template<typename InputIt, typename BitSink>
    result<size_t> compress_to_bits(InputIt first, InputIt last, BitSink& sink) {
        std::vector<LZ77Match> matches;
        auto res = compress(first, last, std::back_inserter(matches));
        if (!res) return res.error();

        // Write header: number of matches
        uint32_t num_matches = matches.size();
        for (int i = 0; i < 32; ++i) {
            sink.write((num_matches >> i) & 1);
        }

        // Write matches
        for (const auto& match : matches) {
            if (auto enc_res = match.encode(sink); !enc_res) {
                return enc_res.error();
            }
        }

        return matches.size();
    }

private:
    LZ77Config config_;
    std::deque<uint8_t, Allocator> window_;
    std::deque<uint8_t, Allocator> lookahead_;

    LZ77Match find_best_match() {
        if (lookahead_.empty()) {
            return {0, 0, 0};
        }

        LZ77Match best_match{0, 0, lookahead_[0]};

        // Search for matches in the window
        for (size_t i = 0; i < window_.size(); ++i) {
            size_t match_length = 0;

            // Check how long the match is
            while (match_length < lookahead_.size() &&
                   match_length < config_.max_match_length &&
                   i + match_length < window_.size() &&
                   window_[i + match_length] == lookahead_[match_length]) {
                match_length++;
            }

            // Update best match if this is better
            if (match_length >= config_.min_match_length && match_length > best_match.length) {
                best_match.distance = window_.size() - i;
                best_match.length = match_length;
            }
        }

        return best_match;
    }

    void advance_window(size_t count) {
        for (size_t i = 0; i < count && !lookahead_.empty(); ++i) {
            if (window_.size() >= config_.window_size) {
                window_.pop_front();
            }
            window_.push_back(lookahead_.front());
            lookahead_.pop_front();
        }
    }
};

// ============================================================
// LZ77 Decompressor
// ============================================================

template<typename Allocator = std::allocator<uint8_t>>
class LZ77Decompressor {
public:
    using allocator_type = Allocator;

    explicit LZ77Decompressor(const Allocator& alloc = Allocator())
        : output_(alloc) {
        output_.reserve(65536); // Reserve initial space
    }

    // Decompress matches
    template<typename InputIt, typename OutputIt>
    result<size_t> decompress(InputIt first, InputIt last, OutputIt output) {
        output_.clear();
        size_t output_size = 0;

        for (auto it = first; it != last; ++it) {
            const LZ77Match& match = *it;

            if (match.is_literal()) {
                // Output literal
                *output++ = match.literal;
                output_.push_back(match.literal);
                output_size++;
            } else {
                // Output match from history
                if (match.distance > output_.size()) {
                    return make_error_code(pfc_error::corrupted_data);
                }

                size_t start = output_.size() - match.distance;
                for (size_t i = 0; i < match.length; ++i) {
                    uint8_t byte = output_[start + i];
                    *output++ = byte;
                    output_.push_back(byte);
                    output_size++;
                }
            }
        }

        return output_size;
    }

    // Decompress from bit stream
    template<typename BitSource, typename OutputIt>
    result<size_t> decompress_from_bits(BitSource& source, OutputIt output) {
        // Read header: number of matches
        uint32_t num_matches = 0;
        for (int i = 0; i < 32; ++i) {
            if (!source.peek()) {
                return make_error_code(pfc_error::incomplete_data);
            }
            num_matches |= source.read() << i;
        }

        if (num_matches > 10000000) { // Sanity check
            return make_error_code(pfc_error::corrupted_data);
        }

        std::vector<LZ77Match> matches;
        matches.reserve(num_matches);

        // Read matches
        for (uint32_t i = 0; i < num_matches; ++i) {
            auto match_res = LZ77Match::decode(source);
            if (!match_res) {
                return match_res.error();
            }
            matches.push_back(*match_res);
        }

        return decompress(matches.begin(), matches.end(), output);
    }

private:
    std::vector<uint8_t, Allocator> output_;
};

// ============================================================
// LZSS (Lempel-Ziv-Storer-Szymanski) - LZ77 variant
// ============================================================

class LZSS {
public:
    // LZSS uses a more efficient encoding than plain LZ77
    // It uses a flag byte to indicate which of the next 8 items are matches vs literals

    template<typename InputIt, typename BitSink>
    static result<size_t> compress(InputIt first, InputIt last, BitSink& sink) {
        LZ77Compressor compressor(LZ77Config::balanced());
        std::vector<LZ77Match> matches;

        auto res = compressor.compress(first, last, std::back_inserter(matches));
        if (!res) return res.error();

        // Group matches into blocks of 8 with flag bytes
        size_t i = 0;
        while (i < matches.size()) {
            uint8_t flags = 0;
            size_t block_size = std::min(size_t(8), matches.size() - i);

            // Set flags for this block
            for (size_t j = 0; j < block_size; ++j) {
                if (!matches[i + j].is_literal()) {
                    flags |= (1 << j);
                }
            }

            // Write flag byte
            for (int b = 0; b < 8; ++b) {
                sink.write((flags >> b) & 1);
            }

            // Write block items
            for (size_t j = 0; j < block_size; ++j) {
                const auto& match = matches[i + j];
                if (match.is_literal()) {
                    // Write literal byte
                    for (int b = 0; b < 8; ++b) {
                        sink.write((match.literal >> b) & 1);
                    }
                } else {
                    // Write distance (12 bits) and length (4 bits) - more compact
                    for (int b = 0; b < 12; ++b) {
                        sink.write((match.distance >> b) & 1);
                    }
                    uint8_t encoded_length = std::min<uint8_t>(match.length - 3, 15);
                    for (int b = 0; b < 4; ++b) {
                        sink.write((encoded_length >> b) & 1);
                    }
                }
            }

            i += block_size;
        }

        return matches.size();
    }
};

} // namespace compression
} // namespace pfc