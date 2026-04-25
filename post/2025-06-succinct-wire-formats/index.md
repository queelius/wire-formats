---
title: "Succinct Bit Vectors and Rank/Select"
date: 2025-06-22
draft: true
tags:
- C++
- information-theory
- data-structures
- succinct
- rank-select
categories:
- Computer Science
- Mathematics
series:
- wire-formats
series_weight: 11
math: true
description: "A bit vector with O(1) rank and O(log n) select using only n + o(n) bits of space. The auxiliary index is asymptotically negligible while enabling constant-time queries."
linked_project:
- pfc
- wire-formats
---
*The claim that drives this post: store $n$ bits and answer prefix-count queries in $O(1)$ time, using only $n + o(n)$ bits total. The auxiliary index is asymptotically negligible. That is not obvious, and it is worth understanding why it holds.*

## Constant-Time Queries on Bit Vectors

Posts 1 through 10 of this series focused on encoding: universal codes, arithmetic coding, Huffman, LZ77. They all ask the same question in slightly different ways: how do we turn a sequence into a compact bit string? This post shifts direction. The question here is different: once we have a bit vector, how do we query it efficiently without expanding it?

The two fundamental queries on a bit vector of $n$ bits are:

- **rank$_1(i)$**: the number of 1-bits in positions $[0, i)$, i.e., strictly before position $i$.
- **select$_1(j)$**: the position of the $j$-th 1-bit (0-indexed).

These appear throughout data structures: inverted indexes, compressed graphs, FM-indexes, wavelet trees. Rank tells you how many elements precede a position. Select inverts it.

Three design points exist along the space-time axis:

| Approach | Space | rank time | select time |
|---|---|---|---|
| Naive scan | $n$ bits | $O(n/64)$ | $O(n/64)$ |
| Full lookup table | $O(n \log n)$ bits | $O(1)$ | $O(1)$ |
| Succinct (this post) | $n + o(n)$ bits | $O(1)$ | $O(\log n)$ |

The succinct approach hits the right trade-off: an auxiliary index that is asymptotically negligible (sublinear, so $o(n)$) while buying constant-time rank. Select costs $O(\log n)$ with a binary search. Getting $O(1)$ select requires one more index structure; that is covered briefly at the end and implemented in PFC's production version.

## The Structure

The bit vector stores its $n$ bits packed into 64-bit words, LSB-first. Bit $i$ lives at position $i \bmod 64$ within word $\lfloor i / 64 \rfloor$. Unused bits in the final word are zeroed.

The auxiliary index has two levels:

- **Superblock array**: one `uint64_t` per 4096-bit chunk. Entry $s$ holds the absolute cumulative rank from bit 0 to the start of superblock $s$.
- **Block array**: one `uint16_t` per 64-bit word. Entry $b$ holds the rank within the enclosing superblock, from the superblock's start to the start of block $b$.

```cpp
class SuccinctBitVector {
public:
    explicit SuccinctBitVector(const std::vector<bool>& bits);
    [[nodiscard]] std::size_t size()  const noexcept;
    [[nodiscard]] bool        bit(std::size_t i)  const noexcept;
    [[nodiscard]] std::size_t rank1(std::size_t i) const noexcept;   // O(1)
    [[nodiscard]] std::size_t select1(std::size_t j) const noexcept; // O(log n)

protected:
    std::size_t n_;
    std::vector<uint64_t> bits_;
    std::vector<uint64_t> superblock_ranks_;  // abs. rank at superblock boundaries
    std::vector<uint16_t> block_ranks_;       // superblock-relative rank per block

    static constexpr std::size_t SUPERBLOCK_BITS = 4096;
    static constexpr std::size_t BLOCK_BITS      = 64;
    static constexpr std::size_t BLOCKS_PER_SB   = 64;
};
```

The production version lives in [PFC's `include/pfc/succinct.hpp`](https://github.com/queelius/wire-formats/tree/master/lib/pfc). The pedagogical version in this post strips the production features to the core structure.

## Why Constant Time

A rank query for position $i$ proceeds in three steps:

1. **Superblock lookup**: $s = \lfloor i / 4096 \rfloor$. Load `superblock_ranks_[s]`, giving the absolute count of 1-bits before superblock $s$.
2. **Block lookup**: $b = \lfloor i / 64 \rfloor$. Load `block_ranks_[b]`, giving the count within superblock $s$ before block $b$.
3. **Word popcount**: mask the word at position $b$ to keep only bits $0 \ldots (i \bmod 64) - 1$, then call `std::popcount`.

```cpp
std::size_t rank1(std::size_t i) const noexcept {
    if (i == 0) return 0;
    std::size_t sb    = i / SUPERBLOCK_BITS;
    std::size_t blk   = i / BLOCK_BITS;
    std::size_t w_off = i % BLOCK_BITS;
    std::size_t res   = superblock_ranks_[sb] + block_ranks_[blk];
    if (w_off > 0) {
        uint64_t mask = (uint64_t{1} << w_off) - 1;
        res += std::popcount(bits_[blk] & mask);
    }
    return res;
}
```

All three steps are $O(1)$: two array indexing operations and one hardware-accelerated popcount (POPCNT on x86-64, VCNT on ARM). The key insight is that partial sums are precomputed at two granularities. The superblock array handles the large-scale prefix sum; the block array handles the within-superblock refinement. The final popcount handles the within-word residual. Three lookups, total.

## Select via Binary Search

Select is harder than rank. Rank has a natural decomposition: superblock boundary, then block boundary, then word bits. Select does not. The $j$-th 1-bit could be anywhere, and without additional precomputed information, there is no shortcut.

The approach here uses binary search over `superblock_ranks_` to find which superblock contains the $j$-th 1-bit (the largest $s$ such that `superblock_ranks_[s] <= j`). Within that superblock, a linear scan over the block array pinpoints the right block. Within that block, iterating over set bits via repeated `__builtin_ctzll` (count trailing zeros) finds the exact position.

Complexity: $O(\log(n / 4096))$ for the binary search over superblocks, then $O(1)$ per block within the superblock (at most 64 blocks), then $O(64)$ for the bit scan. Total: $O(\log n)$.

The $O(1)$ select extension stores one additional array: a "select samples" array holding the position of every $(\log^2 n)$-th 1-bit. That compresses the binary-search range and allows $O(1)$ select at the cost of a few extra kilobytes for typical $n$. PFC's production version implements this. For the pedagogical version here, $O(\log n)$ select is sufficient.

## Where Succinct Bit Vectors Show Up

Inverted indexes in full-text search use bit vectors to represent posting lists (which documents contain a term). Rank and select over those bit vectors power gap compression: the posting list stores gaps between document IDs, and select recovers the original IDs.

The FM-index, used in short-read DNA alignment tools like BWA, builds a rank-queryable bit vector over the Burrows-Wheeler Transform. Each rank query takes $O(1)$ and drives an $O(m)$ pattern-search algorithm for a pattern of length $m$.

Compressed graphs (as in the WebGraph framework) store adjacency lists as bit vectors with succinct structures for traversal. Wavelet trees use a hierarchy of bit vectors to answer range-frequency queries in $O(\log \sigma)$ where $\sigma$ is the alphabet size.

In all of these, the $n + o(n)$ space guarantee is not academic. The index must not dwarf the data it queries.

## The Space-Time Trade

The auxiliary index for a 10000-bit vector costs about 338 bytes: 24 bytes for the superblock array (3 entries at 8 bytes each) and 314 bytes for the block array (157 entries at 2 bytes each). The bit vector itself costs 1256 bytes (10000 / 8, rounded up to the nearest word). The index is roughly 27% of the bit vector, comfortably $o(n)$.

Here is the comparison table across the four main approaches:

| Representation | Space | rank | select | Best for |
|---|---|---|---|---|
| Plain scan | $n$ bits | $O(n/64)$ | $O(n/64)$ | Very small $n$ |
| Full table | $O(n \log n)$ bits | $O(1)$ | $O(1)$ | Tiny $n$, memory to burn |
| Succinct (this post) | $n + o(n)$ bits | $O(1)$ | $O(\log n)$ | Large dense bit vectors |
| Sparse / RLE | $O(k)$ bits for $k$ ones | $O(\log k)$ | $O(\log k)$ | Very sparse sets |

The sparse / RLE row bridges to the next post. When a bit vector is extremely sparse, storing only the positions of the 1-bits beats the $n + o(n)$ dense approach. RoaringBitmap, covered in [post 12](/post/2025-12-roaring-bitmap-wire-formats/), chooses dynamically between an array, a dense bit vector, and a run-length encoding based on the local density of each 64K-integer chunk.

## Cross-References

This post builds on two earlier posts:

- [Post 1](/post/2021-06-kraft-wire-formats/) established the Kraft lower bound: any lossless code must use at least $H$ bits per symbol. The succinct structure respects this by using $n + o(n)$ bits for an $n$-bit vector, asymptotically optimal.
- [Post 3](/post/2022-01-priors-wire-formats/) showed that a coding choice is implicitly a prior over the data. The choice of a dense bit vector (as opposed to an array or run-length encoding) reflects a prior that 1-bits are distributed roughly uniformly. When that prior is wrong, a different structure wins.

The next post shows how RoaringBitmap resolves this by refusing to commit to a single prior.

---

*Footnote: PFC's `include/pfc/succinct.hpp` implements the full `SuccinctBitVector` with O(1) select via a select-samples index, and also includes `RoaringBitmap` with three container types. The pedagogical code in this post covers the core rank structure and O(log n) select.*
