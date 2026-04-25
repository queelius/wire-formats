// roaring_bitmap.hpp
// Pedagogical implementation for the post "RoaringBitmap" in the
// "Algebra over Wire Formats" series.
//
// Production version: PFC include/pfc/succinct.hpp (RoaringBitmap class with
// three container types: array, bitmap, run).
// https://github.com/queelius/pfc

#pragma once

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <map>
#include <utility>
#include <variant>
#include <vector>

namespace roaring {

// ---- ArrayContainer ---------------------------------------------------------
//
// Stores at most 4096 distinct uint16_t values as a sorted array.
// Contains check: binary search, O(log cardinality).
// Add: sorted insertion, O(cardinality) worst case due to shifting.
//
// Space: 2 * cardinality bytes (2 bytes per uint16_t element).
// At 4096 elements: 8 KB. A BitmapContainer costs 8 KB for 65536 bits.
// The threshold ARRAY_MAX = 4096 is chosen so that array and bitmap cost
// the same number of bytes at the crossover point.

static constexpr std::size_t ARRAY_MAX = 4096;

class ArrayContainer {
    std::vector<uint16_t> elems_;   // Sorted, unique.

public:
    ArrayContainer() = default;

    // Add value v to the container. No-op if already present.
    void add(uint16_t v) {
        auto it = std::lower_bound(elems_.begin(), elems_.end(), v);
        if (it != elems_.end() && *it == v) return;  // Already present.
        elems_.insert(it, v);
    }

    // Returns true if v is in the container.
    [[nodiscard]] bool contains(uint16_t v) const noexcept {
        auto it = std::lower_bound(elems_.begin(), elems_.end(), v);
        return it != elems_.end() && *it == v;
    }

    // Number of distinct values.
    [[nodiscard]] std::size_t cardinality() const noexcept {
        return elems_.size();
    }

    // Raw access for conversion helpers.
    [[nodiscard]] const std::vector<uint16_t>& elements() const noexcept {
        return elems_;
    }
};

// ---- BitmapContainer --------------------------------------------------------
//
// Dense bit vector covering the full 16-bit value space (65536 values).
// Stored as 1024 uint64_t words (65536 / 64 = 1024), occupying exactly 8 KB.
//
// Contains check: O(1) word lookup + bit test.
// Add: O(1) word lookup + bit set.
// Cardinality: O(1024) = O(1) amortized, scanning all words with popcount.
//
// Used when a chunk has more than ARRAY_MAX = 4096 elements.

class BitmapContainer {
    static constexpr std::size_t NUM_WORDS = 65536 / 64;  // 1024 words = 8 KB.
    std::vector<uint64_t> words_;

public:
    BitmapContainer() : words_(NUM_WORDS, uint64_t{0}) {}

    void add(uint16_t v) noexcept {
        words_[v / 64] |= (uint64_t{1} << (v % 64));
    }

    [[nodiscard]] bool contains(uint16_t v) const noexcept {
        return (words_[v / 64] >> (v % 64)) & uint64_t{1};
    }

    [[nodiscard]] std::size_t cardinality() const noexcept {
        std::size_t count = 0;
        for (auto w : words_) count += static_cast<std::size_t>(__builtin_popcountll(w));
        return count;
    }

    [[nodiscard]] const std::vector<uint64_t>& raw_words() const noexcept {
        return words_;
    }
};

}  // namespace roaring
