# Succinct Data Structures Design Document

## Executive Summary

This document outlines the architecture for adding succinct data structures to the PFC library, leveraging the library's unique competitive advantage: **wire format = in-memory format** (zero-copy).

## Quick Wins - Phase 1 (COMPLETED)

### Three New Universal Codes

#### 1. VByte (Variable Byte / Varint) ✓
- **Status**: Implemented and tested
- **Use Case**: Industry standard, Protocol Buffers compatible
- **Performance**: Cache-friendly, SIMD-ready
- **Encoding**: 7 data bits + 1 continuation bit per byte
- **Advantages**:
  - Fast encoding/decoding
  - Byte-aligned for efficient processing
  - Widely used in practice (Lucene, Protocol Buffers)

```cpp
// Example usage
uint8_t buffer[100];
BitWriter writer(buffer);
codecs::VByte::encode(1000000u, writer);

BitReader reader(buffer, 100);
uint32_t value = codecs::VByte::decode<uint32_t>(reader);
```

#### 2. Exponential-Golomb (Parameterized Family) ✓
- **Status**: Implemented and tested
- **Use Case**: Video coding standard (H.264, HEVC)
- **Performance**: Tunable for different distributions
- **Orders**:
  - Order 0 = Elias Gamma
  - Higher orders = flatter distributions
- **Advantages**:
  - Flexible parameterization
  - Well-studied theoretical properties
  - Hardware-friendly in video codecs

```cpp
// Order 0 (same as Elias Gamma)
codecs::ExpGolomb<0>::encode(value, sink);

// Order 1 (better for uniform distributions)
codecs::ExpGolomb<1>::encode(value, sink);

// Convenience aliases
using ExpGolomb0 = ExpGolomb<0>;
using ExpGolomb1 = ExpGolomb<1>;
using ExpGolomb2 = ExpGolomb<2>;
```

#### 3. Elias Omega ✓
- **Status**: Implemented and tested
- **Use Case**: Large integers (better than Delta)
- **Performance**: Recursive length encoding
- **Advantages**:
  - More efficient than Delta for large values
  - Theoretically optimal within log* factor
  - Completes the Elias family

```cpp
uint8_t buffer[100];
BitWriter writer(buffer);
codecs::EliasOmega::encode(1000000u, writer);

BitReader reader(buffer, 100);
uint32_t value = codecs::EliasOmega::decode<uint32_t>(reader);
```

### Test Results
- **All tests passing**: 2000 assertions across 9 test cases
- **Coverage**: Small values, large values, powers of 2, random data
- **Signed variants**: SignedVByte, SignedOmega included
- **Comparative tests**: Efficiency comparisons between codecs

## Succinct Structures - Phase 2 Foundation

### 1. Succinct Bit Vector with Rank/Select

**Status**: API designed, implementation in progress

#### Design Principles
1. **Space**: n + o(n) bits total
2. **Operations**: O(1) rank, O(log n) select
3. **Zero-Copy**: Wire format = in-memory format
4. **Composable**: Built on PackedValue concept

#### Architecture

```cpp
class SuccinctBitVector {
private:
    std::vector<uint64_t> bits_;           // Bit data
    size_t num_bits_;                      // Number of bits
    BlockRankSupport rank_support_;        // Rank/select metadata

public:
    // Construction
    SuccinctBitVector(size_t num_bits, bool initial_value = false);

    // Build rank support (call after modifications)
    void build_rank_support();

    // Bit access
    bool operator[](size_t pos) const;     // O(1) read
    void set(size_t pos, bool value = true);
    void reset(size_t pos);
    void flip(size_t pos);

    // Rank/Select operations
    size_t rank(size_t pos) const;         // O(1)
    size_t select(size_t k) const;         // O(log n)

    // Zero-copy serialization
    template<typename Sink>
    static void encode(const SuccinctBitVector& bv, Sink& sink);

    template<typename Source>
    static SuccinctBitVector decode(Source& source);
};
```

#### Block-Based Rank Support

```
Structure:
┌─────────────────────────────────────────────────────┐
│ Superblock (65536 bits = 8KB)                       │
│  ┌──────────────┬──────────────┬──────────────┐    │
│  │ Block (512b) │ Block (512b) │ Block (512b) │... │
│  └──────────────┴──────────────┴──────────────┘    │
└─────────────────────────────────────────────────────┘

Metadata:
- Superblocks: Absolute rank every 65536 bits (32-bit)
- Blocks: Relative rank within superblock (16-bit)
- Space overhead: n/512 + n/65536 = 0.196% + 0.003% ≈ 0.2%
```

#### Zero-Copy Wire Format

```
Encoding:
┌────────────┬──────────────┬───────────────────┐
│ num_bits   │ bit_data     │ rank_support      │
│ (Gamma)    │ (raw bits)   │ (blocks+metadata) │
└────────────┴──────────────┴───────────────────┘

Key advantage: bit_data is directly memory-mappable!
No parsing, no decoding - just point and use.
```

### Example Usage

```cpp
// Build a succinct bit vector
SuccinctBitVector bv(1000);
for (size_t i = 0; i < 1000; i += 7) {
    bv.set(i);  // Set every 7th bit
}
bv.build_rank_support();

// Query operations
size_t ones_before_100 = bv.rank(100);     // Count 1s in [0, 100)
size_t tenth_one = bv.select(10);          // Find 10th 1

// Zero-copy serialization
uint8_t buffer[10000];
BitWriter writer(buffer);
SuccinctBitVector::encode(bv, writer);
writer.align();

// Zero-copy deserialization (no parsing!)
BitReader reader(buffer, 10000);
auto bv2 = SuccinctBitVector::decode(reader);

// Works immediately - no build step needed
size_t rank_check = bv2.rank(100);
assert(rank_check == ones_before_100);
```

## Phase 3: Advanced Succinct Structures

### 2. Roaring Bitmaps

**Compressed integer sets with O(1) operations**

#### Design Sketch

```cpp
class RoaringBitmap {
private:
    // Three container types based on cardinality:
    enum class ContainerType : uint8_t {
        Array,   // Sorted uint16_t array (< 4096 elements)
        Bitmap,  // 8KB bit vector (dense sets)
        Run      // Run-length encoded runs
    };

    struct Chunk {
        uint16_t key;                      // High 16 bits
        ContainerType type;                // Container type
        PackedVector<uint16_t> data;       // Low 16 bits
    };

    PackedVector<Chunk> chunks_;           // Sorted by key

public:
    // STL set-like interface
    bool contains(uint32_t x) const;       // O(log n + 1)
    void insert(uint32_t x);               // O(log n + k)
    void remove(uint32_t x);               // O(log n + k)

    // Set operations (zero-copy where possible)
    RoaringBitmap union_with(const RoaringBitmap& other) const;
    RoaringBitmap intersect_with(const RoaringBitmap& other) const;

    // Iteration
    class iterator {
        // Proxy iterator for zero-copy access
        uint32_t operator*() const;
    };

    // Zero-copy wire format
    template<typename Sink>
    static void encode(const RoaringBitmap& rb, Sink& sink);
};
```

#### Zero-Copy Advantage

```
Traditional Roaring:
┌──────────┬──────────┬──────────┐
│ Chunk 1  │ Chunk 2  │ Chunk 3  │
└──────────┴──────────┴──────────┘
         ↓
    Parse each chunk type
    Allocate containers
    Copy data

PFC Roaring:
┌──────────┬──────────┬──────────┐
│ Chunk 1  │ Chunk 2  │ Chunk 3  │
└──────────┴──────────┴──────────┘
         ↓
    Point directly to wire format
    Zero parsing, zero allocation
    Instant access
```

### 3. Interpolative Coding

**Optimal compression for sorted integers**

```cpp
class InterpolativeCoded {
private:
    SuccinctBitVector bits_;               // Encoded bit stream
    size_t universe_size_;                 // Maximum value
    size_t num_elements_;                  // Number of integers

    // Recursive encoding helper
    void encode_range(const std::vector<uint32_t>& vals,
                     size_t low, size_t high,
                     uint32_t min_val, uint32_t max_val);

public:
    // Construction
    static InterpolativeCoded from_sorted_array(
        const std::vector<uint32_t>& vals,
        uint32_t universe_size
    );

    // O(log n) access with binary interpolation search
    uint32_t operator[](size_t index) const;

    // Iteration
    class iterator {
        uint32_t operator*() const;
    };

    // Zero-copy wire format
    template<typename Sink>
    static void encode(const InterpolativeCoded& ic, Sink& sink);
};
```

#### Use Cases
- Inverted indices (document IDs)
- Sorted sensor readings
- Timestamp sequences
- Any monotonic sequence

### 4. Wavelet Tree

**Powerful sequence structure with rank/select on arbitrary alphabets**

```cpp
template<typename Symbol = uint8_t>
class WaveletTree {
private:
    SuccinctBitVector level_bits_;         // All levels concatenated
    std::vector<size_t> level_boundaries_; // Start of each level
    size_t alphabet_size_;                 // Size of alphabet

public:
    // Construction
    template<typename Iterator>
    static WaveletTree from_sequence(Iterator begin, Iterator end);

    // O(log σ) rank/select for any symbol
    size_t rank(Symbol s, size_t pos) const;
    size_t select(Symbol s, size_t k) const;

    // O(log σ) access
    Symbol operator[](size_t pos) const;

    // Advanced queries
    std::vector<Symbol> range(size_t start, size_t end) const;
    std::pair<Symbol, size_t> most_frequent(size_t start, size_t end) const;

    // Zero-copy wire format
    template<typename Sink>
    static void encode(const WaveletTree& wt, Sink& sink);
};
```

#### Applications
- Text indexing with rank/select on any character
- Range queries on sequences
- Top-k queries
- Substring matching

### 5. Packed Graphs

**Succinct graph representations**

```cpp
// Compressed Sparse Row (CSR) format
class PackedGraph {
private:
    PackedVector<uint32_t, codecs::EliasDelta> offsets_;
    PackedVector<uint32_t, codecs::VByte> edges_;

public:
    // Graph construction
    template<typename EdgeList>
    static PackedGraph from_edges(const EdgeList& edges, size_t num_vertices);

    // Graph queries
    size_t num_vertices() const;
    size_t num_edges() const;

    // Neighbor access with proxy iterator
    struct Neighbors {
        class iterator {
            uint32_t operator*() const;
        };
        iterator begin() const;
        iterator end() const;
    };

    Neighbors neighbors(uint32_t v) const;

    // Graph algorithms (work with standard algorithms via iterators)
    template<typename Visitor>
    void for_each_neighbor(uint32_t v, Visitor&& vis) const;

    // Zero-copy wire format
    template<typename Sink>
    static void encode(const PackedGraph& g, Sink& sink);
};
```

#### Zero-Copy Advantage

```
Traditional CSR:
┌─────────────┬──────────────────────┐
│ Offsets     │ Edges                │
│ [0,3,7,...]│ [1,5,9,2,3,8,...]   │
└─────────────┴──────────────────────┘
         ↓
    Fixed 32-bit or 64-bit integers
    Wastes space for small values

PFC PackedGraph:
┌─────────────┬──────────────────────┐
│ Offsets     │ Edges                │
│ (Delta)     │ (VByte)              │
└─────────────┴──────────────────────┘
         ↓
    Variable-length encoding
    Wire format = in-memory format
    No decompression step
```

### 6. Rank-Select Dictionary

**Space-efficient sparse bit vectors**

```cpp
class RankSelectDictionary {
private:
    PackedVector<uint32_t, codecs::EliasDelta> ones_;
    size_t universe_size_;

public:
    // Construction
    template<typename Iterator>
    static RankSelectDictionary from_positions(
        Iterator begin, Iterator end,
        size_t universe_size
    );

    // O(log n) rank via binary search
    size_t rank(size_t pos) const;

    // O(1) select via direct access
    size_t select(size_t k) const { return ones_[k]; }

    // Size queries
    size_t size() const { return ones_.size(); }
    size_t universe_size() const { return universe_size_; }

    // Zero-copy wire format
    template<typename Sink>
    static void encode(const RankSelectDictionary& rsd, Sink& sink);
};
```

#### Use Case
When the bit vector is very sparse (< 1% density), this representation is more space-efficient than full rank/select structures.

## Design Principles

### 1. Zero-Copy First

**THE COMPETITIVE ADVANTAGE**

```
Traditional Approach:
┌──────────┐    Parse    ┌──────────┐    Build    ┌──────────┐
│   Wire   │──────────→ │  Memory  │──────────→ │  Query   │
│  Format  │             │  Format  │             │  Ready   │
└──────────┘             └──────────┘             └──────────┘
    ↓                         ↓                        ↓
  Compact               Expanded               Fast queries
                        + Metadata

PFC Approach:
┌──────────┐    mmap     ┌──────────┐
│   Wire   │──────────→ │  Query   │
│  Format  │             │  Ready   │
└──────────┘             └──────────┘
    ↓                        ↓
  Compact               Fast queries
  (SAME REPRESENTATION)
```

Key advantages:
- **Instant loading**: No parsing, no allocation
- **Memory efficiency**: One copy, not two
- **Cache efficiency**: Data is already organized for access
- **Persistence**: Write memory directly to disk

### 2. Composability

Build complex structures from simple ones:

```cpp
// Compose codecs
using GraphCodec = Adaptive<VByte, EliasDelta>;

// Compose packed values
using PackedBitVec = Packed<SuccinctBitVector, SuccinctBitVector>;

// Build collections
PackedVector<PackedBitVec> bitvector_collection;

// Nest structures
using WaveletForest = PackedVector<WaveletTree<uint8_t>>;
```

### 3. STL-Adjacent

Familiar interfaces, standard patterns:

```cpp
// STL vector-like
auto bv = SuccinctBitVector(1000);
size_t n = bv.size();
bool bit = bv[42];

// STL set-like
auto rb = RoaringBitmap();
rb.insert(42);
bool has = rb.contains(42);

// STL algorithms work
auto graph = PackedGraph::from_edges(edges, num_vertices);
auto neighbors = graph.neighbors(v);
auto count = std::distance(neighbors.begin(), neighbors.end());

// Range support
for (auto neighbor : graph.neighbors(v)) {
    process(neighbor);
}
```

### 4. Performance First

Cache-friendly, SIMD-ready, theoretically optimal:

```cpp
// Block sizes align with cache lines
static constexpr size_t BLOCK_SIZE = 512;  // 64 bytes

// Use hardware instructions
size_t popcount = __builtin_popcountll(word);

// Avoid branching in hot loops
result += (condition ? value : 0);  // NOT: if (condition) result += value;

// Batch operations
void encode_batch(const uint32_t* values, size_t count, Sink& sink);
```

### 5. Immutable by Default

Clear separation between construction and querying:

```cpp
// Construction phase (mutable)
SuccinctBitVector::Builder builder(1000);
for (size_t i = 0; i < 1000; i += 7) {
    builder.set(i);
}
auto bv = builder.build();  // Build rank support

// Query phase (immutable, thread-safe)
size_t rank = bv.rank(100);
size_t select = bv.select(10);

// Serialization (from immutable structure)
uint8_t buffer[10000];
BitWriter writer(buffer);
SuccinctBitVector::encode(bv, writer);
```

## Integration with Existing PFC

### Header Organization

```
include/pfc/
├── core.hpp              # Bit I/O, concepts
├── codecs.hpp            # Universal codes (now with VByte, ExpGolomb, Omega)
├── packed.hpp            # Packed value types
├── succinct.hpp          # Succinct bit vector (NEW)
├── succinct_roaring.hpp  # Roaring bitmaps (FUTURE)
├── succinct_wavelet.hpp  # Wavelet trees (FUTURE)
├── succinct_graph.hpp    # Packed graphs (FUTURE)
└── pfc.hpp              # Main header
```

### Namespace Structure

```cpp
namespace pfc {
    // Core functionality
    class BitWriter;
    class BitReader;

    namespace codecs {
        struct VByte;           // NEW
        struct ExpGolomb;       // NEW
        struct EliasOmega;      // NEW
        struct EliasGamma;
        // ...
    }

    namespace succinct {        // NEW NAMESPACE
        class SuccinctBitVector;
        class RoaringBitmap;     // FUTURE
        class WaveletTree;       // FUTURE
        class PackedGraph;       // FUTURE
    }
}
```

### PackedValue Integration

All succinct structures implement the PackedValue concept:

```cpp
template<typename P>
concept PackedValue = requires(const P& p, BitWriter& w, BitReader& r) {
    typename P::value_type;
    { P::encode(p, w) } -> std::same_as<void>;
    { P::decode(r) } -> std::same_as<P>;
    { p.value() } -> std::convertible_to<typename P::value_type>;
};

// SuccinctBitVector is a PackedValue
static_assert(PackedValue<SuccinctBitVector>);

// Can be used in PackedVector
PackedVector<SuccinctBitVector> bitvector_collection;

// Can be used in PackedOptional
PackedOptional<SuccinctBitVector> maybe_bitvec;
```

## Performance Considerations

### Space Overhead

| Structure | Space | Overhead | Comment |
|-----------|-------|----------|---------|
| SuccinctBitVector | n + 0.2%n | 0.2% | Block-based rank |
| RoaringBitmap | Variable | 0-50% | Depends on cardinality |
| InterpolativeCoded | ~log₂(U choose N) | Optimal | Information-theoretic |
| WaveletTree | n log σ + o(n log σ) | ~5% | Includes rank structures |
| PackedGraph (CSR) | ~7 bits/edge | 40-50% | vs 64-bit pointers |

### Time Complexity

| Operation | SuccinctBitVector | RoaringBitmap | WaveletTree | PackedGraph |
|-----------|-------------------|---------------|-------------|-------------|
| Access | O(1) | O(log chunks) | O(log σ) | O(log V + degree) |
| Rank | O(1) | O(log chunks) | O(log σ) | N/A |
| Select | O(log n) | O(log chunks) | O(log σ log n) | N/A |
| Insert | O(n) | O(log chunks + k) | O(n log σ) | O(n) |
| Union | O(n) | O(n + m) | N/A | N/A |

## Testing Strategy

### Unit Tests

```cpp
TEST_CASE("SuccinctBitVector rank operations", "[succinct]") {
    SuccinctBitVector bv(1000);

    SECTION("Empty bit vector") {
        bv.build_rank_support();
        REQUIRE(bv.rank(500) == 0);
    }

    SECTION("Single bit set") {
        bv.set(100);
        bv.build_rank_support();
        REQUIRE(bv.rank(100) == 0);
        REQUIRE(bv.rank(101) == 1);
    }

    SECTION("Random bits") {
        std::mt19937 gen(42);
        std::uniform_int_distribution<size_t> dist(0, 999);

        std::set<size_t> set_positions;
        for (int i = 0; i < 100; ++i) {
            size_t pos = dist(gen);
            bv.set(pos);
            set_positions.insert(pos);
        }

        bv.build_rank_support();

        size_t expected_rank = 0;
        for (size_t i = 0; i < 1000; ++i) {
            if (set_positions.count(i)) ++expected_rank;
            REQUIRE(bv.rank(i + 1) == expected_rank);
        }
    }
}

TEST_CASE("Zero-copy round trip", "[succinct]") {
    SuccinctBitVector bv(100);
    for (size_t i = 0; i < 100; i += 3) {
        bv.set(i);
    }
    bv.build_rank_support();

    uint8_t buffer[1000];
    BitWriter writer(buffer);
    SuccinctBitVector::encode(bv, writer);
    writer.align();

    BitReader reader(buffer, 1000);
    auto bv2 = SuccinctBitVector::decode(reader);

    // Verify identical behavior
    for (size_t i = 0; i <= 100; ++i) {
        REQUIRE(bv.rank(i) == bv2.rank(i));
    }
}
```

### Benchmark Tests

```cpp
BENCHMARK("SuccinctBitVector rank") {
    SuccinctBitVector bv(1000000);
    for (size_t i = 0; i < 1000000; i += 7) {
        bv.set(i);
    }
    bv.build_rank_support();

    return bv.rank(500000);
};

BENCHMARK("VByte encode/decode") {
    uint8_t buffer[100];
    BitWriter writer(buffer);

    for (uint32_t i = 0; i < 1000; ++i) {
        codecs::VByte::encode(i, writer);
    }

    BitReader reader(buffer, 100);
    uint32_t sum = 0;
    for (uint32_t i = 0; i < 1000; ++i) {
        sum += codecs::VByte::decode<uint32_t>(reader);
    }

    return sum;
};
```

## Roadmap

### Phase 1: Quick Wins ✓ COMPLETED
- [x] VByte codec
- [x] Exponential-Golomb family
- [x] Elias Omega
- [x] Comprehensive tests
- [x] Documentation

### Phase 2: Foundation (IN PROGRESS)
- [x] Succinct bit vector API design
- [x] Block-based rank support
- [ ] Complete implementation
- [ ] Comprehensive testing
- [ ] Performance benchmarks

### Phase 3: Core Structures (NEXT)
- [ ] Roaring bitmaps
- [ ] Interpolative coding
- [ ] Wavelet trees
- [ ] Packed graphs

### Phase 4: Advanced Features
- [ ] Compressed suffix arrays
- [ ] FM-index
- [ ] Compressed text indices
- [ ] Spatial data structures

### Phase 5: Optimization
- [ ] SIMD acceleration
- [ ] Multi-threaded construction
- [ ] GPU support
- [ ] Memory mapping utilities

## References

### Papers
1. Jacobson, G. (1989). "Space-efficient static trees and graphs"
2. Clark, D. (1996). "Compact Pat Trees"
3. Navarro, G. & Mäkinen, V. (2007). "Compressed full-text indexes"
4. Chambi, S. et al. (2016). "Better bitmap performance with Roaring bitmaps"
5. Moffat, A. & Stuiver, L. (2000). "Binary interpolative coding"

### Libraries (for inspiration, not copying)
- SDSL-lite: Succinct Data Structure Library
- Roaring: Roaring bitmaps reference implementation
- Elias-Fano: Various implementations
- Wavelet trees: Multiple implementations

### Standards
- Protocol Buffers varint encoding (VByte)
- H.264 video standard (Exp-Golomb)

## Conclusion

The PFC succinct structures design leverages the library's unique zero-copy architecture to provide:

1. **Instant loading**: No parsing step
2. **Memory efficiency**: One representation for storage and queries
3. **Performance**: Cache-friendly, theoretically optimal algorithms
4. **Composability**: Build complex structures from simple primitives
5. **Familiarity**: STL-adjacent interfaces

The three new codecs (VByte, ExpGolomb, EliasOmega) are implemented and tested, providing a solid foundation for the succinct structures to come.

**Next Steps**:
1. Complete SuccinctBitVector implementation
2. Add comprehensive tests and benchmarks
3. Begin Roaring bitmap implementation
4. Document usage patterns and best practices
