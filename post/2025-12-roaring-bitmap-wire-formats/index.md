---
title: "RoaringBitmap"
date: 2025-12-07
draft: true
tags:
- C++
- data-structures
- compressed-bitmaps
- roaring
- polyalgorithm
categories:
- Computer Science
- Mathematics
series:
- wire-formats
series_weight: 12
math: true
description: "A hybrid compressed bitmap that picks the optimal sub-representation (array, bitmap, or run-length) per 64K-integer chunk based on density. No single prior dominates: Roaring commits to none and adapts per chunk."
linked_project:
- pfc
- wire-formats
---

## Hybrid Representation as Polyalgorithm

A compressed integer set faces a fundamental tradeoff: the right representation depends on the density of the data. Four density regimes each have a natural winner:

- **Very sparse** (fewer than ~4096 elements in a 64K range): a sorted array of 16-bit values. Each element costs 2 bytes; 4096 elements cost 8 KB.
- **Moderate to dense** (more than ~4096 elements): a dense bit vector, 8 KB for the full 64K range. Contains-check is O(1) via bit indexing.
- **Very dense** (almost full): a sorted array of the absent elements, using the same 2 bytes per gap.
- **Clustered runs** (many consecutive values): run-length encoding. A run of $k$ consecutive integers costs one 4-byte record regardless of $k$.

No single prior dominates across all four regimes. RoaringBitmap (Lemire et al., 2014) is the hybrid answer: partition the 32-bit integer space into 64K-element chunks (indexed by the high 16 bits of each value), then assign each chunk the optimal container based on its actual density. The structure adapts per chunk, not globally.

## The Three Container Types

Every 32-bit integer $v$ maps to chunk-id $v \gg 16$ and low bits $v \mathbin{\&} 0\text{xFFFF}$. Each chunk stores its low bits in one of three containers:

```cpp
class ArrayContainer  { std::vector<uint16_t> elems_;   /* sorted, unique */ };
class BitmapContainer { std::vector<uint64_t> words_;   /* 1024 words = 8 KB */ };
class RunContainer    { std::vector<std::pair<uint16_t,uint16_t>> runs_; /* (start, len-1) */ };

using ContainerVariant = std::variant<ArrayContainer, BitmapContainer, RunContainer>;

class RoaringBitmap {
    std::map<uint16_t, ContainerVariant> chunks_;
public:
    void add(uint32_t v);
    bool contains(uint32_t v) const noexcept;
    std::size_t cardinality() const noexcept;
    void optimize();
    RoaringBitmap union_with(const RoaringBitmap&) const;
    RoaringBitmap intersection_with(const RoaringBitmap&) const;
    RoaringBitmap difference(const RoaringBitmap&) const;
};
```

`ArrayContainer` uses a sorted `std::vector<uint16_t>`. Contains-check is binary search, O(log n). Add is sorted insertion, O(n) worst case but O(1) amortized for sequential loads. Space: 2 bytes per element.

`BitmapContainer` stores 1024 `uint64_t` words covering all 65536 possible values. Contains-check is O(1): `(words_[v/64] >> (v%64)) & 1`. Add is O(1). Cardinality requires scanning all 1024 words with popcount, which is fast in practice. Space: fixed 8 KB regardless of occupancy.

`RunContainer` stores runs as (start, length-1) pairs sorted by start. A run `(s, l)` covers values $s$ through $s+l$ inclusive, using 4 bytes. Space grows with the number of runs, not the number of values. One million consecutive integers fit in one 4-byte record.

The threshold between array and bitmap is 4096 elements: at that point both cost 8 KB, making array-to-bitmap conversion space-neutral. Above 4096, bitmap wins on both space and access time.

## The Conversion Logic

When a chunk crosses the array-to-bitmap threshold, it converts automatically on the next `add`:

```cpp
void add(uint32_t v) {
    uint16_t cid = v >> 16, low = v & 0xFFFF;
    auto it = chunks_.find(cid);
    if (it == chunks_.end()) { chunks_.emplace(cid, ArrayContainer{}); it = chunks_.find(cid); }

    std::visit([&](auto& c) {
        using T = std::decay_t<decltype(c)>;
        if constexpr (std::is_same_v<T, ArrayContainer>) {
            c.add(low);
            if (c.cardinality() > ARRAY_MAX) it->second = array_to_bitmap(c);  // convert
        } else if constexpr (std::is_same_v<T, BitmapContainer>) {
            c.add(low);
        } else { /* RunContainer: rebuild via bitmap */ }
    }, it->second);
}
```

The conversion functions are straightforward:

```cpp
BitmapContainer array_to_bitmap(const ArrayContainer& a) {
    BitmapContainer b;
    for (uint16_t v : a.elements()) b.add(v);
    return b;
}

ArrayContainer bitmap_to_array(const BitmapContainer& b) {
    ArrayContainer a;
    for (std::size_t w = 0; w < b.raw_words().size(); ++w) {
        uint64_t word = b.raw_words()[w];
        while (word) { a.add(w*64 + __builtin_ctzll(word)); word &= word-1; }
    }
    return a;
}
```

Run conversion is explicit, called via `optimize()` after bulk-loading:

```cpp
void optimize() {
    for (auto& [cid, var] : chunks_)
        std::visit([&var](const auto& c) {
            using T = std::decay_t<decltype(c)>;
            if constexpr (std::is_same_v<T, ArrayContainer>)  var = array_to_run(c);
            else if constexpr (std::is_same_v<T, BitmapContainer>) var = bitmap_to_run(c);
        }, var);
}
```

## The Operations

Set operations work chunk-by-chunk. Each chunk dispatches on its container type via `std::visit`. Union iterates over the other bitmap's chunks and adds each element into the result. Intersection only considers chunks present in both bitmaps. Difference keeps elements from the first bitmap not present in the second.

```cpp
RoaringBitmap union_with(const RoaringBitmap& other) const {
    RoaringBitmap result = *this;
    for (const auto& [cid, var] : other.chunks_) {
        std::visit([&](const auto& c) {
            using T = std::decay_t<decltype(c)>;
            if constexpr (std::is_same_v<T, ArrayContainer>) {
                for (uint16_t v : c.elements())
                    result.add(uint32_t(cid) << 16 | v);
            } else if constexpr (std::is_same_v<T, BitmapContainer>) {
                // iterate set bits via __builtin_ctzll, add each
            } else {
                // iterate runs, add each value in range
            }
        }, var);
    }
    return result;
}
```

Each chunk's operation is `std::visit` dispatching to the algorithm that matches the container type. No single code path handles all cases; the type system ensures each case is explicit. This is compile-time polymorphism without virtual dispatch.

## Why It Wins

RoaringBitmap has been measured to compress 50 to 90 percent versus uncompressed bit arrays in typical workloads, and 2 to 5 times better than EWAH (Enhanced Word-Aligned Hybrid) and CONCISE, which are earlier competing bitmap compression formats.

The reasons:

- **Adaptive**: each chunk independently picks the right encoding. A bitmap with one dense chunk and many sparse chunks does not pay the 8 KB overhead for every chunk.
- **Fast operations**: union and intersection iterate only over present chunks. A chunk absent in either operand contributes nothing to the result.
- **Composable**: the outer API (add, contains, cardinality, union, intersection, difference) is independent of the per-chunk representation. Callers need not know which container type is active.
- **Predictable**: the threshold (ARRAY_MAX = 4096) is fixed and documented. Performance is predictable from the input density.

Real-world adoption: Apache Lucene uses RoaringBitmap for posting lists. Apache Druid and ClickHouse use it for filter bitmaps. The reference Java implementation (RoaringBitmap library by Lemire, Kaser, et al.) has tens of millions of downloads.

## The Polyalgorithm Pattern

The pattern here has a name: polyalgorithm. A single interface dispatches to the algorithm optimal for the current input characteristics. The decision is made at runtime based on measured data properties (chunk density), not by the caller.

Other examples of the same pattern:

- `std::sort` dispatches to insertion sort for small ranges, introsort (quicksort with heapsort fallback) for larger ranges.
- Hash table resize policies dispatch between linear probing, robin-hood hashing, and open addressing based on load factor.
- Compiler optimization pipelines dispatch between O0, O1, O2, O3 passes based on function size and loop structure.

The connection to this series: each container type in RoaringBitmap is the right choice for a specific prior over chunk density. The array container assumes density is low; the bitmap assumes moderate to high; the run container assumes clustering. RoaringBitmap is the result of not committing to a single prior and instead measuring the data and choosing per chunk.

Post 3 of this series showed that a coding choice is a prior. RoaringBitmap makes that explicit: it maintains three priors simultaneously and applies the matching one per chunk. The synthesis of this idea, in post 13, will close the arc: codecs as structure, structure as the wire format itself.

## Cross-References

- [Post 11](/post/2025-06-succinct-wire-formats/) covered succinct bit vectors with O(1) rank and O(log n) select. The `BitmapContainer` in this post is the dense bit vector from post 11, without the auxiliary index (since we do not need rank/select on individual containers here).
- [Post 3](/post/2022-01-priors-wire-formats/) framed each codec as an implicit prior over the data distribution. RoaringBitmap is the polyalgorithm result of refusing to pick one prior globally.
- Post 13 (Synthesis: Codecs as Structure) will close the arc.

---

*Footnote: PFC's `include/pfc/succinct.hpp` includes the full `RoaringBitmap` implementation with all three container types, conversion functions, and set operations. The pedagogical version in this post covers the same design but omits production details like SIMD-accelerated popcount and memory-pool allocation.*
