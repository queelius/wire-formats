# Algebra over Wire Formats: Sub-sub-project 3c (Posts 7 and 8) Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Ship posts 7 ("Rice / Golomb", 2023-09-17) and 8 ("VByte / Varint", 2024-02-25) of the Algebra over Wire Formats series, including TDD implementations of `rice_golomb.hpp` and `vbyte.hpp`, full GoogleTest suites, prose drafts, and sync to metafunctor.com.

**Architecture:** Two new post directories under `post/` (`2023-09-rice-golomb-wire-formats/` and `2024-02-vbyte-wire-formats/`), each with a header, a test file, and `index.md`. Post 7's `rice_golomb.hpp` defines `Rice<K>` and `Golomb<M>` templates plus the parameter-selection helpers `optimal_rice_k` and `optimal_golomb_m` in `namespace rice_golomb`; the implementations match the spec verbatim and are consistent with the PFC production versions in `include/pfc/codecs.hpp`. Post 8's `vbyte.hpp` defines `VByte` in `namespace vbyte`; its bit-level implementation is pedagogically consistent with the rest of the series, with a prose note explaining real implementations operate byte-directly. Both posts wire into the existing `post/CMakeLists.txt`, update `docs/about.md` and `mkdocs.yml`, and sync to metafunctor via the Makefile `sync` target.

**Tech Stack:** C++23, GoogleTest v1.14.0 (already wired in `post/CMakeLists.txt`), mkdocs, the soul plugin's banned-phrase hook.

---

## Spec reference

See `docs/superpowers/specs/2026-04-24-arc-posts-3-through-13.md`, sections "Post 7: Rice / Golomb" and "Post 8: VByte / Varint" for per-section content guides A through G, code budgets, and prose budgets.

## Cross-references note

Posts 3 through 6 are implemented in sub-sub-projects 3a and 3b. By the time this plan runs, the following live links exist:

- Post 3: `/post/2022-01-priors-wire-formats/`
- Post 4: `/post/2022-06-elias-gamma-wire-formats/`
- Post 5: `/post/2022-11-elias-delta-omega-wire-formats/`
- Post 6: `/post/2023-04-fibonacci-wire-formats/`

Post 7's forward link to post 8 is a live link (both ship together in this plan). Post 8's forward link to post 9 (Huffman) is plain text ("forthcoming"). Both posts back-link to posts 3 through 6 using the live paths above. The priors library at `post/2022-01-priors-wire-formats/priors.hpp` is used by the integration tests in both post 7 and post 8.

The Stepanov bridge posts (`/post/2026-05-codecs-functors-stepanov/` and `/post/2026-05-prefix-free-stepanov/`) were updated in sub-project 2. Post 7's cross-series link points to both bridges; post 8 has no cross-series link (byte-alignment is orthogonal to the type-algebra story).

---

## Task 1: Reconnaissance (date collision check)

**Files:** read-only.

- [ ] **Step 1: Verify dates 2023-09-17 and 2024-02-25 do not collide with existing metafunctor posts**

```bash
grep -h "^date:" /home/spinoza/github/repos/metafunctor/content/post/*/index.md 2>/dev/null \
  | grep -E "^date: 2023-09-17|^date: 2024-02-25" | sort -u
```

Expected: empty output. If any dates collide, pick adjacent unused days and note them before proceeding.

- [ ] **Step 2: Confirm post 7 and post 8 directories do not yet exist**

```bash
ls /home/spinoza/github/metafunctor-series/wire-formats/post/ | grep -E "2023-02|2023-08"
```

Expected: no output (neither directory exists yet).

- [ ] **Step 3: Confirm the existing CMakeLists ends with the most recent post's block (so append is safe)**

```bash
tail -8 /home/spinoza/github/metafunctor-series/wire-formats/post/CMakeLists.txt
```

Expected: the last `add_test` line belongs to the most recently added post (Fibonacci, from sub-sub-project 3b). No commit for this task.

---

## Task 2: Scaffold post 7 directory and wire CMakeLists

**Files:**
- Create: `post/2023-09-rice-golomb-wire-formats/index.md`
- Create: `post/2023-09-rice-golomb-wire-formats/rice_golomb.hpp`
- Create: `post/2023-09-rice-golomb-wire-formats/test_rice_golomb.cpp`
- Modify: `post/CMakeLists.txt`

- [ ] **Step 1: Create the post 7 directory and skeleton files**

```bash
mkdir -p /home/spinoza/github/metafunctor-series/wire-formats/post/2023-09-rice-golomb-wire-formats
```

Create `post/2023-09-rice-golomb-wire-formats/index.md` with placeholder frontmatter:

```markdown
---
title: "Rice / Golomb"
date: 2023-09-17
draft: true
tags:
- C++
- information-theory
- coding-theory
- prefix-free
- universal-codes
- rice
- golomb
- geometric-distribution
categories:
- Computer Science
- Mathematics
series:
- wire-formats
series_weight: 7
math: true
description: "Rice and Golomb codes are parametric: a single parameter k (or m) tunes the code to a specific geometric distribution. Choosing k is choosing your prior precisely."
linked_project:
- pfc
- wire-formats
---

(Draft in progress. See plan Task 8 for full prose.)
```

Create `post/2023-09-rice-golomb-wire-formats/rice_golomb.hpp` with header guards only:

```cpp
// rice_golomb.hpp
// Pedagogical implementation for the post "Rice / Golomb" in the
// "Algebra over Wire Formats" series. For the production version, see PFC:
// https://github.com/queelius/pfc (codecs.hpp: Rice<K>, Golomb<M>)

#pragma once

#include <bit>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace rice_golomb {

// BitSink and BitSource concepts (minimal local definitions for self-contained
// pedagogical use; production code uses pfc/core.hpp).

template<typename S>
concept BitSink = requires(S& s, bool b) {
    { s.write(b) } -> std::same_as<void>;
};

template<typename S>
concept BitSource = requires(S& s) {
    { s.read() } -> std::same_as<bool>;
};

// Implementation arrives in Tasks 3, 4, and 5.

}  // namespace rice_golomb
```

Create `post/2023-09-rice-golomb-wire-formats/test_rice_golomb.cpp` with a placeholder test:

```cpp
#include <gtest/gtest.h>
#include "rice_golomb.hpp"

TEST(RiceGolombTest, Placeholder) {
    EXPECT_TRUE(true);
}
```

- [ ] **Step 2: Append the post 7 test executable to post/CMakeLists.txt**

Append to `/home/spinoza/github/metafunctor-series/wire-formats/post/CMakeLists.txt`:

```cmake

# =============================================================================
# Rice / Golomb (post 7, 2023-09-17)
# =============================================================================
add_executable(test_rice_golomb 2023-09-rice-golomb-wire-formats/test_rice_golomb.cpp)
target_link_libraries(test_rice_golomb GTest::gtest_main)
target_include_directories(test_rice_golomb PRIVATE 2023-09-rice-golomb-wire-formats)
add_test(NAME test_rice_golomb COMMAND test_rice_golomb)
```

- [ ] **Step 3: Build and verify placeholder test passes**

```bash
cd /home/spinoza/github/metafunctor-series/wire-formats && make test 2>&1 | tail -12
```

Expected: all existing tests pass plus the new `test_rice_golomb.Placeholder` test. Output should include `[  PASSED  ] 1 test.` for test_rice_golomb.

- [ ] **Step 4: Commit**

```bash
cd /home/spinoza/github/metafunctor-series/wire-formats
git add post/2023-09-rice-golomb-wire-formats post/CMakeLists.txt
git commit -m "scaffold(rice-golomb): add post 7 directory and CMake wiring"
```

---

## Task 3: TDD -- implement `Rice<K>` codec

**Files:**
- Modify: `post/2023-09-rice-golomb-wire-formats/rice_golomb.hpp`
- Modify: `post/2023-09-rice-golomb-wire-formats/test_rice_golomb.cpp`

- [ ] **Step 1: Write failing tests for `Rice<K>`**

Replace `test_rice_golomb.cpp` with:

```cpp
#include <gtest/gtest.h>
#include <cstdint>
#include <vector>
#include "rice_golomb.hpp"

using namespace rice_golomb;

// Minimal in-memory BitSink/BitSource for tests.
struct BitBuffer {
    std::vector<bool> bits;
    std::size_t pos_ = 0;
    void write(bool b) { bits.push_back(b); }
    bool read() { bool b = bits[pos_]; ++pos_; return b; }
};

// Helper: encode then decode n via Rice<K>, check round-trip.
template<std::size_t K>
static std::uint64_t rice_round_trip(std::uint64_t n) {
    BitBuffer buf;
    Rice<K>::encode(n, buf);
    buf.pos_ = 0;
    return Rice<K>::decode(buf);
}

// Helper: return the bit count emitted for Rice<K>(n).
template<std::size_t K>
static std::size_t rice_bit_count(std::uint64_t n) {
    BitBuffer buf;
    Rice<K>::encode(n, buf);
    return buf.bits.size();
}

// Round-trip: Rice<1>
TEST(RiceGolombTest, Rice1RoundTrip) {
    for (std::uint64_t n = 0; n <= 20; ++n) {
        EXPECT_EQ(rice_round_trip<1>(n), n) << "n=" << n;
    }
}

// Round-trip: Rice<2>
TEST(RiceGolombTest, Rice2RoundTrip) {
    for (std::uint64_t n = 0; n <= 40; ++n) {
        EXPECT_EQ(rice_round_trip<2>(n), n) << "n=" << n;
    }
}

// Round-trip: Rice<4>
TEST(RiceGolombTest, Rice4RoundTrip) {
    for (std::uint64_t n = 0; n <= 100; ++n) {
        EXPECT_EQ(rice_round_trip<4>(n), n) << "n=" << n;
    }
}

// Codeword length for Rice<K>(n): should be floor(n/2^K) + 1 + K bits.
// (floor(n >> K) zero bits + one '1' bit + K remainder bits)
TEST(RiceGolombTest, Rice2BitCount) {
    // k=2: length = (n >> 2) + 1 + 2 = (n >> 2) + 3
    for (std::uint64_t n = 0; n <= 30; ++n) {
        std::size_t expected = static_cast<std::size_t>(n >> 2) + 1 + 2;
        EXPECT_EQ(rice_bit_count<2>(n), expected) << "n=" << n;
    }
}

// Spot-check encoding of n=0 with K=2: q=0, r=0 -> "1 00"
TEST(RiceGolombTest, Rice2Encoding0) {
    BitBuffer buf;
    Rice<2>::encode(std::uint64_t{0}, buf);
    ASSERT_EQ(buf.bits.size(), 3u);  // 1 unary bit + 2 remainder bits
    EXPECT_EQ(buf.bits[0], true);   // unary(0+1) = "1"
    EXPECT_EQ(buf.bits[1], false);  // r=0 bit 1
    EXPECT_EQ(buf.bits[2], false);  // r=0 bit 0
}

// Spot-check encoding of n=4 with K=2: q=1, r=0 -> "01 00"
TEST(RiceGolombTest, Rice2Encoding4) {
    BitBuffer buf;
    Rice<2>::encode(std::uint64_t{4}, buf);
    ASSERT_EQ(buf.bits.size(), 5u);  // 2 unary bits + 2 remainder bits
    EXPECT_EQ(buf.bits[0], false);  // leading zero of unary(2)
    EXPECT_EQ(buf.bits[1], true);   // terminating '1' of unary(2)
    EXPECT_EQ(buf.bits[2], false);  // r=0 bit 1
    EXPECT_EQ(buf.bits[3], false);  // r=0 bit 0
}

// Spot-check encoding of n=5 with K=2: q=1, r=1 -> "01 01"
TEST(RiceGolombTest, Rice2Encoding5) {
    BitBuffer buf;
    Rice<2>::encode(std::uint64_t{5}, buf);
    ASSERT_EQ(buf.bits.size(), 5u);
    EXPECT_EQ(buf.bits[0], false);  // leading zero of unary(2)
    EXPECT_EQ(buf.bits[1], true);   // terminating '1' of unary(2)
    EXPECT_EQ(buf.bits[2], false);  // r=1 MSB
    EXPECT_EQ(buf.bits[3], true);   // r=1 LSB
}

// n=0 is valid for Rice (non-negative integers).
TEST(RiceGolombTest, Rice1RoundTripZero) {
    EXPECT_EQ(rice_round_trip<1>(std::uint64_t{0}), std::uint64_t{0});
}

// Large value round-trip.
TEST(RiceGolombTest, Rice4LargeRoundTrip) {
    for (std::uint64_t n : {std::uint64_t{1000}, std::uint64_t{65535}, std::uint64_t{1000000}}) {
        EXPECT_EQ(rice_round_trip<4>(n), n) << "n=" << n;
    }
}
```

- [ ] **Step 2: Run to verify compile failure**

```bash
cd /home/spinoza/github/metafunctor-series/wire-formats && make build 2>&1 | grep -E "error:|undefined" | head -10
```

Expected: compile errors about `Rice` not declared.

- [ ] **Step 3: Implement `Rice<K>` in `rice_golomb.hpp`**

Replace the `// Implementation arrives in Tasks 3, 4, and 5.` comment with:

```cpp
// ---- Rice<K> -- parametric code for geometric distributions -----------------
//
// Encodes non-negative integer n >= 0 by splitting into quotient q = n >> K
// and remainder r = n & ((1 << K) - 1):
//   1. Write q zeros (the unary-coded quotient, offset by 1: unary(q+1) minus
//      the final '1' is q zeros, then a '1').
//   2. Write a '1' bit (the unary terminator).
//   3. Write the K-bit binary representation of r, MSB first.
//
// Codeword examples for K=2 (r is always 2 bits):
//   n=0: q=0, r=0 -> "1 00"    (3 bits)
//   n=1: q=0, r=1 -> "1 01"    (3 bits)
//   n=4: q=1, r=0 -> "01 00"   (4 bits)
//   n=5: q=1, r=1 -> "01 01"   (4 bits)
//
// Codeword length: (n >> K) + 1 + K bits.
// Kraft sum: sum_{q=0}^{inf} 2^K * 2^{-(q+1+K)} = 1 (saturates).
// Implied prior: geometric with rate parameter p = 2^K / (2^K + 1), tuned by K.
// Optimal source: geometric distribution with mean mu satisfying K ~ log2(mu).
//
// K must satisfy 0 < K < 64. The template parameter K is the number of
// remainder bits (equivalently, the divisor is 2^K).

template<std::size_t K>
struct Rice {
    using value_type = std::uint64_t;
    static_assert(K > 0 && K < 64, "K must be in [1, 63]");

    template<BitSink S>
    static void encode(value_type n, S& sink) {
        std::uint64_t q = n >> K;
        std::uint64_t r = n & ((std::uint64_t{1} << K) - 1);
        // Write q zero bits (quotient in unary-stop-bit form).
        for (std::uint64_t i = 0; i < q; ++i) sink.write(false);
        // Write the stop bit.
        sink.write(true);
        // Write the K-bit remainder, MSB first.
        for (std::size_t i = 0; i < K; ++i) {
            sink.write(((r >> (K - 1 - i)) & 1) != 0);
        }
    }

    template<BitSource S>
    static value_type decode(S& source) {
        // Count zero bits to get q.
        std::uint64_t q = 0;
        while (!source.read()) ++q;
        // Read K remainder bits, MSB first.
        std::uint64_t r = 0;
        for (std::size_t i = 0; i < K; ++i) {
            r = (r << 1) | (source.read() ? std::uint64_t{1} : std::uint64_t{0});
        }
        return (q << K) | r;
    }
};
```

- [ ] **Step 4: Build and verify tests pass**

```bash
cd /home/spinoza/github/metafunctor-series/wire-formats && make test 2>&1 | grep -E "PASSED|FAILED|test_rice_golomb"
```

Expected: all RiceGolombTest cases (Rice round-trips, bit counts, spot-checks) pass.

- [ ] **Step 5: Commit**

```bash
cd /home/spinoza/github/metafunctor-series/wire-formats
git add post/2023-09-rice-golomb-wire-formats/rice_golomb.hpp \
        post/2023-09-rice-golomb-wire-formats/test_rice_golomb.cpp
git commit -m "feat(rice-golomb): implement Rice<K> codec (TDD)"
```

---

## Task 4: TDD -- implement `Golomb<M>` codec

**Files:**
- Modify: `post/2023-09-rice-golomb-wire-formats/rice_golomb.hpp`
- Modify: `post/2023-09-rice-golomb-wire-formats/test_rice_golomb.cpp`

- [ ] **Step 1: Append failing tests for `Golomb<M>`**

Append to `test_rice_golomb.cpp`:

```cpp
// Helper: encode then decode n via Golomb<M>, check round-trip.
template<std::size_t M>
static std::uint64_t golomb_round_trip(std::uint64_t n) {
    BitBuffer buf;
    Golomb<M>::encode(n, buf);
    buf.pos_ = 0;
    return Golomb<M>::decode(buf);
}

// Helper: return the bit count emitted for Golomb<M>(n).
template<std::size_t M>
static std::size_t golomb_bit_count(std::uint64_t n) {
    BitBuffer buf;
    Golomb<M>::encode(n, buf);
    return buf.bits.size();
}

// Round-trip: Golomb<1> (degenerate: all values have same length - unary)
TEST(RiceGolombTest, Golomb1RoundTrip) {
    for (std::uint64_t n = 0; n <= 10; ++n) {
        EXPECT_EQ(golomb_round_trip<1>(n), n) << "n=" << n;
    }
}

// Round-trip: Golomb<4> (power-of-2: equivalent to Rice<2>)
TEST(RiceGolombTest, Golomb4RoundTrip) {
    for (std::uint64_t n = 0; n <= 40; ++n) {
        EXPECT_EQ(golomb_round_trip<4>(n), n) << "n=" << n;
    }
}

// Round-trip: Golomb<5> (non-power-of-2 m)
TEST(RiceGolombTest, Golomb5RoundTrip) {
    for (std::uint64_t n = 0; n <= 50; ++n) {
        EXPECT_EQ(golomb_round_trip<5>(n), n) << "n=" << n;
    }
}

// Round-trip: Golomb<7> (non-power-of-2 m)
TEST(RiceGolombTest, Golomb7RoundTrip) {
    for (std::uint64_t n = 0; n <= 70; ++n) {
        EXPECT_EQ(golomb_round_trip<7>(n), n) << "n=" << n;
    }
}

// Golomb<4> and Rice<2> agree (4 = 2^2, so K=2 Rice = M=4 Golomb).
TEST(RiceGolombTest, Golomb4EqualsRice2) {
    for (std::uint64_t n = 0; n <= 30; ++n) {
        EXPECT_EQ(golomb_round_trip<4>(n), rice_round_trip<2>(n)) << "n=" << n;
        EXPECT_EQ(golomb_bit_count<4>(n), rice_bit_count<2>(n)) << "n=" << n;
    }
}

// Spot-check Golomb<5> codeword lengths for n=0..9.
// For m=5: bits = ceil(log2(5)) = 3. cutoff = 2^3 - 5 = 3.
// r < 3: use 2 bits. r >= 3: use 3 bits (with r+3 shifted).
// q = floor(n/5), remainder r = n mod 5.
// Length = q + 1 (unary) + (2 if r<3 else 3).
TEST(RiceGolombTest, Golomb5LengthsTable) {
    // n=0: q=0,r=0 -> len=1+2=3
    // n=1: q=0,r=1 -> len=1+2=3
    // n=2: q=0,r=2 -> len=1+2=3
    // n=3: q=0,r=3 -> len=1+3=4
    // n=4: q=0,r=4 -> len=1+3=4
    // n=5: q=1,r=0 -> len=2+2=4
    // n=9: q=1,r=4 -> len=2+3=5
    EXPECT_EQ(golomb_bit_count<5>(0), 3u);
    EXPECT_EQ(golomb_bit_count<5>(1), 3u);
    EXPECT_EQ(golomb_bit_count<5>(2), 3u);
    EXPECT_EQ(golomb_bit_count<5>(3), 4u);
    EXPECT_EQ(golomb_bit_count<5>(4), 4u);
    EXPECT_EQ(golomb_bit_count<5>(5), 4u);
    EXPECT_EQ(golomb_bit_count<5>(9), 5u);
}

// Large-value round-trip for non-power-of-2 M.
TEST(RiceGolombTest, Golomb5LargeRoundTrip) {
    for (std::uint64_t n : {std::uint64_t{100}, std::uint64_t{999}, std::uint64_t{9999}}) {
        EXPECT_EQ(golomb_round_trip<5>(n), n) << "n=" << n;
    }
}
```

- [ ] **Step 2: Run to verify compile failure**

```bash
cd /home/spinoza/github/metafunctor-series/wire-formats && make build 2>&1 | grep -E "error:|undefined" | head -10
```

Expected: compile errors about `Golomb` not declared.

- [ ] **Step 3: Implement `Golomb<M>` and the truncated-binary helper in `rice_golomb.hpp`**

Append to `rice_golomb.hpp` (after the `Rice` struct, still inside `namespace rice_golomb`):

```cpp
// ---- truncated_binary -- helper for Golomb's remainder encoding --------------
//
// Encodes r in the range [0, m) using the minimal-length prefix-free code:
//   bits = ceil(log2(m)) = bit_width(m - 1)
//   cutoff = 2^bits - m       (number of values that use bits-1 bits)
//
// If r < cutoff: encode r in (bits - 1) bits.
// If r >= cutoff: encode (r + cutoff) in bits bits.
//
// This packs the m values into a prefix-free binary code of length
// floor(log2(m)) or ceil(log2(m)), splitting the two groups so no codeword
// is a prefix of another.

namespace detail {

// Number of bits needed to represent values in [0, m): ceil(log2(m)).
// For m=1 we need 0 bits (only one value). For m=2 we need 1 bit, etc.
inline std::size_t min_bits(std::size_t m) {
    if (m <= 1) return 0;
    return static_cast<std::size_t>(std::bit_width(m - 1));
}

template<BitSink S>
inline void truncated_binary_encode(std::uint64_t r, std::size_t m, S& sink) {
    assert(r < m);
    if (m == 1) return;  // Zero bits needed for a single value.
    std::size_t bits = min_bits(m);
    std::uint64_t cutoff = (std::uint64_t{1} << bits) - static_cast<std::uint64_t>(m);
    if (r < cutoff) {
        // Encode r in (bits - 1) bits, MSB first.
        for (std::size_t i = bits - 1; i > 0; --i) {
            sink.write(((r >> (i - 1)) & 1) != 0);
        }
    } else {
        // Encode (r + cutoff) in bits bits, MSB first.
        std::uint64_t val = r + cutoff;
        for (std::size_t i = bits; i > 0; --i) {
            sink.write(((val >> (i - 1)) & 1) != 0);
        }
    }
}

template<BitSource S>
inline std::uint64_t truncated_binary_decode(std::size_t m, S& source) {
    if (m == 1) return 0;
    std::size_t bits = min_bits(m);
    std::uint64_t cutoff = (std::uint64_t{1} << bits) - static_cast<std::uint64_t>(m);
    // Read (bits - 1) bits first.
    std::uint64_t val = 0;
    for (std::size_t i = 0; i < bits - 1; ++i) {
        val = (val << 1) | (source.read() ? std::uint64_t{1} : std::uint64_t{0});
    }
    if (val < cutoff) {
        // The short codeword: r = val, consumed (bits - 1) bits.
        return val;
    } else {
        // Need one more bit to distinguish.
        val = (val << 1) | (source.read() ? std::uint64_t{1} : std::uint64_t{0});
        return val - cutoff;
    }
}

}  // namespace detail

// ---- Golomb<M> -- generalized Rice for non-power-of-2 divisors --------------
//
// Encodes non-negative integer n >= 0 by splitting into quotient q = n / m
// and remainder r = n % m:
//   1. Write q in unary (q zeros then a '1' bit).
//   2. Write r in truncated binary (prefix-free code over [0, m)).
//
// When M is a power of 2 (M = 2^K), Golomb<M> produces identical codewords
// to Rice<K>. For non-power-of-2 M, the truncated-binary remainder is
// slightly more efficient than a fixed K-bit field.
//
// Codeword length: floor(n/M) + 1 + (floor(log2(M)) or ceil(log2(M))) bits.
// Implied prior: geometric with rate parameter p tuned to mean ~ M / (1 - p).
// Optimal source: geometric distribution with mean close to M.

template<std::size_t M>
struct Golomb {
    using value_type = std::uint64_t;
    static_assert(M >= 1, "M must be at least 1");

    template<BitSink S>
    static void encode(value_type n, S& sink) {
        std::uint64_t q = n / static_cast<std::uint64_t>(M);
        std::uint64_t r = n % static_cast<std::uint64_t>(M);
        // Write q in unary: q zeros then a '1'.
        for (std::uint64_t i = 0; i < q; ++i) sink.write(false);
        sink.write(true);
        // Write r in truncated binary.
        detail::truncated_binary_encode(r, M, sink);
    }

    template<BitSource S>
    static value_type decode(S& source) {
        // Count zero bits to get q.
        std::uint64_t q = 0;
        while (!source.read()) ++q;
        // Decode r from truncated binary.
        std::uint64_t r = detail::truncated_binary_decode(M, source);
        return q * static_cast<std::uint64_t>(M) + r;
    }
};
```

- [ ] **Step 4: Build and verify tests pass**

```bash
cd /home/spinoza/github/metafunctor-series/wire-formats && make test 2>&1 | grep -E "PASSED|FAILED|test_rice_golomb"
```

Expected: all RiceGolombTest cases pass (all Rice tests from Task 3 plus all Golomb tests).

- [ ] **Step 5: Commit**

```bash
cd /home/spinoza/github/metafunctor-series/wire-formats
git add post/2023-09-rice-golomb-wire-formats/rice_golomb.hpp \
        post/2023-09-rice-golomb-wire-formats/test_rice_golomb.cpp
git commit -m "feat(rice-golomb): implement Golomb<M> with truncated-binary remainder (TDD)"
```

---

## Task 5: TDD -- implement `optimal_rice_k` and `optimal_golomb_m`

**Files:**
- Modify: `post/2023-09-rice-golomb-wire-formats/rice_golomb.hpp`
- Modify: `post/2023-09-rice-golomb-wire-formats/test_rice_golomb.cpp`

- [ ] **Step 1: Append failing tests for the parameter-selection functions**

Append to `test_rice_golomb.cpp`:

```cpp
// Tests for optimal_rice_k(mean) and optimal_golomb_m(mean).

// For mean = 1 (almost all values are 0), K=0 would be ideal but K must be >=1;
// optimal_rice_k should return 1.
TEST(RiceGolombTest, OptimalRiceKMeanOne) {
    EXPECT_EQ(optimal_rice_k(1.0), std::size_t{1});
}

// For mean = 2 (geometric with p~0.5), K=1 is optimal.
TEST(RiceGolombTest, OptimalRiceKMeanTwo) {
    EXPECT_EQ(optimal_rice_k(2.0), std::size_t{1});
}

// For mean = 4, K=2 is optimal.
TEST(RiceGolombTest, OptimalRiceKMeanFour) {
    EXPECT_EQ(optimal_rice_k(4.0), std::size_t{2});
}

// For mean = 16, K=4 is optimal.
TEST(RiceGolombTest, OptimalRiceKMeanSixteen) {
    EXPECT_EQ(optimal_rice_k(16.0), std::size_t{4});
}

// optimal_golomb_m returns a positive integer.
TEST(RiceGolombTest, OptimalGolombMPositive) {
    for (double mu : {1.5, 2.0, 5.0, 10.0, 100.0}) {
        EXPECT_GE(optimal_golomb_m(mu), std::size_t{1}) << "mu=" << mu;
    }
}

// For a power-of-2 mean, optimal_golomb_m is close to optimal_rice_k's 2^K.
TEST(RiceGolombTest, OptimalGolombMNearPowerOf2ForPowerMean) {
    // mean=4: optimal Rice K=2, so 2^K=4. Golomb m should be near 4.
    std::size_t m = optimal_golomb_m(4.0);
    EXPECT_GE(m, std::size_t{2});
    EXPECT_LE(m, std::size_t{8});
}

// For non-power-of-2 mean, optimal_golomb_m can differ from any power of 2.
TEST(RiceGolombTest, OptimalGolombMForMeanFive) {
    std::size_t m = optimal_golomb_m(5.0);
    EXPECT_GE(m, std::size_t{1});
    EXPECT_LE(m, std::size_t{20});
}
```

- [ ] **Step 2: Run to verify compile failure**

```bash
cd /home/spinoza/github/metafunctor-series/wire-formats && make build 2>&1 | grep -E "error:|undefined" | head -10
```

Expected: compile errors about `optimal_rice_k` and `optimal_golomb_m` not declared.

- [ ] **Step 3: Implement `optimal_rice_k` and `optimal_golomb_m` in `rice_golomb.hpp`**

Append to `rice_golomb.hpp` (after the `Golomb` struct, still inside `namespace rice_golomb`):

```cpp
// ---- Parameter selection (Gallager and van Voorhis 1975) --------------------
//
// For a geometric distribution with mean mu (i.e., p = 1/mu if the distribution
// is over non-negative integers), the optimal Golomb parameter is approximately:
//
//   m* = -1 / log2((mu - 1) / mu)  =  -1 / log2(1 - 1/mu)
//
// For Rice (which requires m = 2^K), round m* to the nearest power of 2 and
// return K = round(log2(m*)).
//
// Both functions clamp their results to sensible ranges to avoid degenerate
// outputs from extreme or near-zero mean values.

// optimal_golomb_m: returns the approximately optimal Golomb parameter m
// for a geometric source with the given mean (mean > 1).
inline std::size_t optimal_golomb_m(double mean) {
    assert(mean > 1.0 && "Golomb parameter undefined for mean <= 1");
    // Gallager-van Voorhis formula.
    double p = (mean - 1.0) / mean;  // geometric success probability
    // m* = -1 / log2(p)
    double m_star = -1.0 / std::log2(p);
    std::size_t m = static_cast<std::size_t>(std::round(m_star));
    if (m < 1) m = 1;
    return m;
}

// optimal_rice_k: returns the approximately optimal Rice parameter K
// for a geometric source with the given mean (mean > 1).
// K is chosen so that 2^K is the nearest power of 2 to the optimal Golomb m*.
inline std::size_t optimal_rice_k(double mean) {
    if (mean <= 2.0) return 1;  // Clamp: K must be >= 1.
    double p = (mean - 1.0) / mean;
    double m_star = -1.0 / std::log2(p);
    // Round log2(m*) to the nearest integer to get K.
    double k_real = std::log2(m_star);
    std::size_t k = static_cast<std::size_t>(std::max(1.0, std::round(k_real)));
    if (k >= 63) k = 62;  // Clamp to valid Rice template range.
    return k;
}
```

- [ ] **Step 4: Build and verify tests pass**

```bash
cd /home/spinoza/github/metafunctor-series/wire-formats && make test 2>&1 | grep -E "PASSED|FAILED|test_rice_golomb"
```

Expected: all RiceGolombTest cases pass including the new optimal_* tests.

- [ ] **Step 5: Commit**

```bash
cd /home/spinoza/github/metafunctor-series/wire-formats
git add post/2023-09-rice-golomb-wire-formats/rice_golomb.hpp \
        post/2023-09-rice-golomb-wire-formats/test_rice_golomb.cpp
git commit -m "feat(rice-golomb): add optimal_rice_k and optimal_golomb_m (TDD)"
```

---

## Task 6: Optimality verification tests using the priors library

**Files:**
- Modify: `post/2023-09-rice-golomb-wire-formats/test_rice_golomb.cpp`
- Modify: `post/CMakeLists.txt` (add priors include path)

These tests verify the spec's section D claim: for a geometric source with mean mu, Rice with the optimal K has small redundancy compared to the entropy.

- [ ] **Step 1: Add priors include path to test_rice_golomb in CMakeLists.txt**

In `post/CMakeLists.txt`, change the `target_include_directories` line for `test_rice_golomb` to also include the priors directory:

```cmake
target_include_directories(test_rice_golomb PRIVATE
    2023-09-rice-golomb-wire-formats
    2022-01-priors-wire-formats)
```

- [ ] **Step 2: Append optimality integration tests to test_rice_golomb.cpp**

Append to `test_rice_golomb.cpp`:

```cpp
#include "../2022-01-priors-wire-formats/priors.hpp"
#include <cmath>

// Build a geometric distribution truncated to N terms with mean mu.
// Geometric over non-negative integers: P(n) = (1 - p)^n * p, where p = 1/mu.
// For the truncated version, we renormalize.
static std::vector<double> geometric_dist(double mu, std::size_t N) {
    double p = 1.0 / mu;
    std::vector<double> dist(N);
    double total = 0.0;
    for (std::size_t i = 0; i < N; ++i) {
        dist[i] = std::pow(1.0 - p, static_cast<double>(i)) * p;
        total += dist[i];
    }
    for (double& d : dist) d /= total;
    return dist;
}

// Build the Rice<K> length vector for n = 0..N-1: length = (n >> K) + 1 + K.
template<std::size_t K>
static std::vector<std::size_t> rice_lengths(std::size_t N) {
    std::vector<std::size_t> v(N);
    for (std::size_t i = 0; i < N; ++i) {
        v[i] = (i >> K) + 1 + K;
    }
    return v;
}

// Optimal Rice<K> has small redundancy on a geometric source with mean 2^K.
// Test: K=1 (mean=2), K=2 (mean=4), K=3 (mean=8).
TEST(RiceGolombTest, Rice1SmallRedundancyOnGeometricMean2) {
    const std::size_t N = 128;
    auto dist = geometric_dist(2.0, N);
    auto lens = rice_lengths<1>(N);
    double r = priors::redundancy(dist, lens);
    EXPECT_GE(r, 0.0);
    EXPECT_LT(r, 2.0);
}

TEST(RiceGolombTest, Rice2SmallRedundancyOnGeometricMean4) {
    const std::size_t N = 256;
    auto dist = geometric_dist(4.0, N);
    auto lens = rice_lengths<2>(N);
    double r = priors::redundancy(dist, lens);
    EXPECT_GE(r, 0.0);
    EXPECT_LT(r, 2.0);
}

TEST(RiceGolombTest, Rice3SmallRedundancyOnGeometricMean8) {
    const std::size_t N = 512;
    auto dist = geometric_dist(8.0, N);
    auto lens = rice_lengths<3>(N);
    double r = priors::redundancy(dist, lens);
    EXPECT_GE(r, 0.0);
    EXPECT_LT(r, 2.0);
}

// Optimal K beats non-optimal K: Rice<2> beats Rice<1> on mean=4 source.
TEST(RiceGolombTest, OptimalKBeatsNonOptimalK) {
    const std::size_t N = 256;
    auto dist = geometric_dist(4.0, N);
    auto lens1 = rice_lengths<1>(N);
    auto lens2 = rice_lengths<2>(N);
    double r1 = priors::redundancy(dist, lens1);
    double r2 = priors::redundancy(dist, lens2);
    EXPECT_LT(r2, r1);  // Rice<2> has lower redundancy on mean=4 source.
}
```

- [ ] **Step 3: Build and verify**

```bash
cd /home/spinoza/github/metafunctor-series/wire-formats && make test 2>&1 | grep -E "PASSED|FAILED|test_rice_golomb"
```

Expected: all RiceGolombTest cases pass including the optimality tests.

- [ ] **Step 4: Commit**

```bash
cd /home/spinoza/github/metafunctor-series/wire-formats
git add post/2023-09-rice-golomb-wire-formats/test_rice_golomb.cpp \
        post/CMakeLists.txt
git commit -m "test(rice-golomb): add optimality integration tests against priors library"
```

---

## Task 7: Verify post 7 full-suite pass (clean rebuild + warning check)

**Files:** no changes; verification only.

- [ ] **Step 1: Clean rebuild**

```bash
cd /home/spinoza/github/metafunctor-series/wire-formats && make clean && make build 2>&1 | tail -5
```

Expected: successful clean build with no errors.

- [ ] **Step 2: Warning check for rice_golomb**

```bash
cd /home/spinoza/github/metafunctor-series/wire-formats
cmake -B post/build -S post -DCMAKE_CXX_FLAGS="-Wall -Wextra -Wpedantic" 2>&1 \
    | grep -E "warning:|error:" | grep "rice_golomb" | head -20
cmake --build post/build --target test_rice_golomb 2>&1 \
    | grep -E "warning:|error:" | head -20
```

Expected: zero warnings and zero errors from `rice_golomb.hpp` and `test_rice_golomb.cpp`.

- [ ] **Step 3: Full ctest pass**

```bash
cd /home/spinoza/github/metafunctor-series/wire-formats && make test 2>&1 | tail -10
```

Expected: all test suites pass (test_kraft, test_mcmillan, test_priors, test_unary_gamma, test_elias_delta_omega, test_fibonacci, test_rice_golomb).

No commit for this task.

---

## Task 8: Draft post 7 prose

**Files:**
- Modify: `post/2023-09-rice-golomb-wire-formats/index.md`

Draft from the spec's sections A through G for Post 7. The target is approximately 2000 words. No em-dashes anywhere in the file.

- [ ] **Step 1: Draft the prose**

Replace the placeholder `(Draft in progress...)` line in `post/2023-09-rice-golomb-wire-formats/index.md` with the full article body. Use the spec's section-by-section outline:

Section A ("The First Parametric Code", ~200 words, no code): all codes seen so far have been monolithic (unary is unary, gamma is gamma). Rice and Golomb introduce a parameter that lets you tune the code to a specific source's expected value. This is the first time we get to choose. Frame it: Rice(k) is a family of codes, one per value of k. Each member is optimal for a specific geometric distribution. Choosing k is choosing your prior precisely.

Section B ("Rice Coding", ~300 words + `Rice<K>` code block): present the splitting rule (q = n >> K, r = n & mask), the struct implementation, and the codeword table for k=2 from the spec. Length analysis: (n >> K) + 1 + K bits. Kraft sum saturates. Implied prior: geometric tuned by K.

Section C ("Golomb Coding", ~250 words + `Golomb<M>` code block): generalize to non-power-of-2 divisors using truncated binary. Show the truncated-binary encoding rule. Show the codewords for m=5 as a small table (n=0..4: q=0 with 2-bit and 3-bit remainders).

Section D ("The Parameter Selection", ~250 words + `optimal_rice_k`/`optimal_golomb_m` code block): present the Gallager-van Voorhis formula. Show that for mean 4, the optimal K is 2. Show the optimality redundancy test result (Rice<2> has lower redundancy than Rice<1> on a mean=4 geometric source). The takeaway: knowing the data's mean lets you pick an optimal code.

Section E ("Use Cases", ~250 words, no code): Rice coding is the standard for run-length encoding (FLAC). Golomb is used in image compression (JPEG-LS) and bitmap formats. The pattern: when your data is genuinely geometric and you can estimate the mean, Rice/Golomb beats every other universal code.

Section F ("The Connection to Huffman", ~250 words, no code): Rice/Golomb is parametric; Huffman is constructed. Rice with optimal k is asymptotically as good as Huffman on a geometric source, but Rice does not require building a tree at run time. Forward to post 9 (Huffman, "forthcoming").

Section G ("Cross-references and footnote", ~120 words): forward to post 8 (VByte, live link `/post/2024-02-vbyte-wire-formats/`). Back to post 3 (Universal Codes as Priors, live link), posts 4 through 6 (live links). Cross-series links to both Stepanov bridge posts (Rice's parameter K is conceptually the "tag-bit-width" choice in the Either combinator). PFC footnote pointing to `include/pfc/codecs.hpp` (`Rice<K>` and `Golomb<M>` structs).

Set `draft: false` in the frontmatter when satisfied with the draft.

- [ ] **Step 2: Soul check (banned-phrase hook)**

```bash
grep -n $'\xe2\x80\x94' /home/spinoza/github/metafunctor-series/wire-formats/post/2023-09-rice-golomb-wire-formats/index.md | head -5
```

Expected: no output (no em-dashes in the file).

- [ ] **Step 3: Commit**

```bash
cd /home/spinoza/github/metafunctor-series/wire-formats
git add post/2023-09-rice-golomb-wire-formats/index.md
git commit -m "docs(rice-golomb): draft post 7 prose (Rice / Golomb)"
```

---

## Task 9: Scaffold post 8 directory and wire CMakeLists

**Files:**
- Create: `post/2024-02-vbyte-wire-formats/index.md`
- Create: `post/2024-02-vbyte-wire-formats/vbyte.hpp`
- Create: `post/2024-02-vbyte-wire-formats/test_vbyte.cpp`
- Modify: `post/CMakeLists.txt`

- [ ] **Step 1: Create the post 8 directory and skeleton files**

```bash
mkdir -p /home/spinoza/github/metafunctor-series/wire-formats/post/2024-02-vbyte-wire-formats
```

Create `post/2024-02-vbyte-wire-formats/index.md` with placeholder frontmatter:

```markdown
---
title: "VByte / Varint"
date: 2024-02-25
draft: true
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

(Draft in progress. See plan Task 13 for full prose.)
```

Create `post/2024-02-vbyte-wire-formats/vbyte.hpp` with header guards only:

```cpp
// vbyte.hpp
// Pedagogical implementation for the post "VByte / Varint" in the
// "Algebra over Wire Formats" series. For the production version, see PFC:
// https://github.com/queelius/pfc (codecs.hpp: VByte)
//
// Note: real VByte implementations operate on bytes directly, not individual
// bits. This bit-level implementation is for consistency with the rest of the
// series. See the post's section B aside for an explanation.

#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace vbyte {

// BitSink and BitSource concepts (minimal local definitions for self-contained
// pedagogical use; production code uses pfc/core.hpp).

template<typename S>
concept BitSink = requires(S& s, bool b) {
    { s.write(b) } -> std::same_as<void>;
};

template<typename S>
concept BitSource = requires(S& s) {
    { s.read() } -> std::same_as<bool>;
};

// Implementation arrives in Task 10.

}  // namespace vbyte
```

Create `post/2024-02-vbyte-wire-formats/test_vbyte.cpp` with a placeholder test:

```cpp
#include <gtest/gtest.h>
#include "vbyte.hpp"

TEST(VByteTest, Placeholder) {
    EXPECT_TRUE(true);
}
```

- [ ] **Step 2: Append the post 8 test executable to post/CMakeLists.txt**

Append to `/home/spinoza/github/metafunctor-series/wire-formats/post/CMakeLists.txt`:

```cmake

# =============================================================================
# VByte / Varint (post 8, 2024-02-25)
# =============================================================================
add_executable(test_vbyte 2024-02-vbyte-wire-formats/test_vbyte.cpp)
target_link_libraries(test_vbyte GTest::gtest_main)
target_include_directories(test_vbyte PRIVATE 2024-02-vbyte-wire-formats)
add_test(NAME test_vbyte COMMAND test_vbyte)
```

- [ ] **Step 3: Build and verify placeholder test passes**

```bash
cd /home/spinoza/github/metafunctor-series/wire-formats && make test 2>&1 | tail -12
```

Expected: all existing tests pass plus the new `test_vbyte.Placeholder` test.

- [ ] **Step 4: Commit**

```bash
cd /home/spinoza/github/metafunctor-series/wire-formats
git add post/2024-02-vbyte-wire-formats post/CMakeLists.txt
git commit -m "scaffold(vbyte): add post 8 directory and CMake wiring"
```

---

## Task 10: TDD -- implement `VByte` codec

**Files:**
- Modify: `post/2024-02-vbyte-wire-formats/vbyte.hpp`
- Modify: `post/2024-02-vbyte-wire-formats/test_vbyte.cpp`

- [ ] **Step 1: Write failing tests for `VByte`**

Replace `test_vbyte.cpp` with:

```cpp
#include <gtest/gtest.h>
#include <cstdint>
#include <vector>
#include "vbyte.hpp"

using namespace vbyte;

// Minimal in-memory BitSink/BitSource for tests.
struct BitBuffer {
    std::vector<bool> bits;
    std::size_t pos_ = 0;
    void write(bool b) { bits.push_back(b); }
    bool read() { bool b = bits[pos_]; ++pos_; return b; }
};

// Helper: encode then decode n via VByte, check round-trip.
static std::uint64_t vbyte_round_trip(std::uint64_t n) {
    BitBuffer buf;
    VByte::encode(n, buf);
    buf.pos_ = 0;
    return VByte::decode(buf);
}

// Helper: return the bit count emitted for VByte(n).
static std::size_t vbyte_bit_count(std::uint64_t n) {
    BitBuffer buf;
    VByte::encode(n, buf);
    return buf.bits.size();
}

// Round-trip: small values (fit in 1 byte = 8 bits).
TEST(VByteTest, RoundTripSmall) {
    for (std::uint64_t n = 0; n <= 127; ++n) {
        EXPECT_EQ(vbyte_round_trip(n), n) << "n=" << n;
    }
}

// Round-trip: two-byte range [128, 16383].
TEST(VByteTest, RoundTripTwoByte) {
    for (std::uint64_t n : {std::uint64_t{128}, std::uint64_t{255},
                             std::uint64_t{1000}, std::uint64_t{16383}}) {
        EXPECT_EQ(vbyte_round_trip(n), n) << "n=" << n;
    }
}

// Round-trip: three-byte range.
TEST(VByteTest, RoundTripThreeByte) {
    for (std::uint64_t n : {std::uint64_t{16384}, std::uint64_t{100000},
                             std::uint64_t{2097151}}) {
        EXPECT_EQ(vbyte_round_trip(n), n) << "n=" << n;
    }
}

// Round-trip: large values (multi-byte).
TEST(VByteTest, RoundTripLarge) {
    for (std::uint64_t n : {std::uint64_t{1000000}, std::uint64_t{0xFFFFFFFF},
                             std::uint64_t{0xFFFFFFFFFFFFull}}) {
        EXPECT_EQ(vbyte_round_trip(n), n) << "n=" << n;
    }
}

// Values 0-127 encode in exactly 8 bits (1 byte).
TEST(VByteTest, OneByteLengthForSmall) {
    for (std::uint64_t n = 0; n <= 127; ++n) {
        EXPECT_EQ(vbyte_bit_count(n), 8u) << "n=" << n;
    }
}

// Values 128-16383 encode in exactly 16 bits (2 bytes).
TEST(VByteTest, TwoByteLengthForMedium) {
    for (std::uint64_t n : {std::uint64_t{128}, std::uint64_t{1000},
                             std::uint64_t{16383}}) {
        EXPECT_EQ(vbyte_bit_count(n), 16u) << "n=" << n;
    }
}

// Values 16384-2097151 encode in exactly 24 bits (3 bytes).
TEST(VByteTest, ThreeByteLengthForLarge) {
    for (std::uint64_t n : {std::uint64_t{16384}, std::uint64_t{2097151}}) {
        EXPECT_EQ(vbyte_bit_count(n), 24u) << "n=" << n;
    }
}

// Spot-check: VByte(0) = single byte 0x00 -> bits 0,0,0,0,0,0,0,0 (LSB first).
TEST(VByteTest, EncodingZero) {
    BitBuffer buf;
    VByte::encode(std::uint64_t{0}, buf);
    ASSERT_EQ(buf.bits.size(), 8u);
    for (int i = 0; i < 8; ++i) {
        EXPECT_EQ(buf.bits[static_cast<std::size_t>(i)], false) << "bit " << i;
    }
}

// Spot-check: VByte(1) = byte 0x01 -> bits 1,0,0,0,0,0,0,0 (LSB first).
TEST(VByteTest, EncodingOne) {
    BitBuffer buf;
    VByte::encode(std::uint64_t{1}, buf);
    ASSERT_EQ(buf.bits.size(), 8u);
    EXPECT_EQ(buf.bits[0], true);   // bit 0 (LSB) of byte 0x01
    for (int i = 1; i < 8; ++i) {
        EXPECT_EQ(buf.bits[static_cast<std::size_t>(i)], false) << "bit " << i;
    }
}

// Spot-check: VByte(128) = two bytes: 0x80, 0x01.
// First byte: bits 0,0,0,0,0,0,0,1 (value 128 & 0x7F = 0, continuation bit set).
// Second byte: bits 1,0,0,0,0,0,0,0 (value 1, no continuation bit).
TEST(VByteTest, Encoding128) {
    BitBuffer buf;
    VByte::encode(std::uint64_t{128}, buf);
    ASSERT_EQ(buf.bits.size(), 16u);
    // First byte: 0x80 = 10000000 binary. Stored LSB first: 0,0,0,0,0,0,0,1.
    for (int i = 0; i < 7; ++i) {
        EXPECT_EQ(buf.bits[static_cast<std::size_t>(i)], false) << "byte0 bit " << i;
    }
    EXPECT_EQ(buf.bits[7], true);   // continuation bit (bit 7 of byte 0)
    // Second byte: 0x01 = 00000001. Stored LSB first: 1,0,0,0,0,0,0,0.
    EXPECT_EQ(buf.bits[8], true);   // bit 0 of second byte
    for (int i = 9; i < 16; ++i) {
        EXPECT_EQ(buf.bits[static_cast<std::size_t>(i)], false) << "byte1 bit " << i;
    }
}

// n=0 is valid.
TEST(VByteTest, RoundTripZero) {
    EXPECT_EQ(vbyte_round_trip(std::uint64_t{0}), std::uint64_t{0});
}
```

- [ ] **Step 2: Run to verify compile failure**

```bash
cd /home/spinoza/github/metafunctor-series/wire-formats && make build 2>&1 | grep -E "error:|undefined" | head -10
```

Expected: compile errors about `VByte` not declared.

- [ ] **Step 3: Implement `VByte` in `vbyte.hpp`**

Replace the `// Implementation arrives in Task 10.` comment with:

```cpp
// ---- VByte -- byte-aligned variable-length encoding -------------------------
//
// Encodes non-negative integer n >= 0 as a sequence of 7-bit groups,
// least significant first. Each group is stored in a byte where:
//   - Bits 0-6 hold 7 bits of the integer's value.
//   - Bit 7 (MSB) is a continuation flag: 1 = more bytes follow, 0 = last byte.
//
// So small integers (0-127) take 1 byte (8 bits); integers up to 2^14 - 1
// take 2 bytes (16 bits); up to 2^21 - 1 take 3 bytes (24 bits); etc.
//
// This bit-level implementation stores each byte LSB first to maintain
// consistency with the rest of the series. Real VByte implementations
// operate on bytes directly without the bit-level layer.
//
// Length: 8 * ceil(log2(n+1) / 7) bits, with a minimum of 8 bits.
// Implied prior: step-uniform over byte boundaries. VByte is optimal for
// sources where the byte-length of the encoding is geometrically distributed.
// Practical interpretation: if most values fit in 1 or 2 bytes, VByte is
// within a constant factor of entropy.

struct VByte {
    using value_type = std::uint64_t;

    template<BitSink S>
    static void encode(value_type n, S& sink) {
        while (n >= 128) {
            // Group the low 7 bits and set the continuation flag (bit 7).
            std::uint8_t byte = static_cast<std::uint8_t>((n & 0x7F) | 0x80);
            // Write the byte LSB first.
            for (int i = 0; i < 8; ++i) sink.write(((byte >> i) & 1) != 0);
            n >>= 7;
        }
        // Last byte: low 7 bits, no continuation flag.
        std::uint8_t byte = static_cast<std::uint8_t>(n & 0x7F);
        for (int i = 0; i < 8; ++i) sink.write(((byte >> i) & 1) != 0);
    }

    template<BitSource S>
    static value_type decode(S& source) {
        value_type result = 0;
        std::size_t shift = 0;
        while (true) {
            // Read one byte (8 bits, LSB first).
            std::uint8_t byte = 0;
            for (int i = 0; i < 8; ++i) {
                if (source.read()) byte |= static_cast<std::uint8_t>(1 << i);
            }
            // Accumulate the 7 data bits at the current shift position.
            result |= static_cast<value_type>(byte & 0x7F) << shift;
            // If bit 7 is clear, this is the last byte.
            if ((byte & 0x80) == 0) break;
            shift += 7;
        }
        return result;
    }
};
```

- [ ] **Step 4: Build and verify tests pass**

```bash
cd /home/spinoza/github/metafunctor-series/wire-formats && make test 2>&1 | grep -E "PASSED|FAILED|test_vbyte"
```

Expected: all VByteTest cases pass (round-trips, length checks, spot-checks).

- [ ] **Step 5: Commit**

```bash
cd /home/spinoza/github/metafunctor-series/wire-formats
git add post/2024-02-vbyte-wire-formats/vbyte.hpp \
        post/2024-02-vbyte-wire-formats/test_vbyte.cpp
git commit -m "feat(vbyte): implement VByte codec (TDD)"
```

---

## Task 11: Length-comparison tests (VByte vs Gamma vs Delta table)

**Files:**
- Modify: `post/2024-02-vbyte-wire-formats/test_vbyte.cpp`
- Modify: `post/CMakeLists.txt` (add include paths for delta and gamma)

These tests verify the spec's section D comparison table and embed it as a machine-checked artifact.

- [ ] **Step 1: Add include paths for the relevant prior posts**

In `post/CMakeLists.txt`, update `target_include_directories` for `test_vbyte`:

```cmake
target_include_directories(test_vbyte PRIVATE
    2024-02-vbyte-wire-formats
    2022-01-priors-wire-formats
    2022-06-elias-gamma-wire-formats
    2022-11-elias-delta-omega-wire-formats)
```

- [ ] **Step 2: Append the length-comparison tests to test_vbyte.cpp**

Append to `test_vbyte.cpp`:

```cpp
#include "../2022-01-priors-wire-formats/priors.hpp"
#include "../2022-06-elias-gamma-wire-formats/unary_gamma.hpp"
#include "../2022-11-elias-delta-omega-wire-formats/elias_delta_omega.hpp"

// Helper: compute VByte length in bits for n.
// Formula: 8 * ceil(log2(n+1) / 7), minimum 8.
static std::size_t vbyte_length_formula(std::uint64_t n) {
    if (n < 128) return 8;
    std::uint64_t tmp = n;
    std::size_t bytes = 0;
    while (tmp > 0) {
        ++bytes;
        tmp >>= 7;
    }
    return bytes * 8;
}

// Verify VByte formula matches actual encode bit count.
TEST(VByteTest, LengthFormulaMatchesEncode) {
    for (std::uint64_t n : {std::uint64_t{0}, std::uint64_t{1}, std::uint64_t{100},
                             std::uint64_t{127}, std::uint64_t{128}, std::uint64_t{1000},
                             std::uint64_t{16383}, std::uint64_t{16384},
                             std::uint64_t{1048575}, std::uint64_t{1048576}}) {
        EXPECT_EQ(vbyte_bit_count(n), vbyte_length_formula(n)) << "n=" << n;
    }
}

// Spot-check the table from the spec (section D):
// | n       | VByte | Gamma | Delta |
// | 1       | 8     | 1     | 1     |
// | 100     | 8     | 13    | 13    |
// | 1000    | 16    | 19    | 16    |
// | 2^20    | 24    | 41    | 26    |
// | 2^32    | 40    | 65    | 39    |
//
// Gamma and Delta lengths are derived from their encoding bit counts.

// Helper: Gamma bit count for n.
static std::size_t gamma_bit_count_for(std::uint64_t n) {
    struct BitCounter {
        std::size_t count = 0;
        void write(bool) { ++count; }
    } counter;
    unary_gamma::Gamma::encode(n, counter);
    return counter.count;
}

// Helper: Delta bit count for n.
static std::size_t delta_bit_count_for(std::uint64_t n) {
    struct BitCounter {
        std::size_t count = 0;
        void write(bool) { ++count; }
    } counter;
    elias_delta_omega::Delta::encode(n, counter);
    return counter.count;
}

TEST(VByteTest, LengthTableN1) {
    EXPECT_EQ(vbyte_bit_count(1u), 8u);
    EXPECT_EQ(gamma_bit_count_for(1u), 1u);
    EXPECT_EQ(delta_bit_count_for(1u), 1u);
}

TEST(VByteTest, LengthTableN100) {
    EXPECT_EQ(vbyte_bit_count(100u), 8u);
    EXPECT_EQ(gamma_bit_count_for(100u), 13u);
    EXPECT_EQ(delta_bit_count_for(100u), 13u);
}

TEST(VByteTest, LengthTableN1000) {
    EXPECT_EQ(vbyte_bit_count(1000u), 16u);
    EXPECT_EQ(gamma_bit_count_for(1000u), 19u);
    EXPECT_EQ(delta_bit_count_for(1000u), 16u);
}

TEST(VByteTest, LengthTableN2pow20) {
    std::uint64_t n = 1u << 20;
    EXPECT_EQ(vbyte_bit_count(n), 24u);
    EXPECT_EQ(gamma_bit_count_for(n), 41u);
    EXPECT_EQ(delta_bit_count_for(n), 26u);
}

TEST(VByteTest, LengthTableN2pow32) {
    std::uint64_t n = std::uint64_t{1} << 32;
    EXPECT_EQ(vbyte_bit_count(n), 40u);
    EXPECT_EQ(gamma_bit_count_for(n), 65u);
    EXPECT_EQ(delta_bit_count_for(n), 39u);
}
```

- [ ] **Step 3: Build and verify**

```bash
cd /home/spinoza/github/metafunctor-series/wire-formats && make test 2>&1 | grep -E "PASSED|FAILED|test_vbyte"
```

Expected: all VByteTest cases pass including the length comparison table tests.

- [ ] **Step 4: Commit**

```bash
cd /home/spinoza/github/metafunctor-series/wire-formats
git add post/2024-02-vbyte-wire-formats/test_vbyte.cpp \
        post/CMakeLists.txt
git commit -m "test(vbyte): add length-comparison tests verifying VByte vs Gamma vs Delta table"
```

---

## Task 12: Verify post 8 full-suite pass (clean rebuild + warning check)

**Files:** no changes; verification only.

- [ ] **Step 1: Clean rebuild**

```bash
cd /home/spinoza/github/metafunctor-series/wire-formats && make clean && make build 2>&1 | tail -5
```

Expected: successful clean build with no errors.

- [ ] **Step 2: Warning check for vbyte**

```bash
cd /home/spinoza/github/metafunctor-series/wire-formats
cmake -B post/build -S post -DCMAKE_CXX_FLAGS="-Wall -Wextra -Wpedantic" 2>&1 \
    | grep -E "warning:|error:" | grep "vbyte" | head -20
cmake --build post/build --target test_vbyte 2>&1 \
    | grep -E "warning:|error:" | head -20
```

Expected: zero warnings and zero errors from `vbyte.hpp` and `test_vbyte.cpp`.

- [ ] **Step 3: Full ctest pass**

```bash
cd /home/spinoza/github/metafunctor-series/wire-formats && make test 2>&1 | tail -12
```

Expected: all test suites pass (test_kraft, test_mcmillan, test_priors, test_unary_gamma, test_elias_delta_omega, test_fibonacci, test_rice_golomb, test_vbyte).

No commit for this task.

---

## Task 13: Draft post 8 prose

**Files:**
- Modify: `post/2024-02-vbyte-wire-formats/index.md`

Draft from the spec's sections A through G for Post 8. The target is approximately 2000 words. No em-dashes anywhere in the file.

- [ ] **Step 1: Draft the prose**

Replace the placeholder in `post/2024-02-vbyte-wire-formats/index.md` with the full article body:

Section A ("The Practical Question", ~200 words, no code): all universal codes seen so far operate at bit granularity. Bit packing is theoretically optimal but computationally expensive. For high-throughput encoding (databases, network protocols, log compression), the overhead of bit packing can exceed the savings from compression. VByte (also called Varint) trades a small amount of length efficiency for byte-alignment. It is the encoding used by Protocol Buffers, Google's columnar databases, and most production columnar file formats.

Section B ("The Encoding", ~250 words + `VByte` code block): VByte splits an integer into 7-bit groups, least significant first. Each group is stored in a byte where bit 7 is a continuation flag. Show the `VByte` struct implementation. Include the aside: real VByte implementations operate on bytes directly, not bits. This bit-level implementation is for pedagogical consistency with the rest of the series.

Section C ("The Implied Prior", ~250 words, no code): length analysis. Integer n takes ceil(log2(n+1) / 7) bytes, or 8*ceil(log2(n+1) / 7) bits. Implied prior: step-uniform over byte boundaries. Within each byte boundary, all values have the same length. VByte is optimal for sources where the byte-length is geometrically distributed. Real-world sources rarely match this exactly, but VByte is competitive across a wide range.

Section D ("Length Comparison", ~250 words, no code): include the comparison table from the spec for n in {1, 100, 1000, 2^20, 2^32} across VByte, Gamma, and Delta. For large values, VByte is competitive with Delta and beats Gamma. For small values (1-127), VByte is much worse: a fixed 8-bit cost per value vs Gamma's 1-3 bits. This is the price of byte-alignment.

Section E ("Why It Wins in Practice", ~250 words, no code): decoding speed, memory layout, hardware support (SIMD VByte decoders on AVX2). Modern CPUs decode VByte at rates of multiple GB/s. Bit-level codes decode at maybe 100 MB/s due to bit-shifting overhead.

Section F ("The Engineering Trade", ~250 words, no code): frame VByte as the engineer's compromise: theoretically suboptimal, practically dominant. The information-theoretic analysis says Gamma is more efficient, but the engineering analysis says the constant factor of bit-vs-byte operations swamps the small length savings. This pattern recurs throughout systems work: theoretically optimal solutions often lose to implementation-friendly approximations.

Section G ("Cross-references and footnote", ~120 words): forward to post 9 (Huffman, "forthcoming" as plain text). Back to posts 4 through 7 (live links). Cross-series: not directly relevant; byte-alignment is orthogonal to the type-algebra story. PFC footnote pointing to `include/pfc/codecs.hpp` (`VByte` struct), and note that Google's protobuf-cpp has the SIMD-optimized version this post does not implement.

Set `draft: false` when satisfied.

- [ ] **Step 2: Soul check**

```bash
grep -n $'\xe2\x80\x94' /home/spinoza/github/metafunctor-series/wire-formats/post/2024-02-vbyte-wire-formats/index.md | head -5
```

Expected: no output (no em-dashes).

- [ ] **Step 3: Commit**

```bash
cd /home/spinoza/github/metafunctor-series/wire-formats
git add post/2024-02-vbyte-wire-formats/index.md
git commit -m "docs(vbyte): draft post 8 prose (VByte / Varint)"
```

---

## Task 14: Update `docs/about.md` (mark posts 7 and 8 as Published)

**Files:**
- Modify: `docs/about.md`

- [ ] **Step 1: Update the status rows for posts 7 and 8**

In `docs/about.md`, change the rows for posts 7 and 8 from `Forthcoming` to `Published`:

Old lines:
```
| 7 | Rice / Golomb | 2023-09-17 | Forthcoming |
| 8 | VByte / Varint | 2024-02-25 | Forthcoming |
```

New lines:
```
| 7 | Rice / Golomb | 2023-09-17 | Published |
| 8 | VByte / Varint | 2024-02-25 | Published |
```

- [ ] **Step 2: Commit**

```bash
cd /home/spinoza/github/metafunctor-series/wire-formats
git add docs/about.md
git commit -m "docs(about): mark posts 7 and 8 as Published"
```

---

## Task 15: Update `mkdocs.yml` (add posts 7 and 8 to nav)

**Files:**
- Modify: `mkdocs.yml`

Posts 7 and 8 both belong to the "Universal Codes" nav section alongside posts 3 through 6. The section should grow to include all six universal-codes posts.

- [ ] **Step 1: Extend the "Universal Codes" nav section**

In `mkdocs.yml`, the "Universal Codes" section (created by sub-sub-projects 3a and 3b) currently ends with the Fibonacci entry. Add posts 7 and 8:

```yaml
  - "Universal Codes":
      - "Universal Codes as Priors": "post/2022-01-priors-wire-formats/index.md"
      - "Unary and Elias Gamma": "post/2022-06-elias-gamma-wire-formats/index.md"
      - "Elias Delta and Omega": "post/2022-11-elias-delta-omega-wire-formats/index.md"
      - "Fibonacci Coding": "post/2023-04-fibonacci-wire-formats/index.md"
      - "Rice / Golomb": "post/2023-09-rice-golomb-wire-formats/index.md"
      - "VByte / Varint": "post/2024-02-vbyte-wire-formats/index.md"
```

- [ ] **Step 2: Commit**

```bash
cd /home/spinoza/github/metafunctor-series/wire-formats
git add mkdocs.yml
git commit -m "docs(mkdocs): add posts 7 and 8 (Rice/Golomb, VByte) to Universal Codes nav"
```

---

## Task 16: Final clean rebuild + soul check + mkdocs build

**Files:** no changes; verification only.

- [ ] **Step 1: Clean rebuild and full test pass**

```bash
cd /home/spinoza/github/metafunctor-series/wire-formats && make clean && make test 2>&1 | tail -12
```

Expected: all eight test suites pass (test_kraft, test_mcmillan, test_priors, test_unary_gamma, test_elias_delta_omega, test_fibonacci, test_rice_golomb, test_vbyte).

- [ ] **Step 2: Soul check on both new prose files**

```bash
grep -n $'\xe2\x80\x94' \
    /home/spinoza/github/metafunctor-series/wire-formats/post/2023-09-rice-golomb-wire-formats/index.md \
    /home/spinoza/github/metafunctor-series/wire-formats/post/2024-02-vbyte-wire-formats/index.md \
    | head -10
```

Expected: no output (no em-dashes in either file).

- [ ] **Step 3: mkdocs build**

```bash
cd /home/spinoza/github/metafunctor-series/wire-formats && make docs 2>&1 | tail -8
```

Expected: successful build. Posts 7 and 8 should resolve cleanly under the "Universal Codes" section.

No commit for this task.

---

## Task 17: Hugo sync via FIXED Makefile

**Files:** changes land in `~/github/repos/metafunctor/` (synced, not committed here).

- [ ] **Step 1: Sync posts 7 and 8 to metafunctor**

```bash
BLOG_POST_DIR=/home/spinoza/github/repos/metafunctor/content/post \
    make -C /home/spinoza/github/metafunctor-series/wire-formats sync 2>&1
```

Expected: rsync output showing two new directories synced:
- `-> 2023-09-rice-golomb-wire-formats`
- `-> 2024-02-vbyte-wire-formats`

- [ ] **Step 2: Verify metafunctor received both post directories**

```bash
ls /home/spinoza/github/repos/metafunctor/content/post/ | grep -E "2023-09-rice-golomb|2024-02-vbyte"
```

Expected: both directories present.

No commit for this task (sync only; metafunctor commit is a separate step).

---

## Task 18: Verify metafunctor state

**Files:** read-only inspection of metafunctor repo.

- [ ] **Step 1: Check git status in metafunctor**

```bash
cd /home/spinoza/github/repos/metafunctor && git status --short | grep -E "2023-09-rice-golomb|2024-02-vbyte"
```

Expected: two new untracked directories (or staged adds if already added).

- [ ] **Step 2: Check mf series scan**

```bash
cd /home/spinoza/github/repos/metafunctor && mf series scan 2>&1 | grep wire-formats | head -5
```

Expected: wire-formats series shows count including the two new posts. After sub-sub-projects 3a, 3b, and 3c, the total should be 8 posts (Kraft, McMillan, priors, unary-gamma, delta-omega, fibonacci, rice-golomb, vbyte).

No commit for this task.

---

## Task 19: Final pre-push verification across both repos

**Files:** read-only.

- [ ] **Step 1: Wire-formats git log (confirm all commits present)**

```bash
git -C /home/spinoza/github/metafunctor-series/wire-formats log --oneline | head -20
```

Expected: all commits from this plan appear in sequence ending with the mkdocs commit. Confirm scaffold(rice-golomb), feat(rice-golomb), feat(rice-golomb), feat(rice-golomb), test(rice-golomb), scaffold(vbyte), feat(vbyte), test(vbyte), docs(rice-golomb), docs(vbyte), docs(about), docs(mkdocs) are all present.

- [ ] **Step 2: Verify no uncommitted changes in wire-formats**

```bash
cd /home/spinoza/github/metafunctor-series/wire-formats && git status --short
```

Expected: clean working tree (the build/ and site/ directories are gitignored).

- [ ] **Step 3: Confirm metafunctor has the two new post directories ready to commit**

```bash
cd /home/spinoza/github/repos/metafunctor && git status --short | grep "2023-"
```

Expected: two new directories shown as untracked or staged.

No commit for this task.

---

## Task 20: User-confirmed push

The user must confirm before pushing. Do not push automatically.

- [ ] **Step 1: Present the push plan and wait for confirmation**

Report the following to the user before proceeding:

- Wire-formats repo: N new commits to push to origin/main (all commits from this plan).
- Metafunctor repo: 2 new post directories to commit and push.
- No changes to the Stepanov repo in this sub-sub-project.

Wait for explicit user approval.

- [ ] **Step 2: Push wire-formats (after user confirms)**

```bash
git -C /home/spinoza/github/metafunctor-series/wire-formats push origin main
```

- [ ] **Step 3: Commit and push metafunctor (after user confirms)**

```bash
cd /home/spinoza/github/repos/metafunctor
git add content/post/2023-09-rice-golomb-wire-formats \
        content/post/2024-02-vbyte-wire-formats
git commit -m "content(wire-formats): sync posts 7 and 8 (rice-golomb, vbyte)"
git push origin main
```

---

## Task 21: Final summary report

- [ ] **Step 1: Produce a summary for the user**

Report:
- Posts shipped: post 7 ("Rice / Golomb", 2023-09-17) and post 8 ("VByte / Varint", 2024-02-25).
- Files created: `rice_golomb.hpp` (Rice<K>, Golomb<M>, detail::truncated_binary_encode/decode, optimal_rice_k, optimal_golomb_m), `test_rice_golomb.cpp` (round-trips, bit-count checks, spot-checks, optimality tests against priors library), `vbyte.hpp` (VByte), `test_vbyte.cpp` (round-trips, length checks, spot-checks, length-comparison table vs Gamma and Delta), `index.md` for each post.
- Test counts: all suites pass (test_kraft, test_mcmillan, test_priors, test_unary_gamma, test_elias_delta_omega, test_fibonacci, test_rice_golomb, test_vbyte).
- Navigation: `mkdocs.yml` updated with posts 7 and 8 under "Universal Codes"; `docs/about.md` updated with Published status for both posts.
- Sync: both posts available in metafunctor under `content/post/`.
- Next: sub-sub-project 3d covers post 9 ("Huffman"). Post 9 shifts from universal codes to entropy-optimal codes; its `huffman.hpp` does not depend on any of the universal-code headers from posts 3 through 8 but can use the priors library for the optimality proof.
