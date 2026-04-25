#pragma once
// succinct.hpp - Succinct data structures with zero-copy semantics
// Wire format = in-memory format: The ultimate competitive advantage

#include "core.hpp"
#include "codecs.hpp"
#include "packed.hpp"
#include <vector>
#include <span>
#include <cmath>

namespace pfc::succinct {

// ============================================================
//  Succinct Bit Vector - Foundation for all succinct structures
//  Space: n + o(n) bits with O(1) rank/select operations
// ============================================================

// Forward declarations
class SuccinctBitVector;

// ============================================================
//  Rank/Select Support - The core operations
// ============================================================

// Concept for rank/select query structures
template<typename T>
concept RankSelectSupport = requires(const T& rs, size_t pos) {
    { rs.rank(pos) } -> std::convertible_to<size_t>;  // Count 1s up to pos
    { rs.select(pos) } -> std::convertible_to<size_t>; // Find pos-th 1
};

// ============================================================
//  Block-based Rank Support
//  Uses superblocks and blocks for O(1) rank queries
// ============================================================

class BlockRankSupport {
private:
    // Superblock size: every 2^16 bits (8KB)
    static constexpr size_t SUPERBLOCK_SIZE = 65536;
    static constexpr size_t SUPERBLOCK_SHIFT = 16;

    // Block size: every 512 bits (64 bytes, one cache line)
    static constexpr size_t BLOCK_SIZE = 512;
    static constexpr size_t BLOCK_SHIFT = 9;
    static constexpr size_t BLOCKS_PER_SUPERBLOCK = SUPERBLOCK_SIZE / BLOCK_SIZE;

    std::vector<uint32_t> superblocks_;  // Absolute rank at superblock boundaries
    std::vector<uint16_t> blocks_;       // Relative rank within superblock
    const uint64_t* bits_;               // Pointer to bit vector
    size_t num_bits_;

public:
    BlockRankSupport() = default;

    // Build from bit vector
    void build(const uint64_t* bits, size_t num_bits) {
        bits_ = bits;
        num_bits_ = num_bits;

        size_t num_superblocks = (num_bits + SUPERBLOCK_SIZE - 1) / SUPERBLOCK_SIZE;
        size_t num_blocks = (num_bits + BLOCK_SIZE - 1) / BLOCK_SIZE;

        superblocks_.resize(num_superblocks + 1);
        blocks_.resize(num_blocks + 1);

        size_t total_rank = 0;
        size_t superblock_rank = 0;

        for (size_t i = 0; i < num_blocks; ++i) {
            size_t bit_start = i * BLOCK_SIZE;
            size_t superblock_idx = bit_start / SUPERBLOCK_SIZE;

            // Start of new superblock
            if (bit_start % SUPERBLOCK_SIZE == 0) {
                superblocks_[superblock_idx] = total_rank;
                superblock_rank = 0;
            }

            // Store relative rank within superblock
            blocks_[i] = superblock_rank;

            // Count bits in this block
            size_t block_rank = 0;
            size_t word_start = bit_start / 64;
            size_t word_end = std::min((bit_start + BLOCK_SIZE) / 64, (num_bits + 63) / 64);

            for (size_t w = word_start; w < word_end; ++w) {
                block_rank += __builtin_popcountll(bits_[w]);
            }

            total_rank += block_rank;
            superblock_rank += block_rank;
        }

        superblocks_[num_superblocks] = total_rank;
    }

    // Count 1s in range [0, pos)
    [[nodiscard]] size_t rank(size_t pos) const {
        if (pos == 0) return 0;
        if (pos >= num_bits_) {
            // Return total rank when at or past end
            size_t num_superblocks = (num_bits_ + SUPERBLOCK_SIZE - 1) / SUPERBLOCK_SIZE;
            return superblocks_[num_superblocks];
        }

        size_t superblock_idx = pos / SUPERBLOCK_SIZE;
        size_t block_idx = pos / BLOCK_SIZE;
        size_t word_idx = pos / 64;
        size_t bit_offset = pos % 64;

        size_t result = superblocks_[superblock_idx];
        result += blocks_[block_idx];

        // Count remaining bits in partial word
        size_t block_start = (pos / BLOCK_SIZE) * BLOCK_SIZE;
        size_t word_start = block_start / 64;

        for (size_t w = word_start; w < word_idx; ++w) {
            result += __builtin_popcountll(bits_[w]);
        }

        if (bit_offset > 0) {
            uint64_t mask = (1ULL << bit_offset) - 1;
            result += __builtin_popcountll(bits_[word_idx] & mask);
        }

        return result;
    }

    // Find position of k-th 1 (0-indexed)
    [[nodiscard]] size_t select(size_t k) const {
        // Binary search on superblocks
        size_t left = 0, right = superblocks_.size() - 1;

        while (left < right) {
            size_t mid = left + (right - left) / 2;
            if (superblocks_[mid] <= k) {
                left = mid + 1;
            } else {
                right = mid;
            }
        }

        size_t superblock_idx = left > 0 ? left - 1 : 0;
        size_t remaining = k - superblocks_[superblock_idx];

        // Linear search within superblock (could be optimized)
        size_t block_start = superblock_idx * (SUPERBLOCK_SIZE / BLOCK_SIZE);
        size_t block_end = std::min(block_start + BLOCKS_PER_SUPERBLOCK, blocks_.size());

        for (size_t b = block_start; b < block_end; ++b) {
            size_t bit_start = b * BLOCK_SIZE;
            size_t word_start = bit_start / 64;
            size_t word_end = std::min((bit_start + BLOCK_SIZE) / 64, (num_bits_ + 63) / 64);

            for (size_t w = word_start; w < word_end; ++w) {
                size_t popcount = __builtin_popcountll(bits_[w]);
                if (remaining < popcount) {
                    // Found the word containing k-th 1
                    uint64_t word = bits_[w];
                    for (size_t i = 0; i < 64; ++i) {
                        if (word & (1ULL << i)) {
                            if (remaining == 0) {
                                return w * 64 + i;
                            }
                            --remaining;
                        }
                    }
                }
                remaining -= popcount;
            }
        }

        return num_bits_; // Not found
    }

    // Serialization
    template<typename Sink>
    void encode(Sink& sink) const requires BitSink<Sink> {
        // Encode structure metadata
        codecs::EliasGamma::encode(static_cast<uint32_t>(num_bits_), sink);
        codecs::EliasGamma::encode(static_cast<uint32_t>(superblocks_.size()), sink);

        for (auto sb : superblocks_) {
            codecs::EliasGamma::encode(sb, sink);
        }

        for (auto b : blocks_) {
            codecs::Fixed<16>::encode(b, sink);
        }
    }

    template<typename Source>
    static BlockRankSupport decode(Source& source, const uint64_t* bits)
        requires BitSource<Source>
    {
        BlockRankSupport rs;
        rs.bits_ = bits;
        rs.num_bits_ = codecs::EliasGamma::decode<uint32_t>(source);

        size_t num_superblocks = codecs::EliasGamma::decode<uint32_t>(source);
        rs.superblocks_.resize(num_superblocks);

        for (auto& sb : rs.superblocks_) {
            sb = codecs::EliasGamma::decode<uint32_t>(source);
        }

        size_t num_blocks = (rs.num_bits_ + BLOCK_SIZE - 1) / BLOCK_SIZE;
        rs.blocks_.resize(num_blocks);

        for (auto& b : rs.blocks_) {
            b = codecs::Fixed<16>::decode<uint16_t>(source);
        }

        return rs;
    }
};

// ============================================================
//  Succinct Bit Vector - The complete package
// ============================================================

class SuccinctBitVector {
private:
    std::vector<uint64_t> bits_;
    size_t num_bits_ = 0;
    BlockRankSupport rank_support_;

public:
    using value_type = bool;
    using size_type = size_t;

    SuccinctBitVector() = default;

    explicit SuccinctBitVector(size_t num_bits, bool initial_value = false)
        : bits_((num_bits + 63) / 64, initial_value ? ~0ULL : 0ULL)
        , num_bits_(num_bits)
    {
        // Clear unused bits in last word
        if (num_bits_ % 64 != 0) {
            size_t last_word = bits_.size() - 1;
            size_t valid_bits = num_bits_ % 64;
            bits_[last_word] &= (1ULL << valid_bits) - 1;
        }
    }

    // Construct from bit pattern
    SuccinctBitVector(std::initializer_list<bool> bits)
        : bits_((bits.size() + 63) / 64, 0)
        , num_bits_(bits.size())
    {
        size_t i = 0;
        for (bool b : bits) {
            if (b) set(i);
            ++i;
        }
        build_rank_support();
    }

    // Build rank/select support (call after modifications)
    void build_rank_support() {
        rank_support_.build(bits_.data(), num_bits_);
    }

    // Size queries
    [[nodiscard]] size_t size() const noexcept { return num_bits_; }
    [[nodiscard]] size_t num_words() const noexcept { return bits_.size(); }
    [[nodiscard]] bool empty() const noexcept { return num_bits_ == 0; }

    // Bit access (read-only for zero-copy semantics)
    [[nodiscard]] bool operator[](size_t pos) const {
        size_t word_idx = pos / 64;
        size_t bit_idx = pos % 64;
        return (bits_[word_idx] >> bit_idx) & 1;
    }

    [[nodiscard]] bool test(size_t pos) const {
        return (*this)[pos];
    }

    // Bit modification (invalidates rank support)
    void set(size_t pos, bool value = true) {
        size_t word_idx = pos / 64;
        size_t bit_idx = pos % 64;
        if (value) {
            bits_[word_idx] |= (1ULL << bit_idx);
        } else {
            bits_[word_idx] &= ~(1ULL << bit_idx);
        }
    }

    void reset(size_t pos) {
        set(pos, false);
    }

    void flip(size_t pos) {
        size_t word_idx = pos / 64;
        size_t bit_idx = pos % 64;
        bits_[word_idx] ^= (1ULL << bit_idx);
    }

    // Rank: count 1s in [0, pos)
    [[nodiscard]] size_t rank(size_t pos) const {
        return rank_support_.rank(pos);
    }

    // Select: find position of k-th 1 (0-indexed)
    [[nodiscard]] size_t select(size_t k) const {
        return rank_support_.select(k);
    }

    // Access raw bit data
    [[nodiscard]] const uint64_t* data() const noexcept {
        return bits_.data();
    }

    [[nodiscard]] std::span<const uint64_t> words() const noexcept {
        return bits_;
    }

    // Serialization - ZERO COPY WIRE FORMAT
    template<typename Sink>
    static void encode(const SuccinctBitVector& bv, Sink& sink)
        requires BitSink<Sink>
    {
        // Encode number of bits
        codecs::EliasGamma::encode(static_cast<uint32_t>(bv.num_bits_), sink);

        // Encode bit data directly (zero-copy!)
        for (auto word : bv.bits_) {
            for (int i = 0; i < 64; ++i) {
                sink.write((word >> i) & 1);
            }
        }

        // Encode rank support structure
        bv.rank_support_.encode(sink);
    }

    template<typename Source>
    static SuccinctBitVector decode(Source& source)
        requires BitSource<Source>
    {
        SuccinctBitVector bv;

        // Decode number of bits
        bv.num_bits_ = codecs::EliasGamma::decode<uint32_t>(source);

        // Decode bit data
        size_t num_words = (bv.num_bits_ + 63) / 64;
        bv.bits_.resize(num_words);

        for (size_t w = 0; w < num_words; ++w) {
            uint64_t word = 0;
            for (int i = 0; i < 64; ++i) {
                word |= static_cast<uint64_t>(source.read()) << i;
            }
            bv.bits_[w] = word;
        }

        // Decode rank support
        bv.rank_support_ = BlockRankSupport::decode(source, bv.bits_.data());

        return bv;
    }
};

// ============================================================
//  Roaring Bitmap - Compressed integer sets
//  Efficient storage with automatic container selection
// ============================================================

class RoaringBitmap {
public:
    using value_type = uint32_t;
    using size_type = size_t;

private:
    // Container types based on cardinality within a chunk
    enum class ContainerType : uint8_t {
        Array = 0,   // Sorted uint16_t array (cardinality < 4096)
        Bitmap = 1,  // 8KB bit vector (cardinality >= 4096)
        Run = 2      // Run-length encoded (consecutive ranges)
    };

    // Threshold for switching between array and bitmap
    static constexpr size_t ARRAY_MAX_SIZE = 4096;
    static constexpr size_t BITMAP_SIZE = 65536;  // 2^16 bits = 8KB
    static constexpr size_t WORDS_PER_BITMAP = BITMAP_SIZE / 64;

    // A chunk covers values with the same high 16 bits
    struct Chunk {
        uint16_t key;           // High 16 bits of values in this chunk
        ContainerType type;

        // Container data (only one is active based on type)
        std::vector<uint16_t> array;   // For Array type
        std::vector<uint64_t> bitmap;  // For Bitmap type (1024 words = 8KB)
        std::vector<std::pair<uint16_t, uint16_t>> runs;  // For Run type: (start, length-1)

        Chunk() = default;
        explicit Chunk(uint16_t k) : key(k), type(ContainerType::Array) {}

        [[nodiscard]] size_t cardinality() const {
            switch (type) {
                case ContainerType::Array:
                    return array.size();
                case ContainerType::Bitmap: {
                    size_t count = 0;
                    for (auto word : bitmap) {
                        count += __builtin_popcountll(word);
                    }
                    return count;
                }
                case ContainerType::Run: {
                    size_t count = 0;
                    for (const auto& [start, len] : runs) {
                        count += len + 1;
                    }
                    return count;
                }
            }
            return 0;
        }

        [[nodiscard]] bool contains(uint16_t low) const {
            switch (type) {
                case ContainerType::Array: {
                    // Binary search in sorted array
                    auto it = std::lower_bound(array.begin(), array.end(), low);
                    return it != array.end() && *it == low;
                }
                case ContainerType::Bitmap: {
                    size_t word_idx = low / 64;
                    size_t bit_idx = low % 64;
                    return (bitmap[word_idx] >> bit_idx) & 1;
                }
                case ContainerType::Run: {
                    // Binary search for run containing low
                    for (const auto& [start, len] : runs) {
                        if (low >= start && low <= start + len) return true;
                        if (start > low) break;
                    }
                    return false;
                }
            }
            return false;
        }

        void insert(uint16_t low) {
            switch (type) {
                case ContainerType::Array: {
                    auto it = std::lower_bound(array.begin(), array.end(), low);
                    if (it == array.end() || *it != low) {
                        array.insert(it, low);
                        // Convert to bitmap if too large
                        if (array.size() > ARRAY_MAX_SIZE) {
                            convert_to_bitmap();
                        }
                    }
                    break;
                }
                case ContainerType::Bitmap: {
                    size_t word_idx = low / 64;
                    size_t bit_idx = low % 64;
                    bitmap[word_idx] |= (1ULL << bit_idx);
                    break;
                }
                case ContainerType::Run: {
                    // For simplicity, convert to array, insert, then optimize
                    convert_to_array();
                    insert(low);
                    optimize();
                    break;
                }
            }
        }

        void remove(uint16_t low) {
            switch (type) {
                case ContainerType::Array: {
                    auto it = std::lower_bound(array.begin(), array.end(), low);
                    if (it != array.end() && *it == low) {
                        array.erase(it);
                    }
                    break;
                }
                case ContainerType::Bitmap: {
                    size_t word_idx = low / 64;
                    size_t bit_idx = low % 64;
                    bitmap[word_idx] &= ~(1ULL << bit_idx);
                    // Convert back to array if sparse
                    if (cardinality() <= ARRAY_MAX_SIZE) {
                        convert_to_array();
                    }
                    break;
                }
                case ContainerType::Run: {
                    convert_to_array();
                    remove(low);
                    break;
                }
            }
        }

        void convert_to_bitmap() {
            if (type == ContainerType::Bitmap) return;

            std::vector<uint64_t> new_bitmap(WORDS_PER_BITMAP, 0);

            if (type == ContainerType::Array) {
                for (uint16_t val : array) {
                    size_t word_idx = val / 64;
                    size_t bit_idx = val % 64;
                    new_bitmap[word_idx] |= (1ULL << bit_idx);
                }
                array.clear();
                array.shrink_to_fit();
            } else if (type == ContainerType::Run) {
                for (const auto& [start, len] : runs) {
                    for (uint32_t v = start; v <= static_cast<uint32_t>(start) + len; ++v) {
                        size_t word_idx = v / 64;
                        size_t bit_idx = v % 64;
                        new_bitmap[word_idx] |= (1ULL << bit_idx);
                    }
                }
                runs.clear();
                runs.shrink_to_fit();
            }

            bitmap = std::move(new_bitmap);
            type = ContainerType::Bitmap;
        }

        void convert_to_array() {
            if (type == ContainerType::Array) return;

            std::vector<uint16_t> new_array;

            if (type == ContainerType::Bitmap) {
                new_array.reserve(cardinality());
                for (size_t w = 0; w < WORDS_PER_BITMAP; ++w) {
                    uint64_t word = bitmap[w];
                    while (word) {
                        size_t bit = __builtin_ctzll(word);
                        new_array.push_back(static_cast<uint16_t>(w * 64 + bit));
                        word &= word - 1;  // Clear lowest set bit
                    }
                }
                bitmap.clear();
                bitmap.shrink_to_fit();
            } else if (type == ContainerType::Run) {
                for (const auto& [start, len] : runs) {
                    for (uint32_t v = start; v <= static_cast<uint32_t>(start) + len; ++v) {
                        new_array.push_back(static_cast<uint16_t>(v));
                    }
                }
                runs.clear();
                runs.shrink_to_fit();
            }

            array = std::move(new_array);
            type = ContainerType::Array;
        }

        // Optimize container representation
        void optimize() {
            size_t card = cardinality();

            // Check if run encoding would be more efficient
            if (type == ContainerType::Array || type == ContainerType::Bitmap) {
                std::vector<std::pair<uint16_t, uint16_t>> potential_runs;

                if (type == ContainerType::Array && !array.empty()) {
                    uint16_t run_start = array[0];
                    uint16_t run_len = 0;

                    for (size_t i = 1; i < array.size(); ++i) {
                        if (array[i] == array[i-1] + 1) {
                            ++run_len;
                        } else {
                            potential_runs.emplace_back(run_start, run_len);
                            run_start = array[i];
                            run_len = 0;
                        }
                    }
                    potential_runs.emplace_back(run_start, run_len);

                    // Run encoding uses 4 bytes per run
                    // Array uses 2 bytes per element
                    // Bitmap uses 8KB fixed
                    size_t run_bytes = potential_runs.size() * 4;
                    size_t array_bytes = array.size() * 2;

                    if (run_bytes < array_bytes && run_bytes < 8192) {
                        runs = std::move(potential_runs);
                        array.clear();
                        array.shrink_to_fit();
                        type = ContainerType::Run;
                    }
                }
            }

            // Convert bitmap to array if sparse enough
            if (type == ContainerType::Bitmap && card <= ARRAY_MAX_SIZE) {
                convert_to_array();
            }
        }
    };

    std::vector<Chunk> chunks_;  // Sorted by key

    // Find or create chunk for given high 16 bits
    Chunk* find_or_create_chunk(uint16_t key) {
        auto it = std::lower_bound(chunks_.begin(), chunks_.end(), key,
            [](const Chunk& c, uint16_t k) { return c.key < k; });

        if (it != chunks_.end() && it->key == key) {
            return &(*it);
        }

        it = chunks_.insert(it, Chunk(key));
        return &(*it);
    }

    const Chunk* find_chunk(uint16_t key) const {
        auto it = std::lower_bound(chunks_.begin(), chunks_.end(), key,
            [](const Chunk& c, uint16_t k) { return c.key < k; });

        if (it != chunks_.end() && it->key == key) {
            return &(*it);
        }
        return nullptr;
    }

public:
    RoaringBitmap() = default;

    // Construct from initializer list
    RoaringBitmap(std::initializer_list<uint32_t> values) {
        for (uint32_t v : values) {
            insert(v);
        }
        optimize();
    }

    // Construct from range
    template<typename Iterator>
    RoaringBitmap(Iterator begin, Iterator end) {
        for (auto it = begin; it != end; ++it) {
            insert(*it);
        }
        optimize();
    }

    // ==================== Core Operations ====================

    [[nodiscard]] bool contains(uint32_t x) const {
        uint16_t high = static_cast<uint16_t>(x >> 16);
        uint16_t low = static_cast<uint16_t>(x & 0xFFFF);

        const Chunk* chunk = find_chunk(high);
        return chunk && chunk->contains(low);
    }

    void insert(uint32_t x) {
        uint16_t high = static_cast<uint16_t>(x >> 16);
        uint16_t low = static_cast<uint16_t>(x & 0xFFFF);

        Chunk* chunk = find_or_create_chunk(high);
        chunk->insert(low);
    }

    void remove(uint32_t x) {
        uint16_t high = static_cast<uint16_t>(x >> 16);
        uint16_t low = static_cast<uint16_t>(x & 0xFFFF);

        auto it = std::lower_bound(chunks_.begin(), chunks_.end(), high,
            [](const Chunk& c, uint16_t k) { return c.key < k; });

        if (it != chunks_.end() && it->key == high) {
            it->remove(low);
            // Remove empty chunks
            if (it->cardinality() == 0) {
                chunks_.erase(it);
            }
        }
    }

    // ==================== Size and Iteration ====================

    [[nodiscard]] size_t cardinality() const {
        size_t count = 0;
        for (const auto& chunk : chunks_) {
            count += chunk.cardinality();
        }
        return count;
    }

    [[nodiscard]] size_t size() const { return cardinality(); }

    [[nodiscard]] bool empty() const {
        return chunks_.empty();
    }

    void clear() {
        chunks_.clear();
    }

    // Optimize all chunks
    void optimize() {
        for (auto& chunk : chunks_) {
            chunk.optimize();
        }
    }

    // ==================== Iterator ====================

    class iterator {
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = uint32_t;
        using difference_type = std::ptrdiff_t;
        using pointer = const uint32_t*;
        using reference = uint32_t;

    private:
        const RoaringBitmap* rb_;
        size_t chunk_idx_;
        size_t pos_in_chunk_;
        uint32_t current_value_;
        bool at_end_;

        void advance_to_next() {
            while (chunk_idx_ < rb_->chunks_.size()) {
                const Chunk& chunk = rb_->chunks_[chunk_idx_];
                uint32_t high = static_cast<uint32_t>(chunk.key) << 16;

                switch (chunk.type) {
                    case ContainerType::Array:
                        if (pos_in_chunk_ < chunk.array.size()) {
                            current_value_ = high | chunk.array[pos_in_chunk_];
                            return;
                        }
                        break;
                    case ContainerType::Bitmap:
                        while (pos_in_chunk_ < BITMAP_SIZE) {
                            size_t word_idx = pos_in_chunk_ / 64;
                            size_t bit_idx = pos_in_chunk_ % 64;
                            if ((chunk.bitmap[word_idx] >> bit_idx) & 1) {
                                current_value_ = high | static_cast<uint16_t>(pos_in_chunk_);
                                return;
                            }
                            ++pos_in_chunk_;
                        }
                        break;
                    case ContainerType::Run:
                        // pos_in_chunk_ encodes: high bits = run index, low bits = offset in run
                        if (!chunk.runs.empty()) {
                            size_t run_idx = pos_in_chunk_ >> 16;
                            size_t offset = pos_in_chunk_ & 0xFFFF;

                            while (run_idx < chunk.runs.size()) {
                                const auto& [start, len] = chunk.runs[run_idx];
                                if (offset <= len) {
                                    current_value_ = high | static_cast<uint16_t>(start + offset);
                                    return;
                                }
                                ++run_idx;
                                offset = 0;
                                pos_in_chunk_ = (run_idx << 16);
                            }
                        }
                        break;
                }

                ++chunk_idx_;
                pos_in_chunk_ = 0;
            }
            at_end_ = true;
        }

    public:
        iterator() : rb_(nullptr), chunk_idx_(0), pos_in_chunk_(0), current_value_(0), at_end_(true) {}

        iterator(const RoaringBitmap* rb, bool end = false)
            : rb_(rb), chunk_idx_(0), pos_in_chunk_(0), current_value_(0), at_end_(end)
        {
            if (!end && rb_ && !rb_->chunks_.empty()) {
                advance_to_next();
            } else {
                at_end_ = true;
            }
        }

        reference operator*() const { return current_value_; }

        iterator& operator++() {
            if (at_end_) return *this;

            const Chunk& chunk = rb_->chunks_[chunk_idx_];

            switch (chunk.type) {
                case ContainerType::Array:
                    ++pos_in_chunk_;
                    break;
                case ContainerType::Bitmap:
                    ++pos_in_chunk_;
                    break;
                case ContainerType::Run: {
                    size_t run_idx = pos_in_chunk_ >> 16;
                    size_t offset = pos_in_chunk_ & 0xFFFF;
                    const auto& [start, len] = chunk.runs[run_idx];
                    if (offset < len) {
                        pos_in_chunk_ = (run_idx << 16) | (offset + 1);
                    } else {
                        pos_in_chunk_ = ((run_idx + 1) << 16);
                    }
                    break;
                }
            }

            advance_to_next();
            return *this;
        }

        iterator operator++(int) {
            iterator tmp = *this;
            ++(*this);
            return tmp;
        }

        bool operator==(const iterator& other) const {
            if (at_end_ && other.at_end_) return true;
            if (at_end_ || other.at_end_) return false;
            return rb_ == other.rb_ && chunk_idx_ == other.chunk_idx_ &&
                   pos_in_chunk_ == other.pos_in_chunk_;
        }

        bool operator!=(const iterator& other) const {
            return !(*this == other);
        }
    };

    [[nodiscard]] iterator begin() const { return iterator(this, false); }
    [[nodiscard]] iterator end() const { return iterator(this, true); }

    // ==================== Set Operations ====================

    [[nodiscard]] RoaringBitmap union_with(const RoaringBitmap& other) const {
        RoaringBitmap result;

        size_t i = 0, j = 0;
        while (i < chunks_.size() && j < other.chunks_.size()) {
            if (chunks_[i].key < other.chunks_[j].key) {
                result.chunks_.push_back(chunks_[i]);
                ++i;
            } else if (chunks_[i].key > other.chunks_[j].key) {
                result.chunks_.push_back(other.chunks_[j]);
                ++j;
            } else {
                // Merge chunks with same key
                Chunk merged(chunks_[i].key);
                merged.convert_to_bitmap();

                // Copy from first chunk
                const Chunk& c1 = chunks_[i];
                if (c1.type == ContainerType::Bitmap) {
                    merged.bitmap = c1.bitmap;
                } else {
                    Chunk temp = c1;
                    temp.convert_to_bitmap();
                    merged.bitmap = std::move(temp.bitmap);
                }

                // OR with second chunk
                const Chunk& c2 = other.chunks_[j];
                if (c2.type == ContainerType::Bitmap) {
                    for (size_t w = 0; w < WORDS_PER_BITMAP; ++w) {
                        merged.bitmap[w] |= c2.bitmap[w];
                    }
                } else {
                    Chunk temp = c2;
                    temp.convert_to_bitmap();
                    for (size_t w = 0; w < WORDS_PER_BITMAP; ++w) {
                        merged.bitmap[w] |= temp.bitmap[w];
                    }
                }

                merged.optimize();
                result.chunks_.push_back(std::move(merged));
                ++i;
                ++j;
            }
        }

        // Add remaining chunks
        while (i < chunks_.size()) {
            result.chunks_.push_back(chunks_[i++]);
        }
        while (j < other.chunks_.size()) {
            result.chunks_.push_back(other.chunks_[j++]);
        }

        return result;
    }

    [[nodiscard]] RoaringBitmap intersect_with(const RoaringBitmap& other) const {
        RoaringBitmap result;

        size_t i = 0, j = 0;
        while (i < chunks_.size() && j < other.chunks_.size()) {
            if (chunks_[i].key < other.chunks_[j].key) {
                ++i;
            } else if (chunks_[i].key > other.chunks_[j].key) {
                ++j;
            } else {
                // Intersect chunks with same key
                Chunk intersected(chunks_[i].key);
                intersected.convert_to_bitmap();

                // Copy from first chunk
                const Chunk& c1 = chunks_[i];
                if (c1.type == ContainerType::Bitmap) {
                    intersected.bitmap = c1.bitmap;
                } else {
                    Chunk temp = c1;
                    temp.convert_to_bitmap();
                    intersected.bitmap = std::move(temp.bitmap);
                }

                // AND with second chunk
                const Chunk& c2 = other.chunks_[j];
                if (c2.type == ContainerType::Bitmap) {
                    for (size_t w = 0; w < WORDS_PER_BITMAP; ++w) {
                        intersected.bitmap[w] &= c2.bitmap[w];
                    }
                } else {
                    Chunk temp = c2;
                    temp.convert_to_bitmap();
                    for (size_t w = 0; w < WORDS_PER_BITMAP; ++w) {
                        intersected.bitmap[w] &= temp.bitmap[w];
                    }
                }

                if (intersected.cardinality() > 0) {
                    intersected.optimize();
                    result.chunks_.push_back(std::move(intersected));
                }
                ++i;
                ++j;
            }
        }

        return result;
    }

    [[nodiscard]] RoaringBitmap difference(const RoaringBitmap& other) const {
        RoaringBitmap result;

        size_t i = 0, j = 0;
        while (i < chunks_.size()) {
            // Skip other chunks with smaller keys
            while (j < other.chunks_.size() && other.chunks_[j].key < chunks_[i].key) {
                ++j;
            }

            if (j >= other.chunks_.size() || other.chunks_[j].key > chunks_[i].key) {
                // No matching chunk in other, copy entire chunk
                result.chunks_.push_back(chunks_[i]);
            } else {
                // Subtract chunks with same key
                Chunk diff(chunks_[i].key);
                diff.convert_to_bitmap();

                const Chunk& c1 = chunks_[i];
                if (c1.type == ContainerType::Bitmap) {
                    diff.bitmap = c1.bitmap;
                } else {
                    Chunk temp = c1;
                    temp.convert_to_bitmap();
                    diff.bitmap = std::move(temp.bitmap);
                }

                const Chunk& c2 = other.chunks_[j];
                if (c2.type == ContainerType::Bitmap) {
                    for (size_t w = 0; w < WORDS_PER_BITMAP; ++w) {
                        diff.bitmap[w] &= ~c2.bitmap[w];
                    }
                } else {
                    Chunk temp = c2;
                    temp.convert_to_bitmap();
                    for (size_t w = 0; w < WORDS_PER_BITMAP; ++w) {
                        diff.bitmap[w] &= ~temp.bitmap[w];
                    }
                }

                if (diff.cardinality() > 0) {
                    diff.optimize();
                    result.chunks_.push_back(std::move(diff));
                }
                ++j;
            }
            ++i;
        }

        return result;
    }

    // Operator overloads for set operations
    RoaringBitmap operator|(const RoaringBitmap& other) const { return union_with(other); }
    RoaringBitmap operator&(const RoaringBitmap& other) const { return intersect_with(other); }
    RoaringBitmap operator-(const RoaringBitmap& other) const { return difference(other); }

    // ==================== Serialization ====================

    template<typename Sink>
    static void encode(const RoaringBitmap& rb, Sink& sink) requires BitSink<Sink> {
        // Encode number of chunks
        codecs::EliasGamma::encode(static_cast<uint32_t>(rb.chunks_.size()), sink);

        for (const auto& chunk : rb.chunks_) {
            // Encode chunk key
            codecs::Fixed<16>::encode(chunk.key, sink);

            // Encode container type
            codecs::Fixed<2>::encode(static_cast<uint8_t>(chunk.type), sink);

            switch (chunk.type) {
                case ContainerType::Array:
                    codecs::EliasGamma::encode(static_cast<uint32_t>(chunk.array.size()), sink);
                    for (uint16_t val : chunk.array) {
                        codecs::Fixed<16>::encode(val, sink);
                    }
                    break;

                case ContainerType::Bitmap:
                    for (uint64_t word : chunk.bitmap) {
                        codecs::Fixed<64>::encode(word, sink);
                    }
                    break;

                case ContainerType::Run:
                    codecs::EliasGamma::encode(static_cast<uint32_t>(chunk.runs.size()), sink);
                    for (const auto& [start, len] : chunk.runs) {
                        codecs::Fixed<16>::encode(start, sink);
                        codecs::Fixed<16>::encode(len, sink);
                    }
                    break;
            }
        }
    }

    template<typename Source>
    static RoaringBitmap decode(Source& source) requires BitSource<Source> {
        RoaringBitmap rb;

        uint32_t num_chunks = codecs::EliasGamma::decode<uint32_t>(source);
        rb.chunks_.reserve(num_chunks);

        for (uint32_t c = 0; c < num_chunks; ++c) {
            Chunk chunk;
            chunk.key = codecs::Fixed<16>::decode<uint16_t>(source);
            chunk.type = static_cast<ContainerType>(codecs::Fixed<2>::decode<uint8_t>(source));

            switch (chunk.type) {
                case ContainerType::Array: {
                    uint32_t size = codecs::EliasGamma::decode<uint32_t>(source);
                    chunk.array.reserve(size);
                    for (uint32_t i = 0; i < size; ++i) {
                        chunk.array.push_back(codecs::Fixed<16>::decode<uint16_t>(source));
                    }
                    break;
                }

                case ContainerType::Bitmap:
                    chunk.bitmap.resize(WORDS_PER_BITMAP);
                    for (size_t w = 0; w < WORDS_PER_BITMAP; ++w) {
                        chunk.bitmap[w] = codecs::Fixed<64>::decode<uint64_t>(source);
                    }
                    break;

                case ContainerType::Run: {
                    uint32_t num_runs = codecs::EliasGamma::decode<uint32_t>(source);
                    chunk.runs.reserve(num_runs);
                    for (uint32_t i = 0; i < num_runs; ++i) {
                        uint16_t start = codecs::Fixed<16>::decode<uint16_t>(source);
                        uint16_t len = codecs::Fixed<16>::decode<uint16_t>(source);
                        chunk.runs.emplace_back(start, len);
                    }
                    break;
                }
            }

            rb.chunks_.push_back(std::move(chunk));
        }

        return rb;
    }

    // ==================== Statistics ====================

    struct Stats {
        size_t num_chunks;
        size_t num_array_chunks;
        size_t num_bitmap_chunks;
        size_t num_run_chunks;
        size_t total_cardinality;
        size_t memory_bytes;
    };

    [[nodiscard]] Stats stats() const {
        Stats s{};
        s.num_chunks = chunks_.size();

        for (const auto& chunk : chunks_) {
            s.total_cardinality += chunk.cardinality();

            switch (chunk.type) {
                case ContainerType::Array:
                    ++s.num_array_chunks;
                    s.memory_bytes += sizeof(Chunk) + chunk.array.size() * sizeof(uint16_t);
                    break;
                case ContainerType::Bitmap:
                    ++s.num_bitmap_chunks;
                    s.memory_bytes += sizeof(Chunk) + chunk.bitmap.size() * sizeof(uint64_t);
                    break;
                case ContainerType::Run:
                    ++s.num_run_chunks;
                    s.memory_bytes += sizeof(Chunk) + chunk.runs.size() * sizeof(std::pair<uint16_t, uint16_t>);
                    break;
            }
        }

        return s;
    }
};

} // namespace pfc::succinct

// Make SuccinctBitVector a PackedValue (must be in std namespace)
namespace std {
template<>
struct tuple_size<pfc::succinct::SuccinctBitVector>
    : std::integral_constant<size_t, 1> {};
}

// ============================================================
//  DESIGN NOTES FOR FUTURE SUCCINCT STRUCTURES
// ============================================================

/*

ROARING BITMAPS
===============
Compressed integer sets with O(1) membership, efficient set operations.

Design sketch:

class RoaringBitmap {
    // Three container types based on cardinality:
    // - Array: sorted uint16_t array (< 4096 elements)
    // - Bitmap: 8KB bit vector (dense sets)
    // - Run: run-length encoded runs

    struct Chunk {
        uint16_t key;        // High 16 bits
        uint8_t type;        // 0=array, 1=bitmap, 2=run
        PackedVector<uint16_t> data;  // Low 16 bits
    };

    PackedVector<Chunk> chunks_;

    // STL set-like interface
    bool contains(uint32_t x) const;
    void insert(uint32_t x);
    void remove(uint32_t x);

    // Set operations (zero-copy where possible)
    RoaringBitmap union_with(const RoaringBitmap& other) const;
    RoaringBitmap intersect_with(const RoaringBitmap& other) const;

    // Zero-copy wire format
    template<typename Sink>
    static void encode(const RoaringBitmap& rb, Sink& sink);
};

INTERPOLATIVE CODING
====================
Optimal compression for sorted integer sequences.

class InterpolativeCoded {
    SuccinctBitVector bits_;
    size_t universe_size_;
    size_t num_elements_;

    // Recursive encoding of sorted integers
    void encode_range(const std::vector<uint32_t>& vals,
                     size_t low, size_t high,
                     uint32_t min_val, uint32_t max_val);

    // O(log n) access with binary interpolation search
    uint32_t operator[](size_t index) const;

    // Zero-copy wire format
    template<typename Sink>
    static void encode(const InterpolativeCoded& ic, Sink& sink);
};

WAVELET TREE
============
Powerful sequence structure with rank/select on arbitrary alphabets.

template<typename Symbol = uint8_t>
class WaveletTree {
    SuccinctBitVector level_bits_;
    std::vector<size_t> level_boundaries_;
    size_t alphabet_size_;

    // O(log σ) rank/select for any symbol
    size_t rank(Symbol s, size_t pos) const;
    size_t select(Symbol s, size_t k) const;

    // O(log σ) access
    Symbol operator[](size_t pos) const;

    // Range queries
    std::vector<Symbol> range(size_t start, size_t end) const;

    // Zero-copy wire format
    template<typename Sink>
    static void encode(const WaveletTree& wt, Sink& sink);
};

PACKED GRAPHS
=============
Succinct graph representations with zero-copy semantics.

// Compressed Sparse Row (CSR) format
class PackedGraph {
    PackedVector<uint32_t, codecs::EliasDelta> offsets_;
    PackedVector<uint32_t, codecs::VByte> edges_;

    struct Neighbors {
        // Proxy iterator for zero-copy access
        class iterator {
            const PackedGraph* graph_;
            size_t pos_;

            uint32_t operator*() const {
                return graph_->edges_[pos_];
            }
        };

        iterator begin() const;
        iterator end() const;
    };

    // Graph queries
    size_t num_vertices() const;
    size_t num_edges() const;
    Neighbors neighbors(uint32_t v) const;

    // Zero-copy wire format
    template<typename Sink>
    static void encode(const PackedGraph& g, Sink& sink);
};

RANK-SELECT DICTIONARY
======================
Space-efficient representation of sparse bit vectors.

class RankSelectDictionary {
    // Store only positions of 1s
    PackedVector<uint32_t, codecs::EliasDelta> ones_;
    size_t universe_size_;

    // O(log n) rank via binary search
    size_t rank(size_t pos) const;

    // O(1) select via direct access
    size_t select(size_t k) const { return ones_[k]; }

    // Zero-copy wire format
    template<typename Sink>
    static void encode(const RankSelectDictionary& rsd, Sink& sink);
};

DESIGN PRINCIPLES
=================

1. ZERO-COPY FIRST
   - Wire format = in-memory format
   - No parsing step, direct memory mapping
   - Prefix-free encoding for self-delimiting

2. COMPOSABILITY
   - Build on existing PackedValue types
   - Use codec composition (VByte for metadata, Delta for deltas)
   - Mix and match structures

3. STL-ADJACENT
   - Familiar interfaces (vector-like, set-like)
   - Standard algorithms work via proxy iterators
   - Range support

4. PERFORMANCE
   - Cache-friendly layouts (64-byte blocks)
   - SIMD-ready where applicable (VByte, popcount)
   - O(1) or O(log n) operations

5. IMMUTABLE BY DEFAULT
   - Builders for construction
   - Frozen structures for queries
   - Clear separation of concerns

EXAMPLE USAGE
=============

// Build a succinct bit vector
SuccinctBitVector bv(1000);
for (size_t i = 0; i < 1000; i += 7) {
    bv.set(i);
}
bv.build_rank_support();

// Query operations
size_t ones_before_100 = bv.rank(100);  // O(1)
size_t tenth_one = bv.select(10);       // O(log n)

// Zero-copy serialization
uint8_t buffer[10000];
BitWriter writer(buffer);
SuccinctBitVector::encode(bv, writer);
writer.align();

// Zero-copy deserialization (no parsing!)
BitReader reader(buffer, 10000);
auto bv2 = SuccinctBitVector::decode(reader);

// Use in packed structures
using PackedBitVec = Packed<SuccinctBitVector, SuccinctBitVector>;
PackedVector<PackedBitVec> collection;

// Roaring bitmap example (future)
RoaringBitmap bitmap;
for (uint32_t i : {1, 5, 100, 1000, 100000}) {
    bitmap.insert(i);
}

auto bitmap2 = bitmap.union_with(other_bitmap);  // Zero-copy where possible

// Wavelet tree example (future)
WaveletTree<char> wt("the quick brown fox");
size_t q_count = wt.rank('q', wt.size());  // Count 'q's
size_t first_q = wt.select('q', 0);        // Find first 'q'

*/
