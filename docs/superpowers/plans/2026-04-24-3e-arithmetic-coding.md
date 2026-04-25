# Algebra over Wire Formats: Sub-sub-project 3e (Post 10, Arithmetic Coding) Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Implement an integer range coder (arithmetic coding with 32-bit integer arithmetic) and ship post 10.

**Architecture:** TDD-build the integer range coder: encoder state (low, high, underflow_count), bit emission via `renormalize()`, decoder state, symbol-encoding/decoding via cumulative-frequency lookups. Careful underflow handling. One commit per logical step; ~18 tasks total.

**Tech Stack:** C++23, GoogleTest v1.14.0, mkdocs, soul plugin.

---

## Spec reference

See `docs/superpowers/specs/2026-04-24-arc-posts-3-through-13.md`, section "Post 10: Arithmetic Coding" for per-section content guides A through G, code budgets (~320 lines), prose budget (~2200 words), and numerical-precision notes.

## Cross-references note

Post 9 (Huffman, planned date 2024-08-04) is the immediate back-reference. Post 3 (priors, 2022-01-15) supplies `entropy()` and `redundancy()` via cross-include. Post 11 (Succinct Bit Vectors, 2025-06-22) is the forward-reference. All forward/back links use the stable `/post/YYYY-MM-slug/` URL pattern; links to posts not yet published remain as plain text in the prose (no href).

The priors.hpp file lives at `post/2022-01-priors-wire-formats/priors.hpp`. The test target for post 10 gets `target_include_directories` pointing at both its own directory and `2022-01-priors-wire-formats/` so `#include "priors.hpp"` resolves without path duplication.

---

## Task 1: Reconnaissance (date collision check)

**Files:** read-only.

- [ ] **Step 1: Verify date 2025-01-12 does not collide with existing metafunctor posts**

```bash
grep -h "^date:" /home/spinoza/github/repos/metafunctor/content/post/*/index.md 2>/dev/null \
  | grep "^date: 2025-01-12" | sort -u
```

Expected: empty output. If a date collides, pick an adjacent unused date (2024-09-03 or 2024-09-05) and note it before proceeding. Use the chosen date throughout this plan.

- [ ] **Step 2: Confirm the post 10 directory does not yet exist**

```bash
ls /home/spinoza/github/metafunctor-series/wire-formats/post/ | grep "2024-09"
```

Expected: no output (directory does not exist yet).

- [ ] **Step 3: Confirm the existing CMakeLists ends with the last existing post block (so append is safe)**

```bash
tail -8 /home/spinoza/github/metafunctor-series/wire-formats/post/CMakeLists.txt
```

Expected: the last `add_test` line belongs to whatever the highest-numbered existing post is. If posts 3 through 9 are already wired in from earlier sub-sub-projects, this should show the Huffman block. No commit for this task.

---

## Task 2: Scaffold post 10 directory and wire CMakeLists

**Files:**
- Create: `post/2025-01-arithmetic-coding-wire-formats/index.md`
- Create: `post/2025-01-arithmetic-coding-wire-formats/arithmetic_coding.hpp`
- Create: `post/2025-01-arithmetic-coding-wire-formats/test_arithmetic_coding.cpp`
- Modify: `post/CMakeLists.txt`

- [ ] **Step 1: Create the post 10 directory and skeleton files**

```bash
mkdir -p /home/spinoza/github/metafunctor-series/wire-formats/post/2025-01-arithmetic-coding-wire-formats
```

Create `post/2025-01-arithmetic-coding-wire-formats/index.md` with placeholder frontmatter:

```markdown
---
title: "Arithmetic Coding"
date: 2025-01-12
draft: true
tags:
- C++
- information-theory
- coding-theory
- prefix-free
- entropy
- arithmetic-coding
- range-coding
categories:
- Computer Science
- Mathematics
series:
- wire-formats
series_weight: 10
math: true
description: "Arithmetic coding closes the gap between Huffman's per-symbol integer lengths and true entropy. A single number in the unit interval encodes an entire sequence; 32-bit integer arithmetic makes it practical."
linked_project:
- pfc
- wire-formats
---

(Draft in progress. See plan Task 14 for full prose.)
```

Create `post/2025-01-arithmetic-coding-wire-formats/arithmetic_coding.hpp` with header guards and namespace skeleton only:

```cpp
// arithmetic_coding.hpp
// Pedagogical integer range coder for the post "Arithmetic Coding" in the
// "Algebra over Wire Formats" series. For the production version, see PFC:
// https://github.com/queelius/pfc (include/pfc/arithmetic_coding.hpp)
//
// Reference: Witten, Neal, Cleary, "Arithmetic Coding for Data Compression,"
// CACM 30(6), 1987.

#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <vector>

namespace arithmetic_coding {

// 32-bit range coder constants.
constexpr std::uint32_t TOP_VALUE      = 0xFFFFFFFFu;
constexpr std::uint32_t HALF           = 0x80000000u;
constexpr std::uint32_t QUARTER        = 0x40000000u;
constexpr std::uint32_t THREE_QUARTER  = 0xC0000000u;

// Forward declarations -- implementations arrive in Tasks 3 through 8.
class BitWriter;
class BitReader;
class ArithmeticEncoder;
class ArithmeticDecoder;

}  // namespace arithmetic_coding
```

Create `post/2025-01-arithmetic-coding-wire-formats/test_arithmetic_coding.cpp` with a placeholder test:

```cpp
#include <gtest/gtest.h>
#include "arithmetic_coding.hpp"

TEST(ArithmeticCodingTest, Placeholder) {
    EXPECT_TRUE(true);
}
```

- [ ] **Step 2: Append the post 10 test executable to post/CMakeLists.txt**

Append to `/home/spinoza/github/metafunctor-series/wire-formats/post/CMakeLists.txt`:

```cmake

# =============================================================================
# Arithmetic Coding (post 10, 2025-01-12)
# =============================================================================
add_executable(test_arithmetic_coding
    2025-01-arithmetic-coding-wire-formats/test_arithmetic_coding.cpp)
target_link_libraries(test_arithmetic_coding GTest::gtest_main)
target_include_directories(test_arithmetic_coding PRIVATE
    2025-01-arithmetic-coding-wire-formats
    2022-01-priors-wire-formats)
add_test(NAME test_arithmetic_coding COMMAND test_arithmetic_coding)
```

Note: the `2022-01-priors-wire-formats` entry in `target_include_directories` allows the test to `#include "priors.hpp"` for entropy and redundancy measurements.

- [ ] **Step 3: Build and verify placeholder test passes**

```bash
cd /home/spinoza/github/metafunctor-series/wire-formats && make test 2>&1 | tail -15
```

Expected: all previous tests pass plus the new `ArithmeticCodingTest.Placeholder` test.

- [ ] **Step 4: Commit**

```bash
cd /home/spinoza/github/metafunctor-series/wire-formats
git add post/2025-01-arithmetic-coding-wire-formats post/CMakeLists.txt
git commit -m "scaffold(arithmetic-coding): add post 10 directory and CMake wiring"
```

---

## Task 3: TDD -- implement `BitWriter` and `BitReader` (local I/O helpers)

**Files:**
- Modify: `post/2025-01-arithmetic-coding-wire-formats/arithmetic_coding.hpp`
- Modify: `post/2025-01-arithmetic-coding-wire-formats/test_arithmetic_coding.cpp`

The integer range coder emits and reads individual bits. Rather than pulling in the full PFC `core.hpp`, define minimal self-contained `BitWriter` and `BitReader` classes that back their bit streams with `std::vector<std::uint8_t>`.

- [ ] **Step 1: Write failing tests for BitWriter / BitReader round-trip**

Replace `test_arithmetic_coding.cpp` with:

```cpp
#include <gtest/gtest.h>
#include "arithmetic_coding.hpp"
#include "priors.hpp"

using namespace arithmetic_coding;

// Round-trip: write N bits and read them back.
TEST(BitIOTest, RoundTripSingleBit) {
    BitWriter bw;
    bw.write(true);
    bw.flush();
    BitReader br(bw.bytes());
    EXPECT_EQ(br.read(), true);
}

TEST(BitIOTest, RoundTripMultipleBits) {
    BitWriter bw;
    std::vector<bool> bits = {1,0,1,1,0,0,1,0, 1,1,0,0,0,1,1,0};
    for (bool b : bits) bw.write(b);
    bw.flush();
    BitReader br(bw.bytes());
    for (std::size_t i = 0; i < bits.size(); ++i) {
        EXPECT_EQ(br.read(), bits[i]) << "bit index " << i;
    }
}

TEST(BitIOTest, EmptyStreamBytesEmpty) {
    BitWriter bw;
    bw.flush();
    // A flushed writer with no bits written should produce exactly 0 or 1
    // bytes (depending on implementation; we require the reader can be
    // constructed from whatever is returned).
    EXPECT_NO_THROW({ BitReader br(bw.bytes()); });
}
```

- [ ] **Step 2: Run to verify compile failure**

```bash
cd /home/spinoza/github/metafunctor-series/wire-formats && make build 2>&1 | grep -E "error:|undefined" | head -10
```

Expected: compile errors about `BitWriter` and `BitReader` not having definitions.

- [ ] **Step 3: Implement `BitWriter` and `BitReader` in `arithmetic_coding.hpp`**

Replace the forward declarations with full class definitions (insert before the `ArithmeticEncoder` forward declaration):

```cpp
// ---- BitWriter -- packs individual bits into bytes -------------------------
//
// Bits are packed MSB-first into each byte. flush() pads the final partial
// byte with zeros and appends it.

class BitWriter {
    std::vector<std::uint8_t> bytes_;
    std::uint8_t              current_byte_ = 0;
    int                       bit_count_    = 0;

public:
    void write(bool bit) {
        current_byte_ = static_cast<std::uint8_t>(
            (current_byte_ << 1) | (bit ? 1u : 0u));
        ++bit_count_;
        if (bit_count_ == 8) {
            bytes_.push_back(current_byte_);
            current_byte_ = 0;
            bit_count_    = 0;
        }
    }

    void flush() {
        if (bit_count_ > 0) {
            current_byte_ = static_cast<std::uint8_t>(
                current_byte_ << (8 - bit_count_));
            bytes_.push_back(current_byte_);
            current_byte_ = 0;
            bit_count_    = 0;
        }
    }

    [[nodiscard]] const std::vector<std::uint8_t>& bytes() const noexcept {
        return bytes_;
    }

    [[nodiscard]] std::size_t bit_count() const noexcept {
        return bytes_.size() * 8;
    }
};

// ---- BitReader -- reads individual bits from a byte vector -----------------
//
// Bits are read MSB-first from each byte. Reading past the end returns 0.

class BitReader {
    const std::vector<std::uint8_t>& bytes_;
    std::size_t byte_idx_ = 0;
    int         bit_idx_  = 7;  // next bit to read within current_byte (MSB=7)

public:
    explicit BitReader(const std::vector<std::uint8_t>& bytes)
        : bytes_(bytes) {}

    bool read() {
        if (byte_idx_ >= bytes_.size()) return false;
        bool bit = ((bytes_[byte_idx_] >> bit_idx_) & 1u) != 0;
        if (--bit_idx_ < 0) {
            bit_idx_ = 7;
            ++byte_idx_;
        }
        return bit;
    }
};
```

- [ ] **Step 4: Build and verify BitIO tests pass**

```bash
cd /home/spinoza/github/metafunctor-series/wire-formats && make test 2>&1 | grep -E "PASSED|FAILED|BitIO|ArithmeticCoding"
```

Expected: BitIOTest.RoundTripSingleBit, BitIOTest.RoundTripMultipleBits, BitIOTest.EmptyStreamBytesEmpty all pass.

- [ ] **Step 5: Commit**

```bash
cd /home/spinoza/github/metafunctor-series/wire-formats
git add post/2025-01-arithmetic-coding-wire-formats/arithmetic_coding.hpp \
        post/2025-01-arithmetic-coding-wire-formats/test_arithmetic_coding.cpp
git commit -m "feat(arithmetic-coding): implement BitWriter and BitReader (TDD)"
```

---

## Task 4: TDD -- implement `ArithmeticEncoder` constructor and state

**Files:**
- Modify: `post/2025-01-arithmetic-coding-wire-formats/arithmetic_coding.hpp`
- Modify: `post/2025-01-arithmetic-coding-wire-formats/test_arithmetic_coding.cpp`

- [ ] **Step 1: Append failing tests for encoder construction**

Append to `test_arithmetic_coding.cpp`:

```cpp
TEST(ArithmeticEncoderTest, ConstructorInitializesState) {
    BitWriter bw;
    ArithmeticEncoder enc(bw);
    // After construction: low = 0, high = TOP_VALUE, underflow_count = 0.
    EXPECT_EQ(enc.low(),             0u);
    EXPECT_EQ(enc.high(),            TOP_VALUE);
    EXPECT_EQ(enc.underflow_count(), 0u);
}
```

- [ ] **Step 2: Run to verify compile failure**

```bash
cd /home/spinoza/github/metafunctor-series/wire-formats && make build 2>&1 | grep -E "error:|undefined" | head -10
```

Expected: errors about `ArithmeticEncoder` not being defined.

- [ ] **Step 3: Implement `ArithmeticEncoder` class with state accessors**

Replace the `ArithmeticEncoder` forward declaration with the class skeleton:

```cpp
// ---- ArithmeticEncoder -- integer range coder (encoder) --------------------
//
// Encodes a sequence of symbols by iteratively shrinking the unit interval
// [low, high] / TOP_VALUE according to each symbol's cumulative-frequency
// sub-interval, then emitting agreed-upon high bits.
//
// Reference: Witten, Neal, Cleary (CACM 1987), Section 2.

class ArithmeticEncoder {
    std::uint32_t low_             = 0;
    std::uint32_t high_            = TOP_VALUE;
    std::size_t   underflow_count_ = 0;
    BitWriter&    sink_;

public:
    explicit ArithmeticEncoder(BitWriter& sink) : sink_(sink) {}

    // State accessors (used in tests; not part of the encoding interface).
    [[nodiscard]] std::uint32_t low()             const noexcept { return low_; }
    [[nodiscard]] std::uint32_t high()            const noexcept { return high_; }
    [[nodiscard]] std::size_t   underflow_count() const noexcept { return underflow_count_; }

    // encode_symbol, renormalize, finish -- implemented in Tasks 5 and 6.
};
```

- [ ] **Step 4: Build and verify encoder construction test passes**

```bash
cd /home/spinoza/github/metafunctor-series/wire-formats && make test 2>&1 | grep -E "PASSED|FAILED|ArithmeticEncoder"
```

Expected: ArithmeticEncoderTest.ConstructorInitializesState passes.

- [ ] **Step 5: Commit**

```bash
cd /home/spinoza/github/metafunctor-series/wire-formats
git add post/2025-01-arithmetic-coding-wire-formats/arithmetic_coding.hpp \
        post/2025-01-arithmetic-coding-wire-formats/test_arithmetic_coding.cpp
git commit -m "feat(arithmetic-coding): implement ArithmeticEncoder skeleton with state (TDD)"
```

---

## Task 5: TDD -- implement `encode_symbol` and `renormalize`

**Files:**
- Modify: `post/2025-01-arithmetic-coding-wire-formats/arithmetic_coding.hpp`
- Modify: `post/2025-01-arithmetic-coding-wire-formats/test_arithmetic_coding.cpp`

This is the numerical heart of the coder. `encode_symbol` shrinks the interval; `renormalize` extracts agreed-upon bits and handles underflow. Both must be correct for any round-trip test to pass.

- [ ] **Step 1: Append failing tests for encode_symbol and renormalize**

Append to `test_arithmetic_coding.cpp`:

```cpp
// After encoding a single symbol over two equiprobable symbols (low_cum=0,
// high_cum=1, total=2), the interval should be the lower half [0, HALF-1].
TEST(ArithmeticEncoderTest, EncodeSymbolShrinksIntervalCorrectly) {
    BitWriter bw;
    ArithmeticEncoder enc(bw);
    // Symbol 0 of 2 equiprobable symbols: low_cum=0, high_cum=1, total=2.
    enc.encode_symbol(0, 1, 2);
    // Expected: new_high = 0 + (TOTAL_RANGE * 1) / 2 - 1 = HALF - 1.
    //           new_low  = 0 + (TOTAL_RANGE * 0) / 2     = 0.
    // Then renormalize should emit one '0' bit and double the interval.
    // After renormalize: low=0, high=TOP_VALUE (full range again).
    EXPECT_EQ(enc.low(),  0u);
    EXPECT_EQ(enc.high(), TOP_VALUE);
}

// After encoding symbol 1 of 2 (the upper half), the interval should be
// the upper half [HALF, TOP_VALUE], then renormalize emits a '1' bit.
TEST(ArithmeticEncoderTest, EncodeSymbolUpperHalf) {
    BitWriter bw;
    ArithmeticEncoder enc(bw);
    enc.encode_symbol(1, 2, 2);
    EXPECT_EQ(enc.low(),  0u);
    EXPECT_EQ(enc.high(), TOP_VALUE);
}

// Underflow test: encoding a symbol that straddles the midpoint
// (low < QUARTER, high >= THREE_QUARTER after shrink) should increment
// underflow_count_ rather than emitting a bit.
TEST(ArithmeticEncoderTest, UnderflowIncrements) {
    // Use a very skewed distribution: symbol with low_cum=1, high_cum=3,
    // total=4. The interval shrinks to [QUARTER, THREE_QUARTER-1], which
    // straddles the midpoint; underflow_count should become >= 1.
    BitWriter bw;
    ArithmeticEncoder enc(bw);
    enc.encode_symbol(1, 3, 4);
    EXPECT_GE(enc.underflow_count(), 1u);
}
```

- [ ] **Step 2: Run to verify compile failure**

```bash
cd /home/spinoza/github/metafunctor-series/wire-formats && make build 2>&1 | grep -E "error:|undefined" | head -10
```

Expected: errors about `encode_symbol` not being declared.

- [ ] **Step 3: Implement `emit_bit_and_underflow`, `renormalize`, and `encode_symbol`**

Add to the `ArithmeticEncoder` class body (after the state accessors):

```cpp
    void encode_symbol(std::uint32_t low_cum, std::uint32_t high_cum,
                       std::uint32_t total) {
        assert(low_cum < high_cum);
        assert(high_cum <= total);
        assert(total > 0);
        std::uint64_t range = static_cast<std::uint64_t>(high_) - low_ + 1;
        high_ = low_ + static_cast<std::uint32_t>((range * high_cum) / total - 1);
        low_  = low_ + static_cast<std::uint32_t>((range * low_cum)  / total);
        renormalize();
    }

    // finish() -- emit the final disambiguation bits.
    // Implemented in Task 6.
    void finish();

private:
    void emit_bit_and_underflow(bool bit) {
        sink_.write(bit);
        while (underflow_count_ > 0) {
            sink_.write(!bit);
            --underflow_count_;
        }
    }

    void renormalize() {
        while (true) {
            if (high_ < HALF) {
                // Both bounds in [0, HALF): high bit is 0.
                emit_bit_and_underflow(false);
            } else if (low_ >= HALF) {
                // Both bounds in [HALF, TOP_VALUE]: high bit is 1.
                emit_bit_and_underflow(true);
                low_  -= HALF;
                high_ -= HALF;
            } else if (low_ >= QUARTER && high_ < THREE_QUARTER) {
                // Underflow: interval straddles HALF; squeeze toward middle.
                ++underflow_count_;
                low_  -= QUARTER;
                high_ -= QUARTER;
            } else {
                break;
            }
            low_  <<= 1;
            high_ = (high_ << 1) | 1u;
        }
    }
```

- [ ] **Step 4: Build and verify encode_symbol / renormalize tests pass**

```bash
cd /home/spinoza/github/metafunctor-series/wire-formats && make test 2>&1 | grep -E "PASSED|FAILED|ArithmeticEncoder"
```

Expected: EncodeSymbolShrinksIntervalCorrectly, EncodeSymbolUpperHalf, UnderflowIncrements all pass.

- [ ] **Step 5: Commit**

```bash
cd /home/spinoza/github/metafunctor-series/wire-formats
git add post/2025-01-arithmetic-coding-wire-formats/arithmetic_coding.hpp \
        post/2025-01-arithmetic-coding-wire-formats/test_arithmetic_coding.cpp
git commit -m "feat(arithmetic-coding): implement encode_symbol and renormalize (TDD)"
```

---

## Task 6: TDD -- implement `ArithmeticEncoder::finish`

**Files:**
- Modify: `post/2025-01-arithmetic-coding-wire-formats/arithmetic_coding.hpp`
- Modify: `post/2025-01-arithmetic-coding-wire-formats/test_arithmetic_coding.cpp`

`finish()` emits the final bits needed to disambiguate the encoded interval. After `finish()`, the bit stream is complete and the decoder can reconstruct the sequence.

- [ ] **Step 1: Append failing tests for finish**

Append to `test_arithmetic_coding.cpp`:

```cpp
// After encoding one symbol and calling finish(), the bit stream should be
// non-empty and the encoder should be in a defined terminal state.
TEST(ArithmeticEncoderTest, FinishProducesNonEmptyStream) {
    BitWriter bw;
    ArithmeticEncoder enc(bw);
    enc.encode_symbol(0, 1, 2);  // symbol 0 of {0,1} equiprobable
    enc.finish();
    bw.flush();
    EXPECT_GT(bw.bytes().size(), 0u);
}

// Encoding the same sequence twice should produce identical bit streams.
TEST(ArithmeticEncoderTest, FinishIsDeterministic) {
    auto encode_once = [](std::uint32_t sym_low, std::uint32_t sym_high,
                          std::uint32_t total) {
        BitWriter bw;
        ArithmeticEncoder enc(bw);
        enc.encode_symbol(sym_low, sym_high, total);
        enc.finish();
        bw.flush();
        return bw.bytes();
    };
    EXPECT_EQ(encode_once(0, 1, 2), encode_once(0, 1, 2));
    EXPECT_EQ(encode_once(1, 2, 2), encode_once(1, 2, 2));
}
```

- [ ] **Step 2: Run to verify link failure (finish is declared but not defined)**

```bash
cd /home/spinoza/github/metafunctor-series/wire-formats && make build 2>&1 | grep -E "error:|undefined" | head -10
```

Expected: linker error about `finish` being undefined.

- [ ] **Step 3: Implement `finish` (out-of-class definition or inline in class)**

Add the `finish` definition to the `ArithmeticEncoder` class. The standard approach: emit a final '0' or '1' bit depending on the current interval, followed by any outstanding underflow-opposite bits. Then emit one more bit to disambiguate:

```cpp
    // Defined inline; moves the out-of-class forward declaration.
    void finish() {
        // Emit one bit to identify which half of the current interval to use,
        // and flush any pending underflow bits.
        ++underflow_count_;  // ensures at least one opposition bit follows
        if (low_ < QUARTER) {
            emit_bit_and_underflow(false);
        } else {
            emit_bit_and_underflow(true);
        }
    }
```

(Remove the earlier "`void finish();`" forward declaration and place this definition at the same location.)

- [ ] **Step 4: Build and verify finish tests pass**

```bash
cd /home/spinoza/github/metafunctor-series/wire-formats && make test 2>&1 | grep -E "PASSED|FAILED|ArithmeticEncoder"
```

Expected: all ArithmeticEncoderTest cases pass (ConstructorInitializesState, EncodeSymbolShrinksIntervalCorrectly, EncodeSymbolUpperHalf, UnderflowIncrements, FinishProducesNonEmptyStream, FinishIsDeterministic).

- [ ] **Step 5: Commit**

```bash
cd /home/spinoza/github/metafunctor-series/wire-formats
git add post/2025-01-arithmetic-coding-wire-formats/arithmetic_coding.hpp \
        post/2025-01-arithmetic-coding-wire-formats/test_arithmetic_coding.cpp
git commit -m "feat(arithmetic-coding): implement ArithmeticEncoder::finish (TDD)"
```

---

## Task 7: TDD -- implement `ArithmeticDecoder` constructor and state

**Files:**
- Modify: `post/2025-01-arithmetic-coding-wire-formats/arithmetic_coding.hpp`
- Modify: `post/2025-01-arithmetic-coding-wire-formats/test_arithmetic_coding.cpp`

The decoder mirrors the encoder. Its state is `low_`, `high_`, and `code_` (the 32-bit window of bits read from the input). The constructor primes `code_` by reading the first 32 bits.

- [ ] **Step 1: Append failing tests for decoder construction**

Append to `test_arithmetic_coding.cpp`:

```cpp
// A decoder constructed from a BitReader derived from a 4-byte stream
// should initialize with low=0, high=TOP_VALUE, and code set to the
// first 32 bits of the stream.
TEST(ArithmeticDecoderTest, ConstructorPrimesCode) {
    // Encode one symbol so we have a non-trivial byte stream.
    BitWriter bw;
    {
        ArithmeticEncoder enc(bw);
        enc.encode_symbol(0, 1, 2);
        enc.finish();
    }
    bw.flush();
    BitReader br(bw.bytes());
    ArithmeticDecoder dec(br);
    EXPECT_EQ(dec.low(),  0u);
    EXPECT_EQ(dec.high(), TOP_VALUE);
    // code_ should be some 32-bit value -- we just verify it is accessible.
    EXPECT_NO_THROW({ (void)dec.code(); });
}
```

- [ ] **Step 2: Run to verify compile failure**

```bash
cd /home/spinoza/github/metafunctor-series/wire-formats && make build 2>&1 | grep -E "error:|undefined" | head -10
```

Expected: errors about `ArithmeticDecoder` not being defined.

- [ ] **Step 3: Implement `ArithmeticDecoder` class skeleton**

Replace the `ArithmeticDecoder` forward declaration with:

```cpp
// ---- ArithmeticDecoder -- integer range coder (decoder) --------------------
//
// Mirrors the encoder. The decoder maintains the same [low, high] interval and
// additionally a 32-bit code register, which holds the current prefix of the
// compressed input. Decoding a symbol finds the sub-interval containing code_,
// then performs the same renormalization as the encoder.

class ArithmeticDecoder {
    std::uint32_t low_  = 0;
    std::uint32_t high_ = TOP_VALUE;
    std::uint32_t code_ = 0;
    BitReader&    src_;

public:
    explicit ArithmeticDecoder(BitReader& src) : src_(src) {
        // Prime the code register with the first 32 bits.
        for (int i = 0; i < 32; ++i) {
            code_ = (code_ << 1) | (src_.read() ? 1u : 0u);
        }
    }

    [[nodiscard]] std::uint32_t low()  const noexcept { return low_; }
    [[nodiscard]] std::uint32_t high() const noexcept { return high_; }
    [[nodiscard]] std::uint32_t code() const noexcept { return code_; }

    // decode_symbol -- implemented in Task 8.
};
```

- [ ] **Step 4: Build and verify decoder construction test passes**

```bash
cd /home/spinoza/github/metafunctor-series/wire-formats && make test 2>&1 | grep -E "PASSED|FAILED|ArithmeticDecoder"
```

Expected: ArithmeticDecoderTest.ConstructorPrimesCode passes.

- [ ] **Step 5: Commit**

```bash
cd /home/spinoza/github/metafunctor-series/wire-formats
git add post/2025-01-arithmetic-coding-wire-formats/arithmetic_coding.hpp \
        post/2025-01-arithmetic-coding-wire-formats/test_arithmetic_coding.cpp
git commit -m "feat(arithmetic-coding): implement ArithmeticDecoder skeleton (TDD)"
```

---

## Task 8: TDD -- implement `ArithmeticDecoder::decode_symbol`

**Files:**
- Modify: `post/2025-01-arithmetic-coding-wire-formats/arithmetic_coding.hpp`
- Modify: `post/2025-01-arithmetic-coding-wire-formats/test_arithmetic_coding.cpp`

`decode_symbol` is the inverse of `encode_symbol`. It computes a scaled value from `code_`, calls a callback to find the symbol whose cumulative-frequency interval contains that value, updates the interval exactly as the encoder would, then renormalizes by reading new bits.

- [ ] **Step 1: Append a failing test for decode_symbol**

Append to `test_arithmetic_coding.cpp`:

```cpp
// Encoding then decoding a single symbol should recover the original.
// Use two equiprobable symbols (total=2, cumulative freqs {0,1,2}).
TEST(ArithmeticDecoderTest, DecodeSymbolAfterEncodeRoundTrips) {
    // Cumulative frequency table: sym 0 -> [0,1), sym 1 -> [1,2). Total=2.
    // Symbol lookup callback: given scaled_value in [0,total), return symbol.
    auto get_freq = [](std::uint32_t scaled) -> std::size_t {
        return (scaled >= 1) ? 1u : 0u;
    };
    // Cumulative intervals for update: sym 0 -> [0,1), sym 1 -> [1,2).
    auto cum_range = [](std::size_t sym)
        -> std::pair<std::uint32_t, std::uint32_t> {
        if (sym == 0) return {0, 1};
        return {1, 2};
    };

    for (std::size_t expected_sym : {0u, 1u}) {
        BitWriter bw;
        {
            ArithmeticEncoder enc(bw);
            auto [lo, hi] = cum_range(expected_sym);
            enc.encode_symbol(lo, hi, 2);
            enc.finish();
        }
        bw.flush();
        BitReader br(bw.bytes());
        ArithmeticDecoder dec(br);
        std::size_t got = dec.decode_symbol(get_freq, cum_range, 2);
        EXPECT_EQ(got, expected_sym);
    }
}
```

- [ ] **Step 2: Run to verify compile failure**

```bash
cd /home/spinoza/github/metafunctor-series/wire-formats && make build 2>&1 | grep -E "error:|undefined" | head -10
```

Expected: errors about `decode_symbol` not being declared.

- [ ] **Step 3: Implement `decode_symbol` and decoder renormalization**

Add to the `ArithmeticDecoder` class body:

```cpp
    // decode_symbol: find the symbol whose cumulative-frequency interval
    // contains the current scaled code value, update the interval, and
    // renormalize (reading new bits from src_).
    //
    // get_freq_cb: (std::uint32_t scaled_value) -> std::size_t symbol
    //   given a value in [0, total), returns the symbol index.
    // cum_range_cb: (std::size_t symbol) -> {low_cum, high_cum}
    //   returns the cumulative-frequency endpoints for the symbol.
    // total: total cumulative frequency.

    template <typename FreqCb, typename RangeCb>
    std::size_t decode_symbol(FreqCb&& get_freq_cb, RangeCb&& cum_range_cb,
                              std::uint32_t total) {
        std::uint64_t range  = static_cast<std::uint64_t>(high_) - low_ + 1;
        std::uint32_t scaled = static_cast<std::uint32_t>(
            (static_cast<std::uint64_t>(code_ - low_) * total) / range);

        std::size_t sym = get_freq_cb(scaled);
        auto [lo_cum, hi_cum] = cum_range_cb(sym);

        // Update interval exactly as the encoder would.
        high_ = low_ + static_cast<std::uint32_t>((range * hi_cum) / total - 1);
        low_  = low_ + static_cast<std::uint32_t>((range * lo_cum) / total);
        decoder_renormalize();
        return sym;
    }

private:
    void decoder_renormalize() {
        while (true) {
            if (high_ < HALF) {
                // Nothing to subtract; shift in a new bit.
            } else if (low_ >= HALF) {
                code_ -= HALF;
                low_  -= HALF;
                high_ -= HALF;
            } else if (low_ >= QUARTER && high_ < THREE_QUARTER) {
                code_ -= QUARTER;
                low_  -= QUARTER;
                high_ -= QUARTER;
            } else {
                break;
            }
            low_  <<= 1;
            high_ = (high_ << 1) | 1u;
            code_ = (code_ << 1) | (src_.read() ? 1u : 0u);
        }
    }
```

- [ ] **Step 4: Build and verify decode_symbol test passes**

```bash
cd /home/spinoza/github/metafunctor-series/wire-formats && make test 2>&1 | grep -E "PASSED|FAILED|ArithmeticDecoder"
```

Expected: ArithmeticDecoderTest.ConstructorPrimesCode and DecodeSymbolAfterEncodeRoundTrips both pass.

- [ ] **Step 5: Commit**

```bash
cd /home/spinoza/github/metafunctor-series/wire-formats
git add post/2025-01-arithmetic-coding-wire-formats/arithmetic_coding.hpp \
        post/2025-01-arithmetic-coding-wire-formats/test_arithmetic_coding.cpp
git commit -m "feat(arithmetic-coding): implement ArithmeticDecoder::decode_symbol (TDD)"
```

---

## Task 9: TDD -- single-symbol round-trip across various probabilities

**Files:**
- Modify: `post/2025-01-arithmetic-coding-wire-formats/test_arithmetic_coding.cpp`

Parameterized tests that encode a single symbol under several different two-symbol distributions and verify the decoder recovers it exactly.

- [ ] **Step 1: Append round-trip tests for various distributions**

Append to `test_arithmetic_coding.cpp`:

```cpp
// Helper: encode a single symbol from a two-symbol source (total=T,
// cumulative freqs 0..lo_cum and lo_cum..T), then decode and verify.
static std::size_t encode_decode_single(std::uint32_t lo_cum,
                                        std::uint32_t hi_cum,
                                        std::uint32_t total,
                                        std::size_t   expected_sym) {
    BitWriter bw;
    {
        ArithmeticEncoder enc(bw);
        enc.encode_symbol(lo_cum, hi_cum, total);
        enc.finish();
    }
    bw.flush();
    BitReader br(bw.bytes());
    ArithmeticDecoder dec(br);

    auto get_freq = [=](std::uint32_t scaled) -> std::size_t {
        return (scaled >= lo_cum) ? 1u : 0u;
    };
    auto cum_range = [=](std::size_t sym)
        -> std::pair<std::uint32_t, std::uint32_t> {
        if (sym == 0) return {0, lo_cum};
        return {lo_cum, total};
    };
    return dec.decode_symbol(get_freq, cum_range, total);
}

// Equiprobable (50/50)
TEST(RoundTripTest, SingleSymbolEquiprobable) {
    EXPECT_EQ(encode_decode_single(0, 1, 2, 0u), 0u);
    EXPECT_EQ(encode_decode_single(1, 2, 2, 1u), 1u);
}

// Skewed 90/10
TEST(RoundTripTest, SingleSymbolSkewed90) {
    EXPECT_EQ(encode_decode_single(0, 9, 10, 0u), 0u);
    EXPECT_EQ(encode_decode_single(9, 10, 10, 1u), 1u);
}

// Skewed 99/1
TEST(RoundTripTest, SingleSymbolSkewed99) {
    EXPECT_EQ(encode_decode_single(0, 99, 100, 0u), 0u);
    EXPECT_EQ(encode_decode_single(99, 100, 100, 1u), 1u);
}
```

- [ ] **Step 2: Build and verify round-trip tests pass**

```bash
cd /home/spinoza/github/metafunctor-series/wire-formats && make test 2>&1 | grep -E "PASSED|FAILED|RoundTrip"
```

Expected: RoundTripTest.SingleSymbolEquiprobable, .SingleSymbolSkewed90, .SingleSymbolSkewed99 all pass.

- [ ] **Step 3: Commit**

```bash
cd /home/spinoza/github/metafunctor-series/wire-formats
git add post/2025-01-arithmetic-coding-wire-formats/test_arithmetic_coding.cpp
git commit -m "test(arithmetic-coding): single-symbol round-trip across distributions (TDD)"
```

---

## Task 10: TDD -- multi-symbol sequence round-trip

**Files:**
- Modify: `post/2025-01-arithmetic-coding-wire-formats/test_arithmetic_coding.cpp`

Round-trip tests for sequences of 10, 50, and 100 symbols using a helper that encodes a full sequence then decodes it symbol-by-symbol.

- [ ] **Step 1: Append sequence round-trip tests**

Append to `test_arithmetic_coding.cpp`:

```cpp
// Helper: encode a sequence of binary symbols under a Bernoulli(p) source
// (p = prob_high/total), then decode and verify.
// Returns true if the decoded sequence matches the original.
static bool sequence_round_trip(const std::vector<std::size_t>& symbols,
                                 std::uint32_t prob_high, std::uint32_t total) {
    // Cumulative freqs: sym 0 -> [0, prob_high), sym 1 -> [prob_high, total).
    BitWriter bw;
    {
        ArithmeticEncoder enc(bw);
        for (std::size_t sym : symbols) {
            if (sym == 0) {
                enc.encode_symbol(0, prob_high, total);
            } else {
                enc.encode_symbol(prob_high, total, total);
            }
        }
        enc.finish();
    }
    bw.flush();
    BitReader br(bw.bytes());
    ArithmeticDecoder dec(br);

    auto get_freq = [=](std::uint32_t scaled) -> std::size_t {
        return (scaled >= prob_high) ? 1u : 0u;
    };
    auto cum_range = [=](std::size_t sym)
        -> std::pair<std::uint32_t, std::uint32_t> {
        if (sym == 0) return {0, prob_high};
        return {prob_high, total};
    };

    for (std::size_t expected : symbols) {
        std::size_t got = dec.decode_symbol(get_freq, cum_range, total);
        if (got != expected) return false;
    }
    return true;
}

TEST(RoundTripTest, Sequence10SymbolsEquiprobable) {
    std::vector<std::size_t> syms = {0,1,0,0,1,1,0,1,0,0};
    EXPECT_TRUE(sequence_round_trip(syms, 1, 2));
}

TEST(RoundTripTest, Sequence50SymbolsSkewed90) {
    std::vector<std::size_t> syms(50);
    for (std::size_t i = 0; i < 50; ++i) syms[i] = (i % 10 == 0) ? 1u : 0u;
    EXPECT_TRUE(sequence_round_trip(syms, 9, 10));
}

TEST(RoundTripTest, Sequence100SymbolsSkewed99) {
    std::vector<std::size_t> syms(100);
    for (std::size_t i = 0; i < 100; ++i) syms[i] = (i % 100 == 0) ? 1u : 0u;
    EXPECT_TRUE(sequence_round_trip(syms, 99, 100));
}
```

- [ ] **Step 2: Build and verify sequence round-trip tests pass**

```bash
cd /home/spinoza/github/metafunctor-series/wire-formats && make test 2>&1 | grep -E "PASSED|FAILED|RoundTrip"
```

Expected: all RoundTripTest cases pass (single-symbol and sequence tests).

- [ ] **Step 3: Commit**

```bash
cd /home/spinoza/github/metafunctor-series/wire-formats
git add post/2025-01-arithmetic-coding-wire-formats/test_arithmetic_coding.cpp
git commit -m "test(arithmetic-coding): multi-symbol sequence round-trip (TDD)"
```

---

## Task 11: TDD -- convergence-to-entropy test

**Files:**
- Modify: `post/2025-01-arithmetic-coding-wire-formats/test_arithmetic_coding.cpp`

Verify that as the sequence length grows, the encoder's output approaches H(p) bits per symbol. Uses `priors::entropy` via the cross-include established in Task 2.

- [ ] **Step 1: Append convergence test**

Append to `test_arithmetic_coding.cpp`:

```cpp
#include "priors.hpp"

// Measure bits per symbol for a Bernoulli(p) source encoded with arithmetic
// coding. Returns bits_per_symbol = (bits_written / n_symbols).
static double measure_bits_per_symbol(std::size_t n_symbols,
                                      std::uint32_t prob_high,
                                      std::uint32_t total) {
    // Generate an alternating sequence (not truly random, but deterministic).
    std::vector<std::size_t> syms(n_symbols);
    std::uint32_t ratio = total / prob_high;
    for (std::size_t i = 0; i < n_symbols; ++i) {
        syms[i] = (i % ratio == ratio - 1) ? 1u : 0u;
    }
    BitWriter bw;
    {
        ArithmeticEncoder enc(bw);
        for (std::size_t sym : syms) {
            if (sym == 0) enc.encode_symbol(0, prob_high, total);
            else          enc.encode_symbol(prob_high, total, total);
        }
        enc.finish();
    }
    bw.flush();
    return static_cast<double>(bw.bytes().size() * 8) /
           static_cast<double>(n_symbols);
}

// For a Bernoulli(99/100) source, entropy H ~ 0.081 bits/symbol.
// With 10000 symbols, the arithmetic coder should achieve <= 0.2 bits/symbol.
TEST(ConvergenceTest, ApproachesEntropyBernoulli99) {
    // H(p) for p0=0.99, p1=0.01.
    std::vector<double> dist = {0.99, 0.01};
    double h = priors::entropy(dist);  // ~0.0808 bits
    double bps = measure_bits_per_symbol(10000, 99, 100);
    // Generous tolerance: within 0.5 bits/symbol of entropy.
    EXPECT_LT(bps, h + 0.5) << "bps=" << bps << " H=" << h;
    // Must be better than 1 bit/symbol (Huffman on binary source).
    EXPECT_LT(bps, 1.0);
}

// For a uniform binary source (entropy = 1 bit/symbol), should be ~1.
TEST(ConvergenceTest, NearEntropyForUniform) {
    std::vector<double> dist = {0.5, 0.5};
    double h = priors::entropy(dist);  // exactly 1.0 bit
    double bps = measure_bits_per_symbol(10000, 1, 2);
    EXPECT_NEAR(bps, h, 0.1) << "bps=" << bps << " H=" << h;
}
```

- [ ] **Step 2: Build and verify convergence tests pass**

```bash
cd /home/spinoza/github/metafunctor-series/wire-formats && make test 2>&1 | grep -E "PASSED|FAILED|Convergence"
```

Expected: ConvergenceTest.ApproachesEntropyBernoulli99 and ConvergenceTest.NearEntropyForUniform both pass.

- [ ] **Step 3: Commit**

```bash
cd /home/spinoza/github/metafunctor-series/wire-formats
git add post/2025-01-arithmetic-coding-wire-formats/test_arithmetic_coding.cpp
git commit -m "test(arithmetic-coding): convergence-to-entropy test for Bernoulli(0.99) (TDD)"
```

---

## Task 12: TDD -- binary-source compression demo (the spec's compelling test)

**Files:**
- Modify: `post/2025-01-arithmetic-coding-wire-formats/test_arithmetic_coding.cpp`

The spec highlights a specific demo: a Bernoulli(0.99) source of 1000 symbols. Huffman cannot compress below 1 bit/symbol on a binary source. Arithmetic coding achieves approximately 0.082 bits/symbol, a factor-of-12 improvement.

- [ ] **Step 1: Append the binary-source demo test**

Append to `test_arithmetic_coding.cpp`:

```cpp
// The compelling comparison: Bernoulli(0.99) with 1000 symbols.
// Huffman: cannot compress below 1 bit/symbol for a binary source.
// Arithmetic: achieves ~0.082 bits/symbol -> ~82 bits total.
TEST(BinarySourceDemoTest, Bernoulli99OneThousandSymbols) {
    // Generate a 1000-symbol sequence with ~1% ones.
    const std::size_t N = 1000;
    std::vector<std::size_t> syms(N, 0u);
    for (std::size_t i = 10; i < N; i += 100) syms[i] = 1u;

    BitWriter bw;
    {
        ArithmeticEncoder enc(bw);
        for (std::size_t sym : syms) {
            if (sym == 0) enc.encode_symbol(0, 99, 100);
            else          enc.encode_symbol(99, 100, 100);
        }
        enc.finish();
    }
    bw.flush();

    // Bytes * 8 = total bits emitted.
    std::size_t bits = bw.bytes().size() * 8;
    double bps = static_cast<double>(bits) / static_cast<double>(N);

    // Arithmetic coding on this source should achieve well under 1 bit/symbol.
    EXPECT_LT(bps, 1.0)
        << "Expected arithmetic coding to beat 1 bit/symbol; got " << bps;

    // And should be in the right ballpark of entropy (within 1 bit/symbol).
    std::vector<double> dist = {0.99, 0.01};
    double h = priors::entropy(dist);  // ~0.081 bits/symbol
    EXPECT_LT(bps, h + 1.0)
        << "Expected close to H=" << h << " bits/symbol; got " << bps;

    // Verify round-trip still works on this sequence.
    BitReader br(bw.bytes());
    ArithmeticDecoder dec(br);
    auto get_freq  = [](std::uint32_t s) -> std::size_t { return (s >= 99) ? 1u : 0u; };
    auto cum_range = [](std::size_t sym)
        -> std::pair<std::uint32_t, std::uint32_t> {
        return sym == 0 ? std::make_pair(0u, 99u)
                        : std::make_pair(99u, 100u);
    };
    for (std::size_t i = 0; i < N; ++i) {
        EXPECT_EQ(dec.decode_symbol(get_freq, cum_range, 100), syms[i])
            << "mismatch at symbol " << i;
    }
}
```

- [ ] **Step 2: Build and verify the demo test passes**

```bash
cd /home/spinoza/github/metafunctor-series/wire-formats && make test 2>&1 | grep -E "PASSED|FAILED|BinarySource"
```

Expected: BinarySourceDemoTest.Bernoulli99OneThousandSymbols passes.

- [ ] **Step 3: Commit**

```bash
cd /home/spinoza/github/metafunctor-series/wire-formats
git add post/2025-01-arithmetic-coding-wire-formats/test_arithmetic_coding.cpp
git commit -m "test(arithmetic-coding): binary-source compression demo, Bernoulli(0.99)"
```

---

## Task 13: Verify post 10 full-suite pass (clean rebuild + warning check)

**Files:** no changes; verification only.

- [ ] **Step 1: Clean rebuild**

```bash
cd /home/spinoza/github/metafunctor-series/wire-formats && make clean && make build 2>&1 | tail -5
```

Expected: successful clean build with no errors.

- [ ] **Step 2: Warning check for arithmetic_coding.hpp and test_arithmetic_coding.cpp**

```bash
cd /home/spinoza/github/metafunctor-series/wire-formats
cmake -B post/build -S post \
    -DCMAKE_CXX_FLAGS="-Wall -Wextra -Wpedantic" 2>&1 \
    | grep -E "warning:|error:" | grep "arithmetic_coding" | head -20
cmake --build post/build --target test_arithmetic_coding 2>&1 \
    | grep -E "warning:|error:" | head -20
```

Expected: zero warnings, zero errors for arithmetic_coding.hpp and test_arithmetic_coding.cpp.

- [ ] **Step 3: Full ctest pass**

```bash
cd /home/spinoza/github/metafunctor-series/wire-formats && make test 2>&1 | tail -10
```

Expected: all tests pass including test_arithmetic_coding (no regressions in earlier suites).

No commit for this task (verification only).

---

## Task 14: Draft post 10 prose

**Files:**
- Modify: `post/2025-01-arithmetic-coding-wire-formats/index.md`

Draft from the spec's sections A through G. Target: approximately 2200 words. Section C (the integer implementation) is the densest section; budget extra time. No em-dashes anywhere in the file.

- [ ] **Step 1: Draft the prose**

Replace the placeholder `(Draft in progress...)` line in `index.md` with the full article body. Follow the spec's section-by-section outline:

**Section A ("The Last Bit of Redundancy", ~250 words):** Huffman achieves "within 1 bit of entropy." Show where that 1 bit comes from: codeword lengths are integers, entropy is real-valued. A symbol with probability 0.7 has optimal length $-\log_2(0.7) \approx 0.515$ bits; Huffman must round to 1 bit. Foreshadow that arithmetic coding avoids this by encoding blocks as a single real number.

**Section B ("The Continuous View", ~250 words):** The unit interval $[0, 1)$. Subdivide by symbol probabilities. Encoding a sequence narrows the interval by a factor of $\prod p_i$; after $L$ symbols, the interval has width $\prod p_i$, requiring exactly $\sum (-\log_2 p_i)$ bits to specify. As $L \to \infty$, bits per symbol approaches $H(p)$ exactly.

**Section C ("The Integer Implementation", ~400 words, with code):** Real coders use 32-bit integer arithmetic. Present the constants, the encoder class, `encode_symbol`, `renormalize` (the bit-extraction loop with underflow handling), and `finish`. Walk through the underflow case: when `low >= QUARTER && high < THREE_QUARTER`, neither high bit agrees but the interval is shrinking around the midpoint; track the count and emit corrective bits when high bits finally agree.

Include the renormalize loop verbatim from the implementation as the code block.

**Section D ("Tests and the Compelling Example", ~200 words):** Show the Bernoulli(0.99) result. On 1000 symbols, arithmetic coding emits roughly 82 bits vs. Huffman's 1000 bits -- a factor-of-12 improvement. Show the entropy calculation: $H(0.99, 0.01) \approx 0.081$ bits per symbol.

**Section E ("The Adaptive Variant", ~250 words):** Generalize to adaptive distributions. After each symbol, update the cumulative-frequency table. Encoder and decoder stay synchronized because they apply identical updates. Mention JPEG XL and AV1 as production users of adaptive arithmetic coding. Note the trade with Huffman: adaptive Huffman requires rebalancing a tree; adaptive arithmetic only updates a frequency count.

**Section F ("The Theoretical Endpoint", ~300 words):** State Shannon's source-coding theorem for memoryless sources: no prefix-free code can achieve fewer expected bits than $H(p)$; arithmetic coding reaches $H(p)$ in the limit. Discuss what comes beyond entropy coding: sources with memory require richer models (Markov chains, context mixing). Context-mixing predictors feeding arithmetic coders are how the best general-purpose lossless compressors of 2024 are built (PAQ, ZPAQ, etc.).

**Section G ("Cross-references and footnote", ~120 words):** Include the exact cross-reference set from the spec:
- Forward: Succinct Bit Vectors and Rank/Select (post 11) shifts from entropy coding to space-efficient data structures (plain text, no href yet).
- Back: [Huffman](/post/2024-08-huffman-wire-formats/) (post 9), [Universal Codes as Priors](/post/2022-01-priors-wire-formats/) (post 3).
- Cross-series: Bits Follow Types -- arithmetic is the entropy-optimal version of the Either combinator's tag bit (plain text reference).
- Footnote: PFC's `include/pfc/arithmetic_coding.hpp` has both the integer range coder and a higher-level adaptive variant.

Set `draft: false` in the frontmatter when satisfied with the draft.

- [ ] **Step 2: Soul check (em-dash grep)**

```bash
grep -c $'\xe2\x80\x94' \
    /home/spinoza/github/metafunctor-series/wire-formats/post/2025-01-arithmetic-coding-wire-formats/index.md
```

Expected: 0. If non-zero, locate and replace each em-dash with a comma, colon, or period before proceeding.

- [ ] **Step 3: Commit**

```bash
cd /home/spinoza/github/metafunctor-series/wire-formats
git add post/2025-01-arithmetic-coding-wire-formats/index.md
git commit -m "docs(arithmetic-coding): draft post 10 prose (Arithmetic Coding)"
```

---

## Task 15: Update `docs/about.md` (mark post 10 Published)

**Files:**
- Modify: `docs/about.md`

- [ ] **Step 1: Change post 10 status from Forthcoming to Published**

In `docs/about.md`, locate the row:

```
| 10 | Arithmetic Coding | 2025-01-12 | Forthcoming |
```

Change it to:

```
| 10 | Arithmetic Coding | 2025-01-12 | Published |
```

- [ ] **Step 2: Commit**

```bash
cd /home/spinoza/github/metafunctor-series/wire-formats
git add docs/about.md
git commit -m "docs(about): mark post 10 (Arithmetic Coding) as Published"
```

---

## Task 16: Update `mkdocs.yml` (add post 10 to "Entropy-Optimal" section)

**Files:**
- Modify: `mkdocs.yml`

- [ ] **Step 1: Add post 10 to the nav under an "Entropy-Optimal" section**

In `mkdocs.yml`, after whatever nav section contains post 9 (Huffman), add post 10. If an "Entropy-Optimal" section already exists from the sub-project 3d plan, append the arithmetic-coding entry to it. If not, create the section:

```yaml
  - "Entropy-Optimal":
      - "Huffman": "post/2024-08-huffman-wire-formats/index.md"
      - "Arithmetic Coding": "post/2025-01-arithmetic-coding-wire-formats/index.md"
```

If the Huffman entry is already there under "Entropy-Optimal", only the arithmetic-coding line is new:

```yaml
      - "Arithmetic Coding": "post/2025-01-arithmetic-coding-wire-formats/index.md"
```

- [ ] **Step 2: Commit**

```bash
cd /home/spinoza/github/metafunctor-series/wire-formats
git add mkdocs.yml
git commit -m "docs(mkdocs): add Arithmetic Coding to Entropy-Optimal nav section"
```

---

## Task 17: Final verification (clean rebuild + soul check + mkdocs build + Hugo sync)

**Files:** no changes; verification only (Hugo sync writes to metafunctor, not wire-formats).

- [ ] **Step 1: Clean rebuild and full test pass**

```bash
cd /home/spinoza/github/metafunctor-series/wire-formats && make clean && make test 2>&1 | tail -10
```

Expected: all test suites pass (test_kraft, test_mcmillan, plus any from posts 3-9 from earlier plans, plus test_arithmetic_coding). No regressions.

- [ ] **Step 2: Soul check on the prose file**

```bash
grep -c $'\xe2\x80\x94' \
    /home/spinoza/github/metafunctor-series/wire-formats/post/2025-01-arithmetic-coding-wire-formats/index.md
```

Expected: 0.

- [ ] **Step 3: mkdocs build**

```bash
cd /home/spinoza/github/metafunctor-series/wire-formats && make docs 2>&1 | tail -8
```

Expected: successful build. Post 10 should resolve cleanly; warnings about forthcoming posts 11-13 are acceptable.

- [ ] **Step 4: Hugo sync via FIXED Makefile target**

```bash
BLOG_POST_DIR=/home/spinoza/github/repos/metafunctor/content/post \
    make -C /home/spinoza/github/metafunctor-series/wire-formats sync 2>&1
```

Expected: rsync output showing `-> 2025-01-arithmetic-coding-wire-formats` synced.

- [ ] **Step 5: Verify metafunctor received the post directory**

```bash
ls /home/spinoza/github/repos/metafunctor/content/post/ | grep "2024-09-arithmetic"
```

Expected: `2025-01-arithmetic-coding-wire-formats` present.

No commit for this task.

---

## Task 18: Final pre-push verification + user-confirmed push

**Files:** read-only verification; then push after user confirms.

- [ ] **Step 1: Wire-formats git log (confirm all commits present)**

```bash
git -C /home/spinoza/github/metafunctor-series/wire-formats log --oneline | head -25
```

Expected: all commits from this plan appear in sequence:
- scaffold(arithmetic-coding): ...
- feat(arithmetic-coding): implement BitWriter and BitReader (TDD)
- feat(arithmetic-coding): implement ArithmeticEncoder skeleton with state (TDD)
- feat(arithmetic-coding): implement encode_symbol and renormalize (TDD)
- feat(arithmetic-coding): implement ArithmeticEncoder::finish (TDD)
- feat(arithmetic-coding): implement ArithmeticDecoder skeleton (TDD)
- feat(arithmetic-coding): implement ArithmeticDecoder::decode_symbol (TDD)
- test(arithmetic-coding): single-symbol round-trip across distributions (TDD)
- test(arithmetic-coding): multi-symbol sequence round-trip (TDD)
- test(arithmetic-coding): convergence-to-entropy test for Bernoulli(0.99) (TDD)
- test(arithmetic-coding): binary-source compression demo, Bernoulli(0.99)
- docs(arithmetic-coding): draft post 10 prose (Arithmetic Coding)
- docs(about): mark post 10 (Arithmetic Coding) as Published
- docs(mkdocs): add Arithmetic Coding to Entropy-Optimal nav section

- [ ] **Step 2: Verify no uncommitted changes in wire-formats**

```bash
cd /home/spinoza/github/metafunctor-series/wire-formats && git status --short
```

Expected: clean working tree (build/ and site/ are gitignored).

- [ ] **Step 3: Confirm metafunctor has the post directory ready to commit**

```bash
cd /home/spinoza/github/repos/metafunctor && git status --short | grep "2024-09-arithmetic"
```

Expected: the directory shown as untracked or staged.

- [ ] **Step 4: Present the push plan and wait for user confirmation**

Report to the user:
- Wire-formats repo: ~14 new commits to push to origin/main.
- Metafunctor repo: 1 new post directory (`2025-01-arithmetic-coding-wire-formats`) to commit and push.

Wait for explicit user approval before proceeding.

- [ ] **Step 5: Push wire-formats (after user confirms)**

```bash
git -C /home/spinoza/github/metafunctor-series/wire-formats push origin main
```

- [ ] **Step 6: Commit and push metafunctor (after user confirms)**

```bash
cd /home/spinoza/github/repos/metafunctor
git add content/post/2025-01-arithmetic-coding-wire-formats
git commit -m "content(wire-formats): sync post 10 (Arithmetic Coding)"
git push origin main
```

---

## Code embedding summary

The integer range coder's key constants and structures for reference during implementation (spec-mandated):

```cpp
constexpr std::uint32_t TOP_VALUE      = 0xFFFFFFFFu;
constexpr std::uint32_t HALF           = 0x80000000u;
constexpr std::uint32_t QUARTER        = 0x40000000u;
constexpr std::uint32_t THREE_QUARTER  = 0xC0000000u;
```

Encode-symbol core (uses `std::uint64_t` for the intermediate product to avoid overflow):

```cpp
void encode_symbol(std::uint32_t low_cum, std::uint32_t high_cum,
                   std::uint32_t total) {
    std::uint64_t range = static_cast<std::uint64_t>(high_) - low_ + 1;
    high_ = low_ + static_cast<std::uint32_t>((range * high_cum) / total - 1);
    low_  = low_ + static_cast<std::uint32_t>((range * low_cum)  / total);
    renormalize();
}
```

Renormalize loop (three cases: agree-on-zero, agree-on-one, underflow-straddle):

```cpp
void renormalize() {
    while (true) {
        if (high_ < HALF) {
            emit_bit_and_underflow(false);
        } else if (low_ >= HALF) {
            emit_bit_and_underflow(true);
            low_  -= HALF;
            high_ -= HALF;
        } else if (low_ >= QUARTER && high_ < THREE_QUARTER) {
            ++underflow_count_;
            low_  -= QUARTER;
            high_ -= QUARTER;
        } else {
            break;
        }
        low_  <<= 1;
        high_ = (high_ << 1) | 1u;
    }
}

void emit_bit_and_underflow(bool bit) {
    sink_.write(bit);
    while (underflow_count_ > 0) {
        sink_.write(!bit);
        --underflow_count_;
    }
}
```

Decode-symbol core (scales code_ into [0, total) then does inverse interval update):

```cpp
template <typename FreqCb, typename RangeCb>
std::size_t decode_symbol(FreqCb&& get_freq_cb, RangeCb&& cum_range_cb,
                          std::uint32_t total) {
    std::uint64_t range  = static_cast<std::uint64_t>(high_) - low_ + 1;
    std::uint32_t scaled = static_cast<std::uint32_t>(
        (static_cast<std::uint64_t>(code_ - low_) * total) / range);
    std::size_t sym = get_freq_cb(scaled);
    auto [lo_cum, hi_cum] = cum_range_cb(sym);
    high_ = low_ + static_cast<std::uint32_t>((range * hi_cum) / total - 1);
    low_  = low_ + static_cast<std::uint32_t>((range * lo_cum) / total);
    decoder_renormalize();
    return sym;
}
```

---

## Cross-references (summary for prose and nav)

- Forward: post 11, Succinct Bit Vectors and Rank/Select (`/post/2025-06-succinct-wire-formats/`) -- plain text in prose until post 11 is published.
- Back: post 9, Huffman (`/post/2024-08-huffman-wire-formats/`); post 3, Universal Codes as Priors (`/post/2022-01-priors-wire-formats/`).
- Cross-series: Bits Follow Types (Stepanov bridge) -- arithmetic is the entropy-optimal version of the Either combinator's tag bit.
- PFC footnote: `include/pfc/arithmetic_coding.hpp` contains both the integer range coder and a higher-level adaptive variant.

---

## Self-review checklist

Before finalizing this plan file:

- [ ] Zero em-dashes in plan source

```bash
grep -c $'\xe2\x80\x94' \
    /home/spinoza/github/metafunctor-series/wire-formats/docs/superpowers/plans/2026-04-24-3e-arithmetic-coding.md
```

Expected: 0.

- [ ] Zero placeholder strings (no "(TBD)", "(TODO)", "FILL IN", etc.)

```bash
grep -in "tbd\|fill in\|\bfixme\b" \
    /home/spinoza/github/metafunctor-series/wire-formats/docs/superpowers/plans/2026-04-24-3e-arithmetic-coding.md
```

Expected: no output (the word "todo" appears only in git commit messages and step labels, not as a content placeholder).

- [ ] Task count: 18 tasks (Tasks 1 through 18), matching the spec estimate.
- [ ] Approximate line count target: 1500 to 2000 lines (the integer range coder is intricate; embedded code is substantial).
- [ ] No banned phrases (no phrases from the soul banned list, e.g. the "state" + "-of-the-" + "art" construction).

---

## Report

- **Status:** DONE
- **File:** `/home/spinoza/github/metafunctor-series/wire-formats/docs/superpowers/plans/2026-04-24-3e-arithmetic-coding.md`
- **Task count:** 18 tasks
- **Concerns:** None. The `decode_symbol` callback signature (two separate callbacks for the lookup and for the cumulative-range retrieval) is slightly verbose; an implementer may prefer a single struct. Either approach is correct. The convergence test (Task 11) uses a deterministic alternating sequence rather than a random one; this is intentional to make the test reproducible, but means the measured bits-per-symbol will differ from the true entropy by more than a random sequence of the same length would. The tolerance (h + 0.5) in the convergence test is generous for this reason.
