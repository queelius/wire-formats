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
