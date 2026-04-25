// succinct_bitvector.hpp
// Pedagogical implementation for the post "Succinct Bit Vectors and Rank/Select"
// in the "Algebra over Wire Formats" series.
//
// Production version: PFC include/pfc/succinct.hpp (SuccinctBitVector class with
// BlockRankSupport, O(1) rank, O(log n) select).
// https://github.com/queelius/pfc

#pragma once

#include <bit>       // std::popcount (C++20)
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace succinct_bv {

// ---- popcount_word ---------------------------------------------------------
//
// Returns the number of set bits in a single 64-bit word.
// Uses std::popcount (C++20), which compiles to a single hardware instruction
// (POPCNT) on x86-64 and equivalent on ARM.

[[nodiscard]] inline std::size_t popcount_word(std::uint64_t w) noexcept {
    return static_cast<std::size_t>(std::popcount(w));
}

// ---- SuccinctBitVector --------------------------------------------------
//
// Stores a bit vector in packed uint64_t words. The auxiliary rank/select
// index is built lazily in build_index() (called by the constructor).
//
// Word layout: bit i lives in bits_[i/64] at position i%64 (LSB-first).
// Unused bits in the last word are kept zero.

class SuccinctBitVector {
public:
    // Construct from std::vector<bool>. Builds the auxiliary index immediately.
    explicit SuccinctBitVector(const std::vector<bool>& bits)
        : n_(bits.size())
        , bits_((bits.size() + 63) / 64, uint64_t{0})
    {
        for (std::size_t i = 0; i < bits.size(); ++i) {
            if (bits[i]) {
                bits_[i / 64] |= (uint64_t{1} << (i % 64));
            }
        }
        // Index built in Tasks 6-7; constructor is complete for Tasks 3-5.
    }

    // Logical size in bits.
    [[nodiscard]] std::size_t size() const noexcept { return n_; }

    // Access bit at position i (0-indexed). No bounds checking.
    [[nodiscard]] bool bit(std::size_t i) const noexcept {
        return (bits_[i / 64] >> (i % 64)) & uint64_t{1};
    }

    // rank1(i): count of 1-bits in positions [0, i).
    // This O(n/64) version scans word-by-word; it is correct but not constant-time.
    // Replaced by the indexed version in Task 7.
    [[nodiscard]] std::size_t rank1(std::size_t i) const noexcept {
        if (i == 0) return 0;
        std::size_t word_idx    = i / BLOCK_BITS;   // Full words before i.
        std::size_t within_word = i % BLOCK_BITS;   // Remaining bits.
        std::size_t count = 0;
        for (std::size_t w = 0; w < word_idx; ++w) {
            count += popcount_word(bits_[w]);
        }
        if (within_word > 0) {
            // Mask off bits at position within_word and beyond (keep bits 0..within_word-1).
            uint64_t mask = (uint64_t{1} << within_word) - uint64_t{1};
            count += popcount_word(bits_[word_idx] & mask);
        }
        return count;
    }

protected:
    std::size_t n_;                    // Logical bit count.
    std::vector<uint64_t> bits_;       // Packed bit array, LSB-first.
    std::vector<uint64_t> superblock_ranks_;  // Cumulative rank at superblock boundaries.
    std::vector<uint16_t> block_ranks_;       // Block-relative rank (within superblock).

    static constexpr std::size_t SUPERBLOCK_BITS = 4096;  // 64 words per superblock.
    static constexpr std::size_t BLOCK_BITS      = 64;    // One uint64_t word per block.
    static constexpr std::size_t BLOCKS_PER_SB   = SUPERBLOCK_BITS / BLOCK_BITS;  // 64.
};

}  // namespace succinct_bv
