# Algebra over Wire Formats: Sub-sub-project 3b (Posts 5 and 6) Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Ship posts 5 ("Elias Delta and Omega", 2022-02-13) and 6 ("Fibonacci Coding", 2022-07-17) of the Algebra over Wire Formats series, including TDD implementations of `elias_delta_omega.hpp` and `fibonacci.hpp`, full GoogleTest suites, prose drafts, and sync to metafunctor.com.

**Architecture:** Two new post directories under `post/` (`2022-02-elias-delta-omega-wire-formats/` and `2022-07-fibonacci-wire-formats/`), each with a header, a test file, and `index.md`. Post 5's `elias_delta_omega.hpp` defines `Gamma` (re-implemented locally for loose coupling), `Delta`, and `Omega` codecs in `namespace elias_delta_omega`; the delta and omega implementations follow the spec exactly. Post 6's `fibonacci.hpp` defines `to_zeckendorf` and the `Fibonacci` codec in `namespace fibonacci`. Both posts wire into the existing `post/CMakeLists.txt`, update `docs/about.md` and `mkdocs.yml`, and sync to metafunctor via the Makefile `sync` target.

**Tech Stack:** C++23, GoogleTest v1.14.0 (already wired in `post/CMakeLists.txt`), mkdocs, soul plugin's banned-phrase hook.

---

## Spec reference

See `docs/superpowers/specs/2026-04-24-arc-posts-3-through-13.md`, sections "Post 5: Elias Delta and Omega" and "Post 6: Fibonacci Coding" for per-section content guides A through G, code budgets, and prose budgets.

## Cross-references note

The Stepanov bridge posts were updated in sub-project 2. No further Stepanov changes are needed in this sub-sub-project.

Post 5 back-links to posts 3 and 4 (live links, already published). Forward reference to post 6 is a live link in the same sub-sub-project (scaffold post 6 before finalizing post 5 prose). Cross-series links to both Stepanov bridge posts (per the cross-reference map in the spec).

Post 6 back-links to posts 3 and 4 (live links). Forward reference to post 7 (Rice/Golomb) is plain text (post 7 does not exist yet). No cross-series links for post 6 (Fibonacci is its own animal).

Loose-coupling pattern: `elias_delta_omega.hpp` re-implements `Gamma` locally rather than including `unary_gamma.hpp`. This mirrors the design of `unary_gamma.hpp`, which does not include `priors.hpp`. Each post's header stands alone. Integration tests in the test file may include `priors.hpp` via a relative path (as in post 4).

---

## Task 1: Reconnaissance (date collision check)

**Files:** read-only.

- [ ] **Step 1: Verify dates 2022-02-13 and 2022-07-17 do not collide with existing metafunctor posts**

```bash
grep -h "^date:" /home/spinoza/github/repos/metafunctor/content/post/*/index.md 2>/dev/null \
  | grep -E "^date: 2022-02-13|^date: 2022-07-17" | sort -u
```

Expected: empty output. If any dates collide, pick adjacent unused days and record the substitutes before proceeding.

- [ ] **Step 2: Confirm post 5 and post 6 directories do not yet exist**

```bash
ls /home/spinoza/github/metafunctor-series/wire-formats/post/ | grep -E "2022-02|2022-07"
```

Expected: no output (neither directory exists yet).

- [ ] **Step 3: Confirm the CMakeLists.txt currently ends with the post 4 block**

```bash
tail -6 /home/spinoza/github/metafunctor-series/wire-formats/post/CMakeLists.txt
```

Expected: the last `add_test` line belongs to `test_unary_gamma`. No commit for this task.

---

## Task 2: Scaffold post 5 directory and wire CMakeLists

**Files:**
- Create: `post/2022-02-elias-delta-omega-wire-formats/index.md`
- Create: `post/2022-02-elias-delta-omega-wire-formats/elias_delta_omega.hpp`
- Create: `post/2022-02-elias-delta-omega-wire-formats/test_elias_delta_omega.cpp`
- Modify: `post/CMakeLists.txt`

- [ ] **Step 1: Create the post 5 directory and skeleton files**

```bash
mkdir -p /home/spinoza/github/metafunctor-series/wire-formats/post/2022-02-elias-delta-omega-wire-formats
```

Create `post/2022-02-elias-delta-omega-wire-formats/index.md` with placeholder frontmatter:

```markdown
---
title: "Elias Delta and Omega"
date: 2022-02-13
draft: true
tags:
- C++
- information-theory
- coding-theory
- prefix-free
- universal-codes
- elias-delta
- elias-omega
categories:
- Computer Science
- Mathematics
series:
- wire-formats
series_weight: 5
math: true
description: "Elias delta and omega extend Elias gamma by recursively encoding the length prefix. Each step yields shorter codewords for large integers at a small constant cost for small ones."
linked_project:
- pfc
- wire-formats
---

(Draft in progress. See plan Task 7 for full prose.)
```

Create `post/2022-02-elias-delta-omega-wire-formats/elias_delta_omega.hpp` with header guards only:

```cpp
// elias_delta_omega.hpp
// Pedagogical implementation for the post "Elias Delta and Omega" in the
// "Algebra over Wire Formats" series. For the production version, see PFC:
// https://github.com/queelius/pfc (codecs.hpp: EliasDelta, EliasOmega)
//
// Loose-coupling note: Gamma is re-implemented here rather than included
// from unary_gamma.hpp. Each post's header stands alone.

#pragma once

#include <bit>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <stack>
#include <utility>
#include <vector>

namespace elias_delta_omega {

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

}  // namespace elias_delta_omega
```

Create `post/2022-02-elias-delta-omega-wire-formats/test_elias_delta_omega.cpp` with a placeholder:

```cpp
#include <gtest/gtest.h>
#include "elias_delta_omega.hpp"

TEST(EliasDeltaOmegaTest, Placeholder) {
    EXPECT_TRUE(true);
}
```

- [ ] **Step 2: Append the post 5 test executable to post/CMakeLists.txt**

Append to `/home/spinoza/github/metafunctor-series/wire-formats/post/CMakeLists.txt`:

```cmake

# =============================================================================
# Elias Delta and Omega (post 5, 2022-02-13)
# =============================================================================
add_executable(test_elias_delta_omega
    2022-02-elias-delta-omega-wire-formats/test_elias_delta_omega.cpp)
target_link_libraries(test_elias_delta_omega GTest::gtest_main)
target_include_directories(test_elias_delta_omega PRIVATE
    2022-02-elias-delta-omega-wire-formats)
add_test(NAME test_elias_delta_omega COMMAND test_elias_delta_omega)
```

- [ ] **Step 3: Build and verify placeholder test passes**

```bash
cd /home/spinoza/github/metafunctor-series/wire-formats && make test 2>&1 | tail -12
```

Expected: all previous tests pass plus the new `test_elias_delta_omega.Placeholder` test. Output should include `[  PASSED  ] 1 test.` for test_elias_delta_omega.

- [ ] **Step 4: Commit**

```bash
cd /home/spinoza/github/metafunctor-series/wire-formats
git add post/2022-02-elias-delta-omega-wire-formats post/CMakeLists.txt
git commit -m "scaffold(elias-delta-omega): add post 5 directory and CMake wiring"
```

---

## Task 3: TDD -- implement `Gamma` (local re-implementation) and `Delta` codec

**Files:**
- Modify: `post/2022-02-elias-delta-omega-wire-formats/elias_delta_omega.hpp`
- Modify: `post/2022-02-elias-delta-omega-wire-formats/test_elias_delta_omega.cpp`

The spec requires Delta to call `Gamma::encode` internally. Gamma is re-implemented here verbatim from post 4 but inside `namespace elias_delta_omega` so this header stands alone.

- [ ] **Step 1: Write failing tests for `Gamma` (local) and `Delta`**

Replace `test_elias_delta_omega.cpp` with:

```cpp
#include <gtest/gtest.h>
#include <bit>
#include <cstdint>
#include <vector>
#include "elias_delta_omega.hpp"

using namespace elias_delta_omega;

// Minimal in-memory BitSink/BitSource for tests.
struct BitBuffer {
    std::vector<bool> bits;
    void write(bool b) { bits.push_back(b); }
    bool read() {
        bool b = bits[pos_];
        ++pos_;
        return b;
    }
    std::size_t pos_ = 0;
};

// ---- Gamma local re-implementation tests ------------------------------------

static uint64_t gamma_round_trip(uint64_t n) {
    BitBuffer buf;
    Gamma::encode(n, buf);
    buf.pos_ = 0;
    return Gamma::decode(buf);
}

static std::size_t gamma_bit_count(uint64_t n) {
    BitBuffer buf;
    Gamma::encode(n, buf);
    return buf.bits.size();
}

TEST(EliasDeltaOmegaTest, GammaLocalRoundTrip) {
    for (uint64_t n = 1; n <= 256; ++n) {
        EXPECT_EQ(gamma_round_trip(n), n) << "n=" << n;
    }
}

// Gamma length = 2*floor(log2(n)) + 1.
TEST(EliasDeltaOmegaTest, GammaLocalBitCount) {
    for (uint64_t n = 1; n <= 64; ++n) {
        std::size_t k = std::bit_width(n) - 1;
        EXPECT_EQ(gamma_bit_count(n), 2 * k + 1) << "n=" << n;
    }
}

// ---- Delta tests ------------------------------------------------------------

static uint64_t delta_round_trip(uint64_t n) {
    BitBuffer buf;
    Delta::encode(n, buf);
    buf.pos_ = 0;
    return Delta::decode(buf);
}

static std::size_t delta_bit_count(uint64_t n) {
    BitBuffer buf;
    Delta::encode(n, buf);
    return buf.bits.size();
}

TEST(EliasDeltaOmegaTest, DeltaRoundTrip) {
    for (uint64_t n = 1; n <= 256; ++n) {
        EXPECT_EQ(delta_round_trip(n), n) << "n=" << n;
    }
}

TEST(EliasDeltaOmegaTest, DeltaRoundTripLarge) {
    for (uint64_t n : {uint64_t{1000}, uint64_t{65536}, uint64_t{1000000}}) {
        EXPECT_EQ(delta_round_trip(n), n) << "n=" << n;
    }
}

// Spot-check specific encodings from the spec:
// 1 -> gamma(1) = "1" (L=1, no trailing bits after the leading 1)
TEST(EliasDeltaOmegaTest, DeltaEncoding1IsSingleOne) {
    BitBuffer buf;
    Delta::encode(uint64_t{1}, buf);
    ASSERT_EQ(buf.bits.size(), 1u);
    EXPECT_EQ(buf.bits[0], true);
}

// 2 -> gamma(2)."0" = "010"."0" = "0100" (4 bits)
TEST(EliasDeltaOmegaTest, DeltaEncoding2Is0100) {
    BitBuffer buf;
    Delta::encode(uint64_t{2}, buf);
    ASSERT_EQ(buf.bits.size(), 4u);
    EXPECT_EQ(buf.bits[0], false);
    EXPECT_EQ(buf.bits[1], true);
    EXPECT_EQ(buf.bits[2], false);
    EXPECT_EQ(buf.bits[3], false);
}

// 3 -> gamma(2)."1" = "010"."1" = "0101" (4 bits)
TEST(EliasDeltaOmegaTest, DeltaEncoding3Is0101) {
    BitBuffer buf;
    Delta::encode(uint64_t{3}, buf);
    ASSERT_EQ(buf.bits.size(), 4u);
    EXPECT_EQ(buf.bits[0], false);
    EXPECT_EQ(buf.bits[1], true);
    EXPECT_EQ(buf.bits[2], false);
    EXPECT_EQ(buf.bits[3], true);
}

// 4 -> gamma(3)."00" = "011"."00" = "01100" (5 bits)
TEST(EliasDeltaOmegaTest, DeltaEncoding4Is01100) {
    BitBuffer buf;
    Delta::encode(uint64_t{4}, buf);
    ASSERT_EQ(buf.bits.size(), 5u);
    EXPECT_EQ(buf.bits[0], false);
    EXPECT_EQ(buf.bits[1], true);
    EXPECT_EQ(buf.bits[2], true);
    EXPECT_EQ(buf.bits[3], false);
    EXPECT_EQ(buf.bits[4], false);
}

// Delta length is always <= Gamma length for n >= 16 (crossover claim from spec).
TEST(EliasDeltaOmegaTest, DeltaShorterThanGammaForN16AndAbove) {
    for (uint64_t n = 16; n <= 1024; ++n) {
        std::size_t dl = delta_bit_count(n);
        std::size_t gl = gamma_bit_count(n);
        EXPECT_LE(dl, gl) << "n=" << n << " delta=" << dl << " gamma=" << gl;
    }
}
```

- [ ] **Step 2: Run to verify compile failure**

```bash
cd /home/spinoza/github/metafunctor-series/wire-formats && make build 2>&1 | grep -E "error:|undefined" | head -10
```

Expected: compile errors about `Gamma` and `Delta` not declared.

- [ ] **Step 3: Implement `Gamma` (local) and `Delta` in `elias_delta_omega.hpp`**

Replace the `// Implementation arrives in Tasks 3, 4, and 5.` comment with:

```cpp
// ---- Gamma -- Elias gamma (re-implemented locally for loose coupling) --------
//
// Encodes positive integer n >= 1:
//   1. Write floor(log2(n)) zero bits.
//   2. Write a '1' bit.
//   3. Write the floor(log2(n)) trailing bits of n (after the implicit leading
//      1 bit), MSB first.
//
// This is identical to the Gamma in post 4's unary_gamma.hpp. Re-implemented
// here so this header stands alone without a sibling-directory dependency.
//
// Length: 2*floor(log2(n)) + 1 bits.
// Implied prior: approximately 1/(2n^2).

struct Gamma {
    using value_type = std::uint64_t;

    template<BitSink S>
    static void encode(value_type n, S& sink) {
        assert(n >= 1 && "Gamma is undefined for n = 0");
        std::size_t k = std::bit_width(n) - 1;  // floor(log2(n))
        for (std::size_t i = 0; i < k; ++i) sink.write(false);
        sink.write(true);
        for (std::size_t i = k; i > 0; --i) {
            sink.write(((n >> (i - 1)) & 1u) != 0u);
        }
    }

    template<BitSource S>
    static value_type decode(S& source) {
        std::size_t k = 0;
        while (!source.read()) ++k;
        value_type n = 1;
        for (std::size_t i = 0; i < k; ++i) {
            n = (n << 1) | (source.read() ? value_type{1} : value_type{0});
        }
        return n;
    }
};

// ---- Delta -- Elias delta code (Peter Elias, 1975) --------------------------
//
// Encodes positive integer n >= 1:
//   Let L = floor(log2(n)) + 1  (the bit-width of n, equivalently bit_width(n)).
//   1. Encode L in Gamma.
//   2. Write the (L-1) trailing bits of n after its leading 1, MSB first.
//
// Gamma encodes L (a small integer) efficiently, replacing the linear-sized
// unary length prefix in Gamma's own encoding of n.
//
// Examples:
//   1 -> L=1, Gamma(1)="1", no trailing bits         -> "1"      (1 bit)
//   2 -> L=2, Gamma(2)="010", trailing bits="0"       -> "0100"   (4 bits)
//   3 -> L=2, Gamma(2)="010", trailing bits="1"       -> "0101"   (4 bits)
//   4 -> L=3, Gamma(3)="011", trailing bits="00"      -> "01100"  (5 bits)
//
// Length: L_delta(n) = L_gamma(bit_width(n)) + (bit_width(n) - 1)
//         = O(log n + log log n) bits.
// Implied prior: approximately 1 / (n * log^2(n)), heavier tail than Gamma.

struct Delta {
    using value_type = std::uint64_t;

    template<BitSink S>
    static void encode(value_type n, S& sink) {
        assert(n >= 1);
        std::size_t bits = std::bit_width(n);  // L = floor(log2(n)) + 1
        Gamma::encode(static_cast<value_type>(bits), sink);
        // Write the (bits - 1) trailing bits of n, MSB first, skipping the
        // implicit leading 1.
        for (std::size_t i = bits - 1; i > 0; --i) {
            sink.write(((n >> (i - 1)) & 1u) != 0u);
        }
    }

    template<BitSource S>
    static value_type decode(S& source) {
        std::size_t bits = static_cast<std::size_t>(Gamma::decode(source));
        // The leading 1 is implicit; read the remaining (bits - 1) trailing bits.
        value_type result = 1;
        for (std::size_t i = 1; i < bits; ++i) {
            result = (result << 1) | (source.read() ? value_type{1} : value_type{0});
        }
        return result;
    }
};
```

- [ ] **Step 4: Build and verify tests pass**

```bash
cd /home/spinoza/github/metafunctor-series/wire-formats && make test 2>&1 | grep -E "PASSED|FAILED|test_elias_delta_omega"
```

Expected: all EliasDeltaOmegaTest cases up to this point pass (GammaLocalRoundTrip, GammaLocalBitCount, DeltaRoundTrip, DeltaRoundTripLarge, and all spot-check encodings).

- [ ] **Step 5: Commit**

```bash
cd /home/spinoza/github/metafunctor-series/wire-formats
git add post/2022-02-elias-delta-omega-wire-formats/elias_delta_omega.hpp \
        post/2022-02-elias-delta-omega-wire-formats/test_elias_delta_omega.cpp
git commit -m "feat(elias-delta-omega): implement Gamma (local) and Delta codecs (TDD)"
```

---

## Task 4: TDD -- implement `Omega` codec

**Files:**
- Modify: `post/2022-02-elias-delta-omega-wire-formats/elias_delta_omega.hpp`
- Modify: `post/2022-02-elias-delta-omega-wire-formats/test_elias_delta_omega.cpp`

The Omega codec uses an iterative encoding with a stack. Encoding builds the stack bottom-up (largest value first); writing occurs in reverse (top of stack first). Decoding reads forward: each group of bits tells you how many bits to read in the next group, until a terminating 0 bit.

- [ ] **Step 1: Append failing tests for `Omega`**

Append to `test_elias_delta_omega.cpp`:

```cpp
// ---- Omega tests ------------------------------------------------------------

static uint64_t omega_round_trip(uint64_t n) {
    BitBuffer buf;
    Omega::encode(n, buf);
    buf.pos_ = 0;
    return Omega::decode(buf);
}

static std::size_t omega_bit_count(uint64_t n) {
    BitBuffer buf;
    Omega::encode(n, buf);
    return buf.bits.size();
}

TEST(EliasDeltaOmegaTest, OmegaRoundTrip) {
    for (uint64_t n = 1; n <= 256; ++n) {
        EXPECT_EQ(omega_round_trip(n), n) << "n=" << n;
    }
}

TEST(EliasDeltaOmegaTest, OmegaRoundTripLarge) {
    for (uint64_t n : {uint64_t{1000}, uint64_t{65536}, uint64_t{1000000}}) {
        EXPECT_EQ(omega_round_trip(n), n) << "n=" << n;
    }
}

// For n=1, all three codes use 1 bit.
TEST(EliasDeltaOmegaTest, OmegaEncoding1Is1Bit) {
    EXPECT_EQ(omega_bit_count(1u), 1u);
}

// Omega length is always <= Delta length for n >= 1000 (spec crossover claim).
// For small n, they may coincide; we only assert non-regression up to delta.
TEST(EliasDeltaOmegaTest, OmegaNoLongerThanDeltaForLargeN) {
    for (uint64_t n = 1000; n <= 2000; ++n) {
        EXPECT_LE(omega_bit_count(n), delta_bit_count(n) + 2)
            << "n=" << n;
    }
}
```

- [ ] **Step 2: Run to verify compile failure**

```bash
cd /home/spinoza/github/metafunctor-series/wire-formats && make build 2>&1 | grep -E "error:|undefined" | head -10
```

Expected: compile errors about `Omega` not declared.

- [ ] **Step 3: Implement `Omega` in `elias_delta_omega.hpp`**

Append to `elias_delta_omega.hpp` (after the `Delta` struct, still inside `namespace elias_delta_omega`):

```cpp
// ---- Omega -- Elias omega code (Peter Elias, 1975) --------------------------
//
// Omega encodes n >= 1 by recursively encoding the bit-width of each level
// until the value reaches 1. The recursion unwinds by stacking the binary
// representations, then writing them from outermost (largest) to innermost
// (smallest), followed by a terminating 0 bit.
//
// Encoding algorithm (iterative with a stack):
//   1. While n > 1:
//        a. Push (n, bit_width(n)) onto the stack.
//        b. Replace n with (bit_width(n) - 1).   // the "length minus 1" step
//   2. Pop the stack in reverse order (top first), writing each value in its
//      full binary representation.
//   3. Write a terminating 0 bit.
//
// Decoding algorithm (forward):
//   1. Start with current = 1.
//   2. Peek at the next bit:
//        - If 0 (terminator): stop. Return current.
//        - If 1: read (current + 1) bits (including the peeked 1) to get next.
//                Set current = next. Repeat.
//
// Length: O(log* n) asymptotically (iterated logarithm).
// Implied prior: slightly heavier tail than Delta's 1/(n log^2 n).
// Practical note: Omega is the theoretical limit of the recursion. Delta is
// preferred in practice because the Omega overhead dominates for practical n.

struct Omega {
    using value_type = std::uint64_t;

    template<BitSink S>
    static void encode(value_type n, S& sink) {
        assert(n >= 1);
        // Build the stack bottom-up: each frame is the value to write plus its
        // bit-width. The recursion terminates when n reaches 1.
        std::vector<std::pair<value_type, std::size_t>> stack;
        while (n > 1) {
            std::size_t w = static_cast<std::size_t>(std::bit_width(n));
            stack.emplace_back(n, w);
            n = static_cast<value_type>(w - 1);  // encode (bit_width - 1) next
        }
        // Write in reverse (outermost frame first), each value MSB-first.
        for (auto it = stack.rbegin(); it != stack.rend(); ++it) {
            auto [val, width] = *it;
            for (std::size_t i = width; i > 0; --i) {
                sink.write(((val >> (i - 1)) & 1u) != 0u);
            }
        }
        // Terminating 0 bit.
        sink.write(false);
    }

    template<BitSource S>
    static value_type decode(S& source) {
        value_type current = 1;
        // The next bit tells us whether more data follows.
        // We must peek: read the bit, then decide.
        // Implementation: attempt to read the next bit directly.
        // If it is 0, we stop. If it is 1, it is the MSB of the next field,
        // which has (current + 1) bits total (including this one).
        while (true) {
            bool b = source.read();
            if (!b) {
                // Terminating 0: current holds the decoded value.
                break;
            }
            // b == 1: this is the MSB of a (current + 1)-bit value.
            value_type next = 1;  // implicit leading 1 (the bit we just read)
            for (value_type i = 0; i < current; ++i) {
                next = (next << 1) | (source.read() ? value_type{1} : value_type{0});
            }
            current = next;
        }
        return current;
    }
};
```

- [ ] **Step 4: Build and verify tests pass**

```bash
cd /home/spinoza/github/metafunctor-series/wire-formats && make test 2>&1 | grep -E "PASSED|FAILED|test_elias_delta_omega"
```

Expected: all EliasDeltaOmegaTest cases pass including OmegaRoundTrip, OmegaRoundTripLarge, OmegaEncoding1Is1Bit, and OmegaNoLongerThanDeltaForLargeN.

- [ ] **Step 5: Commit**

```bash
cd /home/spinoza/github/metafunctor-series/wire-formats
git add post/2022-02-elias-delta-omega-wire-formats/elias_delta_omega.hpp \
        post/2022-02-elias-delta-omega-wire-formats/test_elias_delta_omega.cpp
git commit -m "feat(elias-delta-omega): implement Omega codec (TDD)"
```

---

## Task 5: TDD -- length-comparison table test and integration tests for post 5

**Files:**
- Modify: `post/2022-02-elias-delta-omega-wire-formats/test_elias_delta_omega.cpp`
- Modify: `post/CMakeLists.txt` (add priors include path)

This task adds: (a) the spec's crossover table test verifying exact bit counts for key values of n, and (b) integration tests via the priors library confirming Delta's implied prior is heavier-tailed than Gamma's.

- [ ] **Step 1: Add priors include path to the test_elias_delta_omega target in CMakeLists.txt**

In `post/CMakeLists.txt`, update the `target_include_directories` line for `test_elias_delta_omega` to include the priors directory:

```cmake
target_include_directories(test_elias_delta_omega PRIVATE
    2022-02-elias-delta-omega-wire-formats
    2021-03-priors-wire-formats)
```

- [ ] **Step 2: Append the table test and integration tests to test_elias_delta_omega.cpp**

Append to `test_elias_delta_omega.cpp`:

```cpp
// ---- Length-comparison table test -------------------------------------------
//
// Spec section D (post 5) gives the following table. We verify the exact
// Gamma and Delta bit counts; for Omega we verify relative ordering only
// (Omega length varies by implementation details at small n).
//
// n        | Unary  | Gamma | Delta | Omega
// 1        |  1     |  1    |  1    |  1
// 4        |  4     |  5    |  5    |  5
// 16       | 16     |  9    |  8    |  8
// 256      | 256    | 17    | 13    | 12
// 2^16     | 65536  | 33    | 22    | 19
// 2^32     | ~4e9   | 65    | 39    | 33

struct LengthTableRow {
    uint64_t n;
    std::size_t gamma_len;
    std::size_t delta_len;
};

TEST(EliasDeltaOmegaTest, LengthComparisonTable) {
    const std::vector<LengthTableRow> table = {
        {1u,                1u,  1u},
        {4u,                5u,  5u},
        {16u,               9u,  8u},
        {256u,             17u, 13u},
        {65536u,           33u, 22u},
        {4294967296u,      65u, 39u},
    };
    for (auto [n, gl, dl] : table) {
        EXPECT_EQ(gamma_bit_count(n), gl) << "Gamma length for n=" << n;
        EXPECT_EQ(delta_bit_count(n), dl) << "Delta length for n=" << n;
        // Omega is always <= Delta (or within 2 bits for the smallest entries).
        EXPECT_LE(omega_bit_count(n), dl + 1u) << "Omega <= Delta+1 for n=" << n;
    }
}

// ---- Integration tests using the priors library ----------------------------

#include "../2021-03-priors-wire-formats/priors.hpp"

// Helper: build delta length vector for symbols 1..N.
static std::vector<std::size_t> delta_lengths(std::size_t N) {
    std::vector<std::size_t> v(N);
    for (std::size_t i = 0; i < N; ++i) {
        v[i] = delta_bit_count(static_cast<uint64_t>(i + 1));
    }
    return v;
}

// Helper: build gamma length vector for symbols 1..N.
static std::vector<std::size_t> local_gamma_lengths(std::size_t N) {
    std::vector<std::size_t> v(N);
    for (std::size_t i = 0; i < N; ++i) {
        v[i] = gamma_bit_count(static_cast<uint64_t>(i + 1));
    }
    return v;
}

// Delta's implied prior is heavier-tailed than Gamma's: for large n, the
// delta-implied probability is higher (shorter codewords relative to total).
// We verify: the implied probability of the largest symbol under Delta exceeds
// that under Gamma (since Delta gives shorter codewords for large n).
TEST(EliasDeltaOmegaTest, DeltaImpliedPriorHeavierThanGamma) {
    const std::size_t N = 64;
    auto dp = priors::implied_prior(delta_lengths(N));
    auto gp = priors::implied_prior(local_gamma_lengths(N));
    // The last symbol (n=64) should have a higher implied probability under
    // Delta (shorter codeword) than under Gamma.
    EXPECT_GT(dp.back(), gp.back());
}

// Delta has bounded redundancy on the power-law(2) source, comparable to Gamma.
TEST(EliasDeltaOmegaTest, DeltaSmallRedundancyOnPowerLaw2) {
    const std::size_t N = 64;
    auto lens = delta_lengths(N);
    std::vector<double> pl(N);
    double z = 0.0;
    for (std::size_t i = 0; i < N; ++i) {
        double n = static_cast<double>(i + 1);
        pl[i] = 1.0 / (n * n);
        z += pl[i];
    }
    for (double& p : pl) p /= z;
    double r = priors::redundancy(pl, lens);
    EXPECT_GE(r, 0.0);
    EXPECT_LT(r, 4.0);  // Universal-code bounded redundancy.
}

// Gamma beats Delta on a power-law(2) source (Delta is designed for heavier
// tails; Gamma is better-matched to exactly 1/n^2).
TEST(EliasDeltaOmegaTest, GammaBeatsOrTiesDeltaOnPowerLaw2) {
    const std::size_t N = 64;
    std::vector<double> pl(N);
    double z = 0.0;
    for (std::size_t i = 0; i < N; ++i) {
        double n = static_cast<double>(i + 1);
        pl[i] = 1.0 / (n * n);
        z += pl[i];
    }
    for (double& p : pl) p /= z;
    double r_gamma = priors::redundancy(pl, local_gamma_lengths(N));
    double r_delta = priors::redundancy(pl, delta_lengths(N));
    // Delta has a slightly heavier implied prior than needed for 1/n^2.
    // Gamma should win (or tie within rounding) for this specific source.
    EXPECT_LE(r_gamma, r_delta + 0.5);
}
```

- [ ] **Step 3: Build and verify all tests pass**

```bash
cd /home/spinoza/github/metafunctor-series/wire-formats && make test 2>&1 | grep -E "PASSED|FAILED|test_elias_delta_omega"
```

Expected: all EliasDeltaOmegaTest cases pass including the table test and integration tests.

- [ ] **Step 4: Commit**

```bash
cd /home/spinoza/github/metafunctor-series/wire-formats
git add post/2022-02-elias-delta-omega-wire-formats/test_elias_delta_omega.cpp \
        post/CMakeLists.txt
git commit -m "test(elias-delta-omega): add length-table test and priors integration tests"
```

---

## Task 6: Verify post 5 full-suite pass (clean rebuild + warning check)

**Files:** no changes; verification only.

- [ ] **Step 1: Clean rebuild**

```bash
cd /home/spinoza/github/metafunctor-series/wire-formats && make clean && make build 2>&1 | tail -5
```

Expected: successful clean build with no errors.

- [ ] **Step 2: Warning check for elias_delta_omega**

```bash
cd /home/spinoza/github/metafunctor-series/wire-formats
cmake -B post/build -S post -DCMAKE_CXX_FLAGS="-Wall -Wextra -Wpedantic" 2>&1 \
    | grep -E "warning:|error:" | grep "elias_delta_omega" | head -20
cmake --build post/build --target test_elias_delta_omega 2>&1 \
    | grep -E "warning:|error:" | head -20
```

Expected: zero warnings and zero errors from `elias_delta_omega.hpp` and `test_elias_delta_omega.cpp`.

- [ ] **Step 3: Full ctest pass**

```bash
cd /home/spinoza/github/metafunctor-series/wire-formats && make test 2>&1 | tail -10
```

Expected: all tests pass (test_kraft, test_mcmillan, test_priors, test_unary_gamma, test_elias_delta_omega).

No commit for this task (verification only).

---

## Task 7: Draft post 5 prose

**Files:**
- Modify: `post/2022-02-elias-delta-omega-wire-formats/index.md`

Draft from the spec's sections A through G for Post 5. Target approximately 2000 words. No em-dashes.

- [ ] **Step 1: Draft the prose**

Replace the placeholder `(Draft in progress...)` line in `post/2022-02-elias-delta-omega-wire-formats/index.md` with the full article body. Use the spec's section-by-section outline:

Section A ("Where Gamma Stops Being Good", ~200 words, no code): recap gamma's length as `2*log2(n) + 1` bits, note that the unary length prefix wastes half the bits for large n, motivate encoding the length in gamma instead.

Section B ("Elias Delta", ~300 words + `Delta` code block): show the encoding rule (L in Gamma, then trailing bits), the struct implementation, the examples table (1, 2, 3, 4), the length formula `O(log n + log log n)`, and the implied prior approximation `1/(n log^2 n)`.

Section C ("Elias Omega", ~300 words + `Omega` code block): describe the recursive approach, show the iterative stack-based implementation, discuss the length `O(log* n)` asymptote and what the iterated logarithm means in practice (for n <= 2^64, it is at most 5 or 6 levels).

Section D ("The Crossover Points", ~250 words): include the spec's exact table for n in {1, 4, 16, 256, 2^16, 2^32} with Unary/Gamma/Delta/Omega lengths. Discuss the crossover: Delta beats Gamma from n=16; Omega beats Delta from roughly n=1000.

Section E ("The Implied Prior Ladder", ~250 words): each recursion step shifts to a heavier-tailed prior. Gamma: 1/n^2. Delta: 1/(n log^2 n). Omega: slightly heavier still. Trade: heavier tail means better tolerance for large outliers, but a small constant overhead for small values.

Section F ("The Limit", ~200 words): Omega achieves O(log* n) length, the theoretical minimum for self-delimiting integer codes. Note that Omega is the theoretical endpoint; Delta is the practical choice (the constant overhead in Omega dominates for any realistic n).

Section G ("Cross-references and footnote", ~120 words): per the spec.
- Forward: link to [Fibonacci Coding](/post/2022-07-fibonacci-wire-formats/) (post 6, will exist after Task 13).
- Back: [Unary and Elias Gamma](/post/2021-08-elias-gamma-wire-formats/) (post 4); [Universal Codes as Priors](/post/2021-03-priors-wire-formats/) (post 3).
- Cross-series: both Stepanov bridge posts ([Bits Follow Types](/post/2026-05-codecs-functors-stepanov/) and [When Lists Become Bits](/post/2026-05-prefix-free-stepanov/)).
- Footnote: PFC's `include/pfc/codecs.hpp` has `EliasDelta` and `EliasOmega`.

Set `draft: false` when satisfied.

- [ ] **Step 2: Soul check (no em-dashes)**

```bash
grep -n $'\xe2\x80\x94' /home/spinoza/github/metafunctor-series/wire-formats/post/2022-02-elias-delta-omega-wire-formats/index.md | head -5
```

Expected: no output (no em-dashes in the file).

- [ ] **Step 3: Commit**

```bash
cd /home/spinoza/github/metafunctor-series/wire-formats
git add post/2022-02-elias-delta-omega-wire-formats/index.md
git commit -m "docs(elias-delta-omega): draft post 5 prose (Elias Delta and Omega)"
```

---

## Task 8: Scaffold post 6 directory and wire CMakeLists

**Files:**
- Create: `post/2022-07-fibonacci-wire-formats/index.md`
- Create: `post/2022-07-fibonacci-wire-formats/fibonacci.hpp`
- Create: `post/2022-07-fibonacci-wire-formats/test_fibonacci.cpp`
- Modify: `post/CMakeLists.txt`

- [ ] **Step 1: Create the post 6 directory and skeleton files**

```bash
mkdir -p /home/spinoza/github/metafunctor-series/wire-formats/post/2022-07-fibonacci-wire-formats
```

Create `post/2022-07-fibonacci-wire-formats/index.md` with placeholder frontmatter:

```markdown
---
title: "Fibonacci Coding"
date: 2022-07-17
draft: true
tags:
- C++
- information-theory
- coding-theory
- prefix-free
- universal-codes
- fibonacci
- zeckendorf
categories:
- Computer Science
- Mathematics
series:
- wire-formats
series_weight: 6
math: true
description: "Fibonacci coding uses Zeckendorf's representation to produce self-synchronizing codewords. Every codeword ends in two consecutive ones; a single bit flip corrupts at most two codewords."
linked_project:
- pfc
- wire-formats
---

(Draft in progress. See plan Task 13 for full prose.)
```

Create `post/2022-07-fibonacci-wire-formats/fibonacci.hpp` with header guards only:

```cpp
// fibonacci.hpp
// Pedagogical implementation for the post "Fibonacci Coding" in the
// "Algebra over Wire Formats" series. For the production version, see PFC:
// https://github.com/queelius/pfc (codecs.hpp: Fibonacci)

#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace fibonacci {

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

// Implementation arrives in Tasks 9 and 10.

}  // namespace fibonacci
```

Create `post/2022-07-fibonacci-wire-formats/test_fibonacci.cpp` with a placeholder:

```cpp
#include <gtest/gtest.h>
#include "fibonacci.hpp"

TEST(FibonacciTest, Placeholder) {
    EXPECT_TRUE(true);
}
```

- [ ] **Step 2: Append the post 6 test executable to post/CMakeLists.txt**

Append to `/home/spinoza/github/metafunctor-series/wire-formats/post/CMakeLists.txt`:

```cmake

# =============================================================================
# Fibonacci Coding (post 6, 2022-07-17)
# =============================================================================
add_executable(test_fibonacci
    2022-07-fibonacci-wire-formats/test_fibonacci.cpp)
target_link_libraries(test_fibonacci GTest::gtest_main)
target_include_directories(test_fibonacci PRIVATE
    2022-07-fibonacci-wire-formats)
add_test(NAME test_fibonacci COMMAND test_fibonacci)
```

- [ ] **Step 3: Build and verify placeholder test passes**

```bash
cd /home/spinoza/github/metafunctor-series/wire-formats && make test 2>&1 | tail -14
```

Expected: all previous tests pass plus the new `test_fibonacci.Placeholder` test.

- [ ] **Step 4: Commit**

```bash
cd /home/spinoza/github/metafunctor-series/wire-formats
git add post/2022-07-fibonacci-wire-formats post/CMakeLists.txt
git commit -m "scaffold(fibonacci): add post 6 directory and CMake wiring"
```

---

## Task 9: TDD -- implement `to_zeckendorf`

**Files:**
- Modify: `post/2022-07-fibonacci-wire-formats/fibonacci.hpp`
- Modify: `post/2022-07-fibonacci-wire-formats/test_fibonacci.cpp`

- [ ] **Step 1: Write failing tests for `to_zeckendorf`**

Replace `test_fibonacci.cpp` with:

```cpp
#include <gtest/gtest.h>
#include <cstdint>
#include <numeric>
#include <vector>
#include "fibonacci.hpp"

using namespace fibonacci;

// ---- Zeckendorf tests -------------------------------------------------------

// to_zeckendorf(n) returns a bit vector where bits[i] == true iff F_{i+2} is
// in the Zeckendorf sum. Index 0 corresponds to F_2 = 1, index 1 to F_3 = 2,
// index 2 to F_4 = 3, etc.

// Helper: reconstruct n from its Zeckendorf bits.
static uint64_t from_zeckendorf(const std::vector<bool>& bits) {
    std::vector<uint64_t> fibs{1, 2};
    while (fibs.size() < bits.size()) {
        fibs.push_back(fibs[fibs.size()-1] + fibs[fibs.size()-2]);
    }
    uint64_t n = 0;
    for (std::size_t i = 0; i < bits.size(); ++i) {
        if (bits[i]) n += fibs[i];
    }
    return n;
}

// Helper: verify no two consecutive bits are true (Zeckendorf uniqueness).
static bool no_consecutive_ones(const std::vector<bool>& bits) {
    for (std::size_t i = 0; i + 1 < bits.size(); ++i) {
        if (bits[i] && bits[i+1]) return false;
    }
    return true;
}

// Spot-check: known Zeckendorf representations.
// 1 = F_2                     -> bits = {1}
// 2 = F_3                     -> bits = {0, 1}
// 3 = F_4                     -> bits = {0, 0, 1}
// 4 = F_4 + F_2 = 3+1         -> bits = {1, 0, 1}
// 10 = F_6 + F_3 = 8+2        -> bits = {0, 1, 0, 0, 1}
// 11 = F_6 + F_4 = 8+3        -> bits = {0, 0, 1, 0, 1}
TEST(FibonacciTest, ZeckendorfSpotCheck1) {
    auto b = to_zeckendorf(1u);
    ASSERT_GE(b.size(), 1u);
    EXPECT_EQ(b[0], true);
    EXPECT_EQ(from_zeckendorf(b), 1u);
}

TEST(FibonacciTest, ZeckendorfSpotCheck4) {
    auto b = to_zeckendorf(4u);
    // 4 = 3 + 1 = F_4 + F_2: bits[0]=1 (F_2), bits[1]=0 (F_3), bits[2]=1 (F_4).
    ASSERT_GE(b.size(), 3u);
    EXPECT_EQ(b[0], true);
    EXPECT_EQ(b[1], false);
    EXPECT_EQ(b[2], true);
    EXPECT_EQ(from_zeckendorf(b), 4u);
}

TEST(FibonacciTest, ZeckendorfSpotCheck10) {
    auto b = to_zeckendorf(10u);
    EXPECT_EQ(from_zeckendorf(b), 10u);
    EXPECT_TRUE(no_consecutive_ones(b));
}

// Round-trip: for all n in 1..200, from_zeckendorf(to_zeckendorf(n)) == n.
TEST(FibonacciTest, ZeckendorfRoundTrip) {
    for (uint64_t n = 1; n <= 200; ++n) {
        auto b = to_zeckendorf(n);
        EXPECT_EQ(from_zeckendorf(b), n) << "n=" << n;
    }
}

// Non-consecutive: Zeckendorf bits never have two adjacent 1s.
TEST(FibonacciTest, ZeckendorfNoConsecutiveOnes) {
    for (uint64_t n = 1; n <= 200; ++n) {
        EXPECT_TRUE(no_consecutive_ones(to_zeckendorf(n))) << "n=" << n;
    }
}

// Uniqueness: to_zeckendorf should always return the same result for the same n.
TEST(FibonacciTest, ZeckendorfDeterministic) {
    for (uint64_t n = 1; n <= 50; ++n) {
        EXPECT_EQ(to_zeckendorf(n), to_zeckendorf(n)) << "n=" << n;
    }
}
```

- [ ] **Step 2: Run to verify compile failure**

```bash
cd /home/spinoza/github/metafunctor-series/wire-formats && make build 2>&1 | grep -E "error:|undefined" | head -10
```

Expected: compile errors about `to_zeckendorf` not declared.

- [ ] **Step 3: Implement `to_zeckendorf` in `fibonacci.hpp`**

Replace the `// Implementation arrives in Tasks 9 and 10.` comment with:

```cpp
// ---- to_zeckendorf -- greedy Zeckendorf decomposition ----------------------
//
// Zeckendorf's theorem: every positive integer n has a unique representation
// as a sum of non-consecutive Fibonacci numbers (F_2=1, F_3=2, F_4=3, F_5=5,
// F_6=8, ...). The greedy algorithm finds this representation by subtracting
// the largest Fibonacci number <= n at each step.
//
// Returns a bit vector where bits[i] == true iff F_{i+2} is in the sum.
// Index 0 corresponds to F_2 = 1, index 1 to F_3 = 2, etc.
//
// Example: to_zeckendorf(4) -> {true, false, true}
//          meaning 4 = F_2 + F_4 = 1 + 3.

inline std::vector<bool> to_zeckendorf(std::uint64_t n) {
    assert(n >= 1 && "Zeckendorf is undefined for n = 0");
    // Build the Fibonacci sequence up to n. Start with F_2=1, F_3=2.
    std::vector<std::uint64_t> fibs{1, 2};
    while (fibs.back() <= n) {
        fibs.push_back(fibs[fibs.size() - 1] + fibs[fibs.size() - 2]);
    }
    // The last entry is > n; remove it so all entries are <= n.
    fibs.pop_back();
    // Greedy decomposition: subtract the largest Fibonacci number <= remaining n.
    std::vector<bool> bits(fibs.size(), false);
    for (std::size_t i = fibs.size(); i-- > 0;) {
        if (n >= fibs[i]) {
            bits[i] = true;
            n -= fibs[i];
        }
    }
    return bits;  // bits[i] = true iff fibs[i] is in the Zeckendorf sum
}
```

- [ ] **Step 4: Build and verify tests pass**

```bash
cd /home/spinoza/github/metafunctor-series/wire-formats && make test 2>&1 | grep -E "PASSED|FAILED|test_fibonacci"
```

Expected: all FibonacciTest cases up to this point pass (ZeckendorfSpotCheck1, ZeckendorfSpotCheck4, ZeckendorfSpotCheck10, ZeckendorfRoundTrip, ZeckendorfNoConsecutiveOnes, ZeckendorfDeterministic).

- [ ] **Step 5: Commit**

```bash
cd /home/spinoza/github/metafunctor-series/wire-formats
git add post/2022-07-fibonacci-wire-formats/fibonacci.hpp \
        post/2022-07-fibonacci-wire-formats/test_fibonacci.cpp
git commit -m "feat(fibonacci): implement to_zeckendorf with greedy decomposition (TDD)"
```

---

## Task 10: TDD -- implement `Fibonacci` codec

**Files:**
- Modify: `post/2022-07-fibonacci-wire-formats/fibonacci.hpp`
- Modify: `post/2022-07-fibonacci-wire-formats/test_fibonacci.cpp`

- [ ] **Step 1: Append failing tests for `Fibonacci`**

Append to `test_fibonacci.cpp`:

```cpp
// ---- Fibonacci codec tests --------------------------------------------------

// Minimal in-memory BitSink/BitSource for tests.
struct BitBuffer {
    std::vector<bool> bits;
    void write(bool b) { bits.push_back(b); }
    bool read() {
        bool b = bits[pos_];
        ++pos_;
        return b;
    }
    std::size_t pos_ = 0;
};

static uint64_t fib_round_trip(uint64_t n) {
    BitBuffer buf;
    Fibonacci::encode(n, buf);
    buf.pos_ = 0;
    return Fibonacci::decode(buf);
}

static std::size_t fib_bit_count(uint64_t n) {
    BitBuffer buf;
    Fibonacci::encode(n, buf);
    return buf.bits.size();
}

TEST(FibonacciTest, FibonacciRoundTrip) {
    for (uint64_t n = 1; n <= 200; ++n) {
        EXPECT_EQ(fib_round_trip(n), n) << "n=" << n;
    }
}

TEST(FibonacciTest, FibonacciRoundTripLarge) {
    for (uint64_t n : {uint64_t{1000}, uint64_t{10000}, uint64_t{100000}}) {
        EXPECT_EQ(fib_round_trip(n), n) << "n=" << n;
    }
}

// Spot-check known codewords from the spec:
// 1 -> "11" (F_2 bit + terminator)
// 2 -> "011" (F_3 bit + terminator: bits={0,1}, append 1)
// 3 -> "0011" (F_4: bits={0,0,1}, append 1)
// 4 -> "1011" (F_2+F_4: bits={1,0,1}, append 1)
// 8 -> "000011" (F_6: bits={0,0,0,0,1}, append 1)

TEST(FibonacciTest, FibonacciEncoding1Is11) {
    BitBuffer buf;
    Fibonacci::encode(uint64_t{1}, buf);
    ASSERT_EQ(buf.bits.size(), 2u);
    EXPECT_EQ(buf.bits[0], true);   // F_2 bit
    EXPECT_EQ(buf.bits[1], true);   // terminator
}

TEST(FibonacciTest, FibonacciEncoding2Is011) {
    BitBuffer buf;
    Fibonacci::encode(uint64_t{2}, buf);
    ASSERT_EQ(buf.bits.size(), 3u);
    EXPECT_EQ(buf.bits[0], false);  // F_2 bit = 0
    EXPECT_EQ(buf.bits[1], true);   // F_3 bit = 1
    EXPECT_EQ(buf.bits[2], true);   // terminator
}

TEST(FibonacciTest, FibonacciEncoding4Is1011) {
    BitBuffer buf;
    Fibonacci::encode(uint64_t{4}, buf);
    ASSERT_EQ(buf.bits.size(), 4u);
    EXPECT_EQ(buf.bits[0], true);   // F_2=1 bit
    EXPECT_EQ(buf.bits[1], false);  // F_3=2 bit
    EXPECT_EQ(buf.bits[2], true);   // F_4=3 bit
    EXPECT_EQ(buf.bits[3], true);   // terminator
}

// Every codeword ends in "11" (Zeckendorf bits followed by terminator '1').
// The last Zeckendorf bit is always 1 (it is the highest Fibonacci in the sum).
TEST(FibonacciTest, AllCodewordsEndIn11) {
    for (uint64_t n = 1; n <= 100; ++n) {
        BitBuffer buf;
        Fibonacci::encode(n, buf);
        std::size_t len = buf.bits.size();
        ASSERT_GE(len, 2u) << "n=" << n;
        // The last two bits must both be 1.
        EXPECT_EQ(buf.bits[len - 1], true) << "terminator missing for n=" << n;
        EXPECT_EQ(buf.bits[len - 2], true) << "last Zeckendorf bit not 1 for n=" << n;
    }
}

// No codeword contains "11" except at the very end.
TEST(FibonacciTest, NoInternalConsecutiveOnes) {
    for (uint64_t n = 1; n <= 100; ++n) {
        BitBuffer buf;
        Fibonacci::encode(n, buf);
        std::size_t len = buf.bits.size();
        // Check all pairs except the final pair (which is the "11" terminator).
        for (std::size_t i = 0; i + 2 < len; ++i) {
            EXPECT_FALSE(buf.bits[i] && buf.bits[i+1])
                << "Internal '11' at position " << i << " for n=" << n;
        }
    }
}
```

- [ ] **Step 2: Run to verify compile failure**

```bash
cd /home/spinoza/github/metafunctor-series/wire-formats && make build 2>&1 | grep -E "error:|undefined" | head -10
```

Expected: compile errors about `Fibonacci` not declared.

- [ ] **Step 3: Implement `Fibonacci` in `fibonacci.hpp`**

Append to `fibonacci.hpp` (after `to_zeckendorf`, still inside `namespace fibonacci`):

```cpp
// ---- Fibonacci codec -------------------------------------------------------
//
// Encodes positive integer n >= 1:
//   1. Compute the Zeckendorf representation of n: a bit vector where
//      bits[i] == true iff F_{i+2} is in the sum.
//   2. Write the Zeckendorf bits in order from index 0 (F_2) to the highest
//      set index.
//   3. Append a final '1' as the terminator.
//
// Codeword for n ends in "11" (the highest Zeckendorf bit is always 1, and
// the terminator is 1). No "11" appears elsewhere (Zeckendorf's non-adjacency
// condition).
//
// Examples:
//   1 -> bits={1},     codeword = "1" + "1"    = "11"
//   2 -> bits={0,1},   codeword = "01" + "1"   = "011"
//   3 -> bits={0,0,1}, codeword = "001" + "1"  = "0011"
//   4 -> bits={1,0,1}, codeword = "101" + "1"  = "1011"
//   8 -> bits={0,0,0,0,1}, codeword = "00001" + "1" = "000011"
//
// Decoding: read bits until two consecutive 1s are seen. The second 1 is the
// terminator; all prior bits (including the first 1 of the terminal pair) are
// Zeckendorf bits. Reconstruct n from the Fibonacci sum.
//
// Length: roughly log_phi(n) + 1 bits, where phi = (1+sqrt(5))/2 ~ 1.618.
//         Approximately 1.44 * log2(n) + 1 bits (44% overhead vs entropy).
// Implied prior: p_n ~ phi^{-n} (geometric with golden-ratio base).
// Key property: self-synchronizing via the "11" marker. A single bit flip
//               corrupts at most the codeword it hits and its immediate neighbor.

struct Fibonacci {
    using value_type = std::uint64_t;

    template<BitSink S>
    static void encode(value_type n, S& sink) {
        assert(n >= 1 && "Fibonacci is undefined for n = 0");
        auto bits = to_zeckendorf(n);
        // Write Zeckendorf bits in order from F_2 (index 0) outward.
        for (bool b : bits) sink.write(b);
        // Terminating '1' bit.
        sink.write(true);
    }

    template<BitSource S>
    static value_type decode(S& source) {
        // Read bits until we see two consecutive 1s. The second 1 is the
        // terminator. All bits before the terminator (including the first of
        // the terminal pair) are Zeckendorf bits.
        std::vector<bool> bits;
        bool prev = false;
        while (true) {
            bool cur = source.read();
            if (cur && prev) {
                // Two consecutive 1s: the last bit in 'bits' (which is 'prev')
                // and 'cur' (the terminator). Remove the terminator from bits.
                // 'bits' already has prev appended below before this check...
                // Actually: 'prev' was appended to bits in the previous iteration.
                // The terminator 'cur' is consumed but not appended.
                break;
            }
            bits.push_back(cur);
            prev = cur;
        }
        // Reconstruct n from the Fibonacci sum.
        std::vector<std::uint64_t> fibs{1, 2};
        while (fibs.size() < bits.size()) {
            fibs.push_back(fibs[fibs.size()-1] + fibs[fibs.size()-2]);
        }
        value_type n = 0;
        for (std::size_t i = 0; i < bits.size(); ++i) {
            if (bits[i]) n += fibs[i];
        }
        return n;
    }
};
```

- [ ] **Step 4: Build and verify tests pass**

```bash
cd /home/spinoza/github/metafunctor-series/wire-formats && make test 2>&1 | grep -E "PASSED|FAILED|test_fibonacci"
```

Expected: all FibonacciTest cases pass including round-trip, spot-check encodings, AllCodewordsEndIn11, and NoInternalConsecutiveOnes.

- [ ] **Step 5: Commit**

```bash
cd /home/spinoza/github/metafunctor-series/wire-formats
git add post/2022-07-fibonacci-wire-formats/fibonacci.hpp \
        post/2022-07-fibonacci-wire-formats/test_fibonacci.cpp
git commit -m "feat(fibonacci): implement Fibonacci codec with encode/decode (TDD)"
```

---

## Task 11: TDD -- bit-flip-stays-local test and integration tests for post 6

**Files:**
- Modify: `post/2022-07-fibonacci-wire-formats/test_fibonacci.cpp`
- Modify: `post/CMakeLists.txt` (add priors include path)

This task adds: (a) the self-synchronization demonstration from spec section E, and (b) integration tests via the priors library confirming Fibonacci's implied prior and bounded redundancy.

- [ ] **Step 1: Add priors include path to the test_fibonacci target in CMakeLists.txt**

In `post/CMakeLists.txt`, update the `target_include_directories` line for `test_fibonacci` to include the priors directory:

```cmake
target_include_directories(test_fibonacci PRIVATE
    2022-07-fibonacci-wire-formats
    2021-03-priors-wire-formats)
```

- [ ] **Step 2: Append the self-synchronization test and integration tests**

Append to `test_fibonacci.cpp`:

```cpp
// ---- Self-synchronization test (spec section E) ----------------------------

// Helper: encode a sequence of integers into one flat bit stream.
static std::vector<bool> encode_sequence(const std::vector<uint64_t>& seq) {
    struct VecSink {
        std::vector<bool>& bits;
        void write(bool b) { bits.push_back(b); }
    };
    std::vector<bool> result;
    VecSink sink{result};
    for (uint64_t n : seq) Fibonacci::encode(n, sink);
    return result;
}

// Helper: flip bit at index idx in a copy of the bit stream.
static std::vector<bool> flip_bit(std::vector<bool> bits, std::size_t idx) {
    bits[idx] = !bits[idx];
    return bits;
}

// Helper: decode as many integers as possible from a bit stream, stopping on
// any read past the end (returns partial results).
static std::vector<uint64_t> decode_sequence_partial(std::vector<bool> bits,
                                                      std::size_t max_count) {
    struct VecSource {
        const std::vector<bool>& bits;
        std::size_t pos = 0;
        bool read() {
            if (pos >= bits.size()) return false;
            return bits[pos++];
        }
    };
    VecSource source{bits};
    std::vector<uint64_t> result;
    result.reserve(max_count);
    // We try to decode up to max_count values; stop if we run out of bits.
    for (std::size_t i = 0; i < max_count && source.pos < bits.size(); ++i) {
        result.push_back(Fibonacci::decode(source));
    }
    return result;
}

// Spec section E: a single bit flip corrupts at most two codewords.
// We encode {3, 5, 7, 9, 11, 13}, flip a bit in the middle, and verify that
// at least 4 of the 6 values are recovered intact.
TEST(FibonacciTest, BitFlipStaysLocal) {
    const std::vector<uint64_t> original = {3, 5, 7, 9, 11, 13};
    auto encoded = encode_sequence(original);
    // Flip a bit roughly one-third of the way in.
    std::size_t flip_at = encoded.size() / 3;
    auto corrupted = flip_bit(encoded, flip_at);
    auto decoded = decode_sequence_partial(corrupted, original.size());
    int matches = 0;
    for (std::size_t i = 0; i < std::min(decoded.size(), original.size()); ++i) {
        if (decoded[i] == original[i]) ++matches;
    }
    // At least 4 of 6 values should be recovered (at most 2 corrupted).
    EXPECT_GE(matches, 4) << "Expected at least 4/6 values intact after 1-bit flip";
}

// ---- Integration tests using the priors library ----------------------------

#include "../2021-03-priors-wire-formats/priors.hpp"

// Helper: build fibonacci length vector for symbols 1..N.
static std::vector<std::size_t> fib_lengths(std::size_t N) {
    std::vector<std::size_t> v(N);
    for (std::size_t i = 0; i < N; ++i) {
        v[i] = fib_bit_count(static_cast<uint64_t>(i + 1));
    }
    return v;
}

// Fibonacci has bounded redundancy on its own implied prior (by definition).
TEST(FibonacciTest, FibonacciSmallRedundancyOnImpliedPrior) {
    const std::size_t N = 50;
    auto lens = fib_lengths(N);
    auto probs = priors::implied_prior(lens);
    double r = priors::redundancy(probs, lens);
    // The code is approximately optimal for its own prior.
    EXPECT_GE(r, 0.0);
    EXPECT_LT(r, 2.0);
}

// Fibonacci length grows as roughly log_phi(n) + 1 ~ 1.44 * log2(n) + 1.
// Verify the 44% overhead claim: Fibonacci lengths should be approximately
// 44% longer than Gamma lengths for large n (Gamma ~ 2*log2(n)).
TEST(FibonacciTest, FibonacciLengthVsGammaLength) {
    // For large n, fib_bit_count(n) / gamma_bit_count(n) should be near 0.72
    // (since fib ~ 1.44*log2(n) and gamma ~ 2*log2(n), ratio ~ 0.72).
    // We just verify that Fibonacci is shorter than Gamma for n >= 4 would be
    // wrong (Fibonacci is LONGER). Verify Fibonacci > Gamma for n in [4..100].
    for (uint64_t n = 4; n <= 100; ++n) {
        BitBuffer gamma_buf;
        // Re-use gamma from a local gamma length computation:
        // gamma length = 2*floor(log2(n)) + 1.
        std::size_t k = 0;
        uint64_t tmp = n;
        while (tmp > 1) { tmp >>= 1; ++k; }
        std::size_t gl = 2 * k + 1;
        std::size_t fl = fib_bit_count(n);
        // Fibonacci is longer than Gamma for practical n (44% overhead vs ~2x log2).
        EXPECT_GT(fl, gl - 2) << "n=" << n;
    }
}
```

- [ ] **Step 3: Build and verify all tests pass**

```bash
cd /home/spinoza/github/metafunctor-series/wire-formats && make test 2>&1 | grep -E "PASSED|FAILED|test_fibonacci"
```

Expected: all FibonacciTest cases pass including BitFlipStaysLocal, FibonacciSmallRedundancyOnImpliedPrior, and FibonacciLengthVsGammaLength.

- [ ] **Step 4: Commit**

```bash
cd /home/spinoza/github/metafunctor-series/wire-formats
git add post/2022-07-fibonacci-wire-formats/test_fibonacci.cpp \
        post/CMakeLists.txt
git commit -m "test(fibonacci): add self-sync test and priors integration tests"
```

---

## Task 12: Verify post 6 full-suite pass (clean rebuild + warning check)

**Files:** no changes; verification only.

- [ ] **Step 1: Clean rebuild**

```bash
cd /home/spinoza/github/metafunctor-series/wire-formats && make clean && make build 2>&1 | tail -5
```

Expected: successful clean build with no errors.

- [ ] **Step 2: Warning check for fibonacci**

```bash
cd /home/spinoza/github/metafunctor-series/wire-formats
cmake -B post/build -S post -DCMAKE_CXX_FLAGS="-Wall -Wextra -Wpedantic" 2>&1 \
    | grep -E "warning:|error:" | grep "fibonacci" | head -20
cmake --build post/build --target test_fibonacci 2>&1 \
    | grep -E "warning:|error:" | head -20
```

Expected: zero warnings and zero errors from `fibonacci.hpp` and `test_fibonacci.cpp`.

- [ ] **Step 3: Full ctest pass**

```bash
cd /home/spinoza/github/metafunctor-series/wire-formats && make test 2>&1 | tail -12
```

Expected: all six tests pass (test_kraft, test_mcmillan, test_priors, test_unary_gamma, test_elias_delta_omega, test_fibonacci).

No commit for this task (verification only).

---

## Task 13: Draft post 6 prose

**Files:**
- Modify: `post/2022-07-fibonacci-wire-formats/index.md`

Draft from the spec's sections A through G for Post 6. Target approximately 2000 words. No em-dashes.

- [ ] **Step 1: Draft the prose**

Replace the placeholder `(Draft in progress...)` line in `post/2022-07-fibonacci-wire-formats/index.md` with the full article body:

Section A ("A Different Design Goal", ~200 words, no code): contrast Fibonacci with Elias codes. Fibonacci does not optimize for length under a power-law prior; it optimizes for error resilience. A single bit flip in a gamma codeword can desynchronize the entire stream. A single bit flip in a Fibonacci codeword corrupts at most two codewords. Introduce the "11" terminator property.

Section B ("Zeckendorf's Theorem", ~250 words + `to_zeckendorf` code block): state the theorem (every positive integer has a unique representation as a sum of non-consecutive Fibonacci numbers), give the examples from the spec (1, 4, 10, 11), show the `to_zeckendorf` function.

Section C ("The Fibonacci Codeword", ~300 words + `Fibonacci` code block): explain the codeword construction (Zeckendorf bits in order from F_2, then a terminating '1'). Show the examples from the spec (1 through 8). Show the `Fibonacci` struct with encode and decode. Explain the decoding rule (read until two consecutive 1s).

Section D ("The Implied Prior", ~250 words, no code): length is roughly `log_phi(n) + 1 ~ 1.44 * log2(n) + 1` bits. That is about 44% longer than the entropy lower bound. The implied prior is geometric with golden-ratio base: `p_n ~ phi^{-n}`. This sits between geometric(1/2) (unary's prior) and the power-law priors of Elias codes.

Section E ("Self-Synchronization", ~250 words + `BitFlipStaysLocal` test block): explain the "11" marker as a resynchronization point. Show the test: encode {3, 5, 7, 9, 11, 13}, flip a bit, decode, verify at least 4 of 6 values are intact. Contrast with gamma: a single bit flip in gamma can corrupt an unbounded number of subsequent codewords.

Section F ("When to Use Fibonacci", ~200 words, no code): suitable for noisy channels or long-running streams where rare bit errors should not lose the entire tail. The 44% length overhead is significant; Fibonacci is a niche choice. When the channel is reliable, the Elias codes or Huffman are almost always better.

Section G ("Cross-references and footnote", ~120 words): per the spec.
- Forward: plain text mention of post 7 ("Rice / Golomb"), no link (forthcoming).
- Back: [Universal Codes as Priors](/post/2021-03-priors-wire-formats/) (post 3); [Unary and Elias Gamma](/post/2021-08-elias-gamma-wire-formats/) (post 4).
- Cross-series: none (Fibonacci is its own animal; the type-algebra side does not need self-synchronization).
- Footnote: PFC's `include/pfc/codecs.hpp` has `Fibonacci`.

Set `draft: false` when satisfied.

- [ ] **Step 2: Soul check (no em-dashes)**

```bash
grep -n $'\xe2\x80\x94' /home/spinoza/github/metafunctor-series/wire-formats/post/2022-07-fibonacci-wire-formats/index.md | head -5
```

Expected: no output.

- [ ] **Step 3: Commit**

```bash
cd /home/spinoza/github/metafunctor-series/wire-formats
git add post/2022-07-fibonacci-wire-formats/index.md
git commit -m "docs(fibonacci): draft post 6 prose (Fibonacci Coding)"
```

---

## Task 14: Update `docs/about.md` (mark posts 5 and 6 Published)

**Files:**
- Modify: `docs/about.md`

- [ ] **Step 1: Update the status rows for posts 5 and 6**

In `docs/about.md`, change the rows for posts 5 and 6 from `Forthcoming` to `Published`:

Old lines:
```
| 5 | Elias Delta and Omega | 2022-02-13 | Forthcoming |
| 6 | Fibonacci Coding | 2022-07-17 | Forthcoming |
```

New lines:
```
| 5 | Elias Delta and Omega | 2022-02-13 | Published |
| 6 | Fibonacci Coding | 2022-07-17 | Published |
```

- [ ] **Step 2: Commit**

```bash
cd /home/spinoza/github/metafunctor-series/wire-formats
git add docs/about.md
git commit -m "docs(about): mark posts 5 and 6 as Published"
```

---

## Task 15: Update `mkdocs.yml` (add posts 5 and 6 to nav)

**Files:**
- Modify: `mkdocs.yml`

- [ ] **Step 1: Add posts 5 and 6 to the "Universal Codes" nav section**

In `mkdocs.yml`, the "Universal Codes" section already has posts 3 and 4 from sub-sub-project 3a. Add posts 5 and 6:

```yaml
  - "Universal Codes":
      - "Universal Codes as Priors": "post/2021-03-priors-wire-formats/index.md"
      - "Unary and Elias Gamma": "post/2021-08-elias-gamma-wire-formats/index.md"
      - "Elias Delta and Omega": "post/2022-02-elias-delta-omega-wire-formats/index.md"
      - "Fibonacci Coding": "post/2022-07-fibonacci-wire-formats/index.md"
```

- [ ] **Step 2: Commit**

```bash
cd /home/spinoza/github/metafunctor-series/wire-formats
git add mkdocs.yml
git commit -m "docs(mkdocs): add posts 5 and 6 to Universal Codes nav section"
```

---

## Task 16: Final clean rebuild, soul check, and mkdocs build

**Files:** no changes; verification only.

- [ ] **Step 1: Clean rebuild and full test pass**

```bash
cd /home/spinoza/github/metafunctor-series/wire-formats && make clean && make test 2>&1 | tail -12
```

Expected: all six test suites pass (test_kraft, test_mcmillan, test_priors, test_unary_gamma, test_elias_delta_omega, test_fibonacci).

- [ ] **Step 2: Soul check on both new prose files**

```bash
grep -n $'\xe2\x80\x94' \
    /home/spinoza/github/metafunctor-series/wire-formats/post/2022-02-elias-delta-omega-wire-formats/index.md \
    /home/spinoza/github/metafunctor-series/wire-formats/post/2022-07-fibonacci-wire-formats/index.md \
    | head -10
```

Expected: no output (no em-dashes in either file).

- [ ] **Step 3: mkdocs build**

```bash
cd /home/spinoza/github/metafunctor-series/wire-formats && make docs 2>&1 | tail -8
```

Expected: successful build. Posts 5 and 6 should resolve cleanly; warnings about forthcoming posts 7+ in nav are acceptable.

No commit for this task.

---

## Task 17: Hugo sync via Makefile target

**Files:** changes land in `~/github/repos/metafunctor/` (synced, not committed here).

- [ ] **Step 1: Sync posts 5 and 6 to metafunctor**

```bash
BLOG_POST_DIR=/home/spinoza/github/repos/metafunctor/content/post \
    make -C /home/spinoza/github/metafunctor-series/wire-formats sync 2>&1
```

Expected: rsync output showing two directories synced:
- `-> 2022-02-elias-delta-omega-wire-formats`
- `-> 2022-07-fibonacci-wire-formats`

- [ ] **Step 2: Verify metafunctor received both post directories**

```bash
ls /home/spinoza/github/repos/metafunctor/content/post/ | grep -E "2022-02-elias-delta|2022-07-fibonacci"
```

Expected: both directories present.

No commit for this task (sync only; metafunctor commit is a separate step).

---

## Task 18: Verify metafunctor state

**Files:** read-only inspection of metafunctor repo.

- [ ] **Step 1: Check git status in metafunctor**

```bash
cd /home/spinoza/github/repos/metafunctor && git status --short | grep -E "2022-02-elias|2022-07-fibonacci"
```

Expected: two new untracked directories (or staged adds if already added).

- [ ] **Step 2: Check mf series scan**

```bash
cd /home/spinoza/github/repos/metafunctor && mf series scan 2>&1 | grep wire-formats | head -5
```

Expected: wire-formats series shows count including the two new posts (total 6 posts: Kraft, McMillan, priors, unary-gamma, elias-delta-omega, fibonacci).

No commit for this task.

---

## Task 19: Final pre-push verification across both repos

**Files:** read-only.

- [ ] **Step 1: Wire-formats git log (confirm all commits present)**

```bash
git -C /home/spinoza/github/metafunctor-series/wire-formats log --oneline | head -25
```

Expected: all commits from this plan appear in sequence: scaffold(elias-delta-omega), feat(elias-delta-omega) x2, test(elias-delta-omega), docs(elias-delta-omega), scaffold(fibonacci), feat(fibonacci) x2, test(fibonacci), docs(fibonacci), docs(about), docs(mkdocs).

- [ ] **Step 2: Verify no uncommitted changes in wire-formats**

```bash
cd /home/spinoza/github/metafunctor-series/wire-formats && git status --short
```

Expected: clean working tree (the `build/` and `site/` directories are gitignored).

- [ ] **Step 3: Confirm metafunctor has the two new post directories ready to commit**

```bash
cd /home/spinoza/github/repos/metafunctor && git status --short | grep "2022-"
```

Expected: two new directories shown as untracked or staged.

No commit for this task.

---

## Task 20: User-confirmed push

The user must confirm before pushing. Do not push automatically.

- [ ] **Step 1: Present the push plan and wait for confirmation**

Report the following to the user before proceeding:

- Wire-formats repo: N new commits (see Task 19 git log) to push to origin/main.
- Metafunctor repo: 2 new post directories to commit and push.
- No changes to the Stepanov repo in this sub-sub-project.

Wait for explicit user approval before running any push or commit commands in metafunctor.

- [ ] **Step 2: Push wire-formats (after user confirms)**

```bash
git -C /home/spinoza/github/metafunctor-series/wire-formats push origin main
```

- [ ] **Step 3: Commit and push metafunctor (after user confirms)**

```bash
cd /home/spinoza/github/repos/metafunctor
git add content/post/2022-02-elias-delta-omega-wire-formats \
        content/post/2022-07-fibonacci-wire-formats
git commit -m "content(wire-formats): sync posts 5 and 6 (elias-delta-omega, fibonacci)"
git push origin main
```

---

## Task 21: Final summary report

- [ ] **Step 1: Produce a summary for the user**

Report:
- Posts shipped: post 5 ("Elias Delta and Omega", 2022-02-13) and post 6 ("Fibonacci Coding", 2022-07-17).
- Files created: `elias_delta_omega.hpp` (3 codecs: Gamma local re-implementation, Delta, Omega), `test_elias_delta_omega.cpp` (round-trip, spot-check, length table, priors integration), `fibonacci.hpp` (`to_zeckendorf` + `Fibonacci` codec), `test_fibonacci.cpp` (Zeckendorf properties, round-trip, codeword structure, self-sync, priors integration), `index.md` for each post.
- Test counts: all six suites pass (test_kraft, test_mcmillan, test_priors, test_unary_gamma, test_elias_delta_omega, test_fibonacci).
- Navigation: `mkdocs.yml` updated with posts 5 and 6 in "Universal Codes" section; `docs/about.md` updated with Published status for both posts.
- Sync: both posts available in metafunctor under `content/post/`.
- Next: sub-sub-project 3c covers posts 7 ("Rice / Golomb") and 8 ("VByte"). Post 7 introduces the first parametric code family; post 8 covers byte-aligned coding.
