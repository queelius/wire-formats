---
title: "VByte / Varint"
date: 2024-02-25
draft: false
tags:
- C++
- information-theory
- coding-theory
- prefix-free
- universal-codes
- vbyte
- varint
- byte-aligned
categories:
- Computer Science
- Mathematics
series:
- wire-formats
series_weight: 8
math: true
description: "VByte trades bit-level precision for byte-alignment, and that trade wins in practice. Most production columnar databases and network protocols use VByte for integer encoding."
linked_project:
- pfc
- wire-formats
---

*Every code in this series so far operates at bit granularity. VByte does not. It gives up bit-level precision for byte-alignment, and in production systems, that trade wins most of the time.*

## The Practical Question

Every code in this series so far operates at bit granularity. Elias gamma encodes 1 in a single bit. Fibonacci coding uses exactly as many bits as the Zeckendorf representation requires. Bit packing is theoretically attractive because it minimizes the number of bits written, which minimizes the encoded size.

But bit packing is computationally expensive. Reading or writing a single bit requires a shift, a mask, and often a branch to handle byte boundaries. Encoding a sequence of integers this way burns CPU cycles that scale with the number of integers, independent of their values. For high-throughput applications, the overhead of bit manipulation can easily exceed the savings from compact encoding.

VByte (also called Varint in Google's ecosystem, and LEB128 in the DWARF debug format) trades a small amount of length efficiency for byte-alignment. The idea is simple: encode each integer as a sequence of 7-bit groups, one per byte, with a continuation flag in the high bit of each byte. The result is self-delimiting, compact for small values, and requires no bit-level manipulation to decode.

VByte is the encoding used by Protocol Buffers for all integer fields. It appears in Apache Arrow, Parquet, Snappy's block format, LevelDB's metadata, and most production columnar file formats. These are high-throughput systems. Byte-alignment is why VByte is their choice over the more compact universal codes from posts 4 through 7.

---

## The Encoding

VByte splits an integer into 7-bit groups, starting from the least significant bits. Each group occupies one byte where bits 0 through 6 carry 7 data bits and bit 7 is a continuation flag: `1` means more bytes follow, `0` means this is the last byte.

A value in $[0, 127]$ fits in a single byte (continuation flag clear). A value in $[128, 16383]$ requires two bytes (first byte has flag set, second has flag clear). The pattern continues: each additional byte adds 7 bits of capacity.

```cpp
struct VByte {
    using value_type = std::uint64_t;

    template<BitSink S>
    static void encode(value_type n, S& sink) {
        while (n >= 128) {
            std::uint8_t byte = static_cast<std::uint8_t>((n & 0x7F) | 0x80);
            for (int i = 0; i < 8; ++i) sink.write(((byte >> i) & 1) != 0);
            n >>= 7;
        }
        std::uint8_t byte = static_cast<std::uint8_t>(n & 0x7F);
        for (int i = 0; i < 8; ++i) sink.write(((byte >> i) & 1) != 0);
    }

    template<BitSource S>
    static value_type decode(S& source) {
        value_type result = 0;
        std::size_t shift = 0;
        while (true) {
            std::uint8_t byte = 0;
            for (int i = 0; i < 8; ++i)
                if (source.read()) byte |= static_cast<std::uint8_t>(1 << i);
            result |= static_cast<value_type>(byte & 0x7F) << shift;
            if ((byte & 0x80) == 0) break;
            shift += 7;
        }
        return result;
    }
};
```

**Aside:** This implementation routes each byte LSB-first through a bit-level sink, for consistency with the rest of the series. Real VByte implementations do not use a bit sink at all: they write bytes directly to a buffer. A production VByte encoder is a tight loop over `byte = (n & 0x7F) | 0x80; *ptr++ = byte; n >>= 7;`, terminating with `*ptr++ = n`. No bit shuffling, no branching on byte boundaries. The bit-level wrapper here exists only to keep the pedagogical interface uniform.

---

## The Implied Prior

VByte's length function is:

$$L(n) = 8 \cdot \left\lceil \frac{\lfloor \log_2(n+1) \rfloor + 1}{7} \right\rceil \text{ bits}$$

with a minimum of 8 bits. Equivalently, the number of bytes is $\lceil \text{bit\_width}(n) / 7 \rceil$.

Within each byte-length tier, all values have the same encoded length:
- 1 byte (8 bits): $n \in [0, 127]$, all 128 values.
- 2 bytes (16 bits): $n \in [128, 16383]$, all 16256 values.
- 3 bytes (24 bits): $n \in [16384, 2097151]$, and so on.

This "step-uniform" distribution is VByte's implied prior: within each tier, it treats all values as equally likely. VByte is optimal for sources where the byte-length of each value is geometrically distributed, with most values falling in the first few tiers.

Real-world integer distributions for database columns, network port numbers, document lengths, and similar data tend to be heavily skewed toward small values. For these sources, VByte is competitive: most values land in the 1-byte or 2-byte tier, so average encoded length stays close to the minimum.

VByte does not adapt within a tier. If your values cluster heavily below 64 (half the 1-byte range), VByte wastes the unused bits in each byte. In that case, Elias gamma would be more efficient. The trade is explicit: VByte gives up intra-tier precision for byte-alignment.

---

## Length Comparison

How does VByte compare to the universal codes from earlier posts? The table below shows codeword lengths in bits for selected values:

| $n$    | VByte | Elias Gamma | Elias Delta |
|--------|-------|-------------|-------------|
| 1      | 8     | 1           | 1           |
| 100    | 8     | 13          | 11          |
| 1,000  | 16    | 19          | 16          |
| $2^{20}$ | 24  | 41          | 29          |
| $2^{32}$ | 40  | 65          | 43          |

For small values (1 to 127), VByte is much worse than Gamma or Delta: 8 bits vs 1 bit for $n = 1$. Byte-alignment is expensive at the small end.

For medium values (around 1000), VByte is competitive with Delta. For large values, VByte's growth rate is $8 \lceil \log_2(n) / 7 \rceil \approx 1.14 \log_2(n)$, while Gamma grows as $2 \log_2(n)$ and Delta grows as approximately $\log_2(n) + 2\log_2(\log_2(n))$. For large $n$, VByte is actually more compact than Gamma and not far behind Delta.

The pattern: for very small values, use Gamma or a short fixed-width code. For skewed data with occasional large values, VByte often beats everything else once you factor in decoding speed.

---

## Why It Wins in Practice

VByte's advantage is not in bits-per-symbol. It is in decoding throughput.

A bit-level Elias gamma decoder must maintain a bit offset within a byte, mask bits across byte boundaries, and branch on every symbol. A VByte decoder reads whole bytes, uses a single `& 0x7F` to extract 7 data bits, and checks a single flag bit. The inner loop is:

```c
// Production VByte decoder (byte-level, not bit-level):
while (byte & 0x80) {
    result |= (byte & 0x7F) << shift;
    shift += 7;
    byte = *ptr++;
}
result |= byte << shift;
```

Modern CPUs execute this at cache-fill bandwidth: around 1 to 3 bytes per clock cycle. For a stream of small values where most decode as a single byte, VByte decoding saturates L1 cache bandwidth.

The SIMD VByte decoders (such as the ones in Daniel Lemire's library, implemented with AVX2) process 128 or 256 bits at a time by predicting lengths from the continuation bits, shuffling bytes, and accumulating results in vector registers. These decoders achieve several GB/s of throughput on modern hardware.

Bit-level Gamma or Delta decoders, by contrast, typically top out around 100 to 200 MB/s because each symbol requires multiple shifts and masks that cannot easily be vectorized.

---

## The Engineering Trade

VByte is the engineer's compromise: theoretically suboptimal, practically dominant. This is not a tension to resolve; it is a fact to internalize.

The information-theoretic case says Elias gamma is more efficient for small values, Elias delta is more efficient for a wide range of values, and Rice coding is more efficient when the distribution is geometric and you know the mean. All of these statements are true. In bits per symbol, VByte loses.

The engineering analysis says the constant factor of bit-vs-byte operations swamps the small length savings for high-throughput workloads. A gamma decoder that saves 3 bits per integer but decodes at 150 MB/s is slower in wall-clock time than a VByte decoder that wastes 5 bits per integer but decodes at 3 GB/s. Byte-aligned and theoretically suboptimal beats bit-optimal and slow. This is not a coincidence; it is why byte-alignment exists.

This pattern recurs throughout systems engineering. A theoretically optimal sort requires $O(n \log n)$ comparisons; radix sort uses $O(nk)$ operations and often wins in practice because $k$ is fixed and the constant factor is small. VByte is the radix sort of integer compression.

The lesson is not that theory is wrong. It is that the relevant model for engineering decisions includes hardware constants, cache effects, and parallelism, not just asymptotic bit counts.

---

## Cross-references and Footnotes

**Next in series:** Post 9 covers Huffman coding (forthcoming). Huffman shifts from universal codes to entropy-optimal codes: given a known source distribution, Huffman constructs the optimal prefix-free code. Unlike Rice/Golomb (which assume a specific parametric family) or VByte (which ignores the distribution), Huffman uses the actual probabilities.

**Earlier in series:**
- [Unary and Elias Gamma](/post/2022-06-elias-gamma-wire-formats/) (post 4): Gamma is the bit-level alternative to VByte for sources with heavy skew toward small values.
- [Elias Delta and Omega](/post/2022-11-elias-delta-omega-wire-formats/) (post 5): Delta is more bit-efficient than VByte for large values, at the cost of decoding complexity.
- [Fibonacci Coding](/post/2023-04-fibonacci-wire-formats/) (post 6): Fibonacci codes also encode one integer per read without byte alignment, using a very different structural property.
- [Rice / Golomb](/post/2023-09-rice-golomb-wire-formats/) (post 7): for geometrically distributed data with known mean, Rice outperforms VByte in bits-per-symbol, but VByte wins in decoding speed.

**Cross-series:** Byte-alignment is orthogonal to the type-algebra story in the Stepanov bridge posts. VByte's byte structure does not compose with the `Either`/`Product` algebraic codec combinators in any natural way; it is a flat encoding designed for performance, not algebraic regularity.

**Production code:** The `VByte` struct in this post matches the algorithmic structure of the production implementation in [PFC](https://github.com/queelius/wire-formats/tree/master/lib/pfc): see `include/pfc/codecs.hpp`. The PFC version integrates with `BitReader`/`BitWriter` infrastructure from `core.hpp` in the same bit-level style used here. For the SIMD-optimized byte-level version used in production databases, see Google's protobuf-cpp or Daniel Lemire's `FastPFOR` and `streamvbyte` libraries.
